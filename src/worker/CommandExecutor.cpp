#include "CommandExecutor.h"

#include <climits>
#include <cmath>
#include <cstring>
#include <vector>

#include "GuardedBuffer.h"
#include "PathUtil.h"
#include "Status.h"

namespace fpe {

namespace {

constexpr size_t kCanaryBytes = 4096;
constexpr uint8_t kCanary = 0xCD;
constexpr int kSehCaught = INT_MIN;

// __try 블록이 있는 함수에는 소멸자가 있는 객체를 둘 수 없으므로 호출부만 분리한다.
int callExtractSeh(KisaExtractFn fn, int iType, unsigned char* img, unsigned char* out, int* size, int* ec,
                   char* msg, DWORD* seh) {
  __try {
    return fn(iType, img, out, size, ec, msg);
  } __except (*seh = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
    return kSehCaught;
  }
}

int callCompareSeh(KisaCompareFn fn, unsigned char* e, unsigned char* a, double* score, int* ec, char* msg,
                   DWORD* seh) {
  __try {
    return fn(e, a, score, ec, msg);
  } __except (*seh = GetExceptionCode(), EXCEPTION_EXECUTE_HANDLER) {
    return kSehCaught;
  }
}

int64_t elapsedUs(const LARGE_INTEGER& a, const LARGE_INTEGER& b) {
  static LARGE_INTEGER freq = [] {
    LARGE_INTEGER f;
    QueryPerformanceFrequency(&f);
    return f;
  }();
  return (b.QuadPart - a.QuadPart) * 1000000 / freq.QuadPart;
}

void copyMsg(Reply& r, const char* src) {
  memcpy(r.errMsg, src, 256);
  r.errMsg[255] = '\0';
}

// 안내서 p29: 점수는 0~1이며 최대 소수점 3째 자리까지 표현한다. 부동소수 오차는 허용한다.
bool scoreHasValidPrecision(double s) {
  double scaled = s * 1000.0;
  return std::fabs(scaled - std::round(scaled)) <= 1e-6;
}

bool loadToGuarded(const std::wstring& path, std::vector<uint8_t>& tmp, size_t expect, std::string& err) {
  if (!readFile(path, tmp)) {
    err = "cannot read " + toUtf8(path);
    return false;
  }
  if (expect && tmp.size() != expect) {
    err = "size mismatch " + toUtf8(path);
    return false;
  }
  return true;
}

}  // namespace

Reply makeReply(int status, const char* msg) {
  Reply r{};
  r.status = status;
  r.workerBits = sizeof(void*) * 8;
  if (msg) strncpy_s(r.errMsg, msg, _TRUNCATE);
  return r;
}

CommandExecutor::~CommandExecutor() {
  if (dll_) FreeLibrary(dll_);
}

bool CommandExecutor::load(const std::wstring& dllPath, std::string& err) {
  // DLL 폴더의 부속 DLL을 찾을 수 있도록 altered search path 사용 (안내서 p31)
  dll_ = LoadLibraryExW(dllPath.c_str(), nullptr, LOAD_WITH_ALTERED_SEARCH_PATH);
  if (!dll_) {
    err = "LoadLibrary failed (error " + std::to_string(GetLastError()) + ")";
    return false;
  }
  enroll_ = reinterpret_cast<KisaExtractFn>(GetProcAddress(dll_, kEnrollName));
  attempt_ = reinterpret_cast<KisaExtractFn>(GetProcAddress(dll_, kAttemptName));
  compare_ = reinterpret_cast<KisaCompareFn>(GetProcAddress(dll_, kCompareName));
  if (!loaded()) {
    err = "missing export:";
    if (!enroll_) err += " KISA_Enroll";
    if (!attempt_) err += " KISA_Attempt";
    if (!compare_) err += " KISA_Compare";
    return false;
  }
  return true;
}

Reply CommandExecutor::extract(bool isEnroll, const ExtractReq& req) {
  if (!loaded()) return makeReply((int)Status::InternalError, "dll not loaded");
  size_t imgSize = (size_t)req.width * (size_t)req.height;
  std::vector<uint8_t> tmp;
  std::string err;
  if (!loadToGuarded(req.rawPath, tmp, imgSize, err)) return makeReply((int)Status::InternalError, err.c_str());

  GuardedBuffer img(imgSize);
  if (!img.ok()) return makeReply((int)Status::InternalError, "VirtualAlloc failed");
  memcpy(img.data(), tmp.data(), imgSize);

  std::vector<uint8_t> out(kMaxTemplateSize + kCanaryBytes, kCanary);
  int size = -1, ec = 0;
  char msg[256] = {};
  DWORD seh = 0;

  LARGE_INTEGER t0, t1;
  QueryPerformanceCounter(&t0);
  int ret = callExtractSeh(isEnroll ? enroll_ : attempt_, req.iType, img.data(), out.data(), &size, &ec, msg, &seh);
  QueryPerformanceCounter(&t1);

  Reply r = makeReply((int)Status::OK);
  r.retVal = ret;
  r.errCode = ec;
  r.outSize = size;
  r.elapsedUs = elapsedUs(t0, t1);
  copyMsg(r, msg);

  if (ret == kSehCaught) {
    r.status = (int)Status::Crash;
    r.sehCode = seh;
    return r;
  }
  for (size_t i = kMaxTemplateSize; i < out.size(); ++i) {
    if (out[i] != kCanary) {
      r.status = (int)Status::Overflow;
      return r;
    }
  }
  // 안내서 p26·p28: 성공 1, 실패 0만 규격. 그 밖의 값은 출력과 동작이 맞지 않는 것으로 본다.
  if (ret == 0) {
    r.status = (int)Status::Fail;
    return r;
  }
  if (ret != 1) {
    r.status = (int)Status::Mismatch;
    snprintf(r.errMsg, sizeof(r.errMsg), "return value %d (spec: 1 or 0)", ret);
    return r;
  }
  if (size <= 0 || size >= kMaxTemplateSize) {
    r.status = (int)Status::Mismatch;
    snprintf(r.errMsg, sizeof(r.errMsg), "returned 1 but template size=%d", size);
    return r;
  }
  if (!writeFile(req.outPath, out.data(), (size_t)size)) {
    r.status = (int)Status::InternalError;
    strncpy_s(r.errMsg, "cannot write template", _TRUNCATE);
  }
  return r;
}

Reply CommandExecutor::compare(const CompareReq& req) {
  if (!loaded()) return makeReply((int)Status::InternalError, "dll not loaded");
  std::vector<uint8_t> te, ta;
  std::string err;
  if (!loadToGuarded(req.enrollPath, te, 0, err) || !loadToGuarded(req.attemptPath, ta, 0, err))
    return makeReply((int)Status::InternalError, err.c_str());

  // 템플릿도 크기만큼만 할당하고 뒤에 가드 페이지 → 템플릿 범위 밖 읽기 감지
  GuardedBuffer be(te.size()), ba(ta.size());
  memcpy(be.data(), te.data(), te.size());
  memcpy(ba.data(), ta.data(), ta.size());

  double score = -1.0;
  int ec = 0;
  char msg[256] = {};
  DWORD seh = 0;
  LARGE_INTEGER t0, t1;
  QueryPerformanceCounter(&t0);
  int ret = callCompareSeh(compare_, be.data(), ba.data(), &score, &ec, msg, &seh);
  QueryPerformanceCounter(&t1);

  Reply r = makeReply((int)Status::OK);
  r.retVal = ret;
  r.errCode = ec;
  r.score = score;
  r.elapsedUs = elapsedUs(t0, t1);
  copyMsg(r, msg);

  if (ret == kSehCaught) {
    r.status = (int)Status::Crash;
    r.sehCode = seh;
  } else if (ret == 0) {
    r.status = (int)Status::Fail;
  } else if (ret != 1) {
    r.status = (int)Status::Mismatch;
    snprintf(r.errMsg, sizeof(r.errMsg), "return value %d (spec: 1 or 0)", ret);
  } else if (std::isnan(score) || score < 0.0 || score > 1.0) {
    r.status = (int)Status::Mismatch;
    snprintf(r.errMsg, sizeof(r.errMsg), "returned 1 but score=%g", score);
  } else if (!scoreHasValidPrecision(score)) {
    r.status = (int)Status::Mismatch;
    snprintf(r.errMsg, sizeof(r.errMsg), "score %.17g has more than 3 decimal places", score);
  }
  return r;
}

}  // namespace fpe
