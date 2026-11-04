// FPWorker32.exe / FPWorker64.exe
// 평가 대상 DLL을 이 프로세스 안에서만 로드해 실행한다. DLL이 죽어도 평가 프로그램은 안전하다.
// 사용법: FPWorker64.exe --pipe \\.\pipe\fpeval_<pid>_<slot>_<gen>
#include <windows.h>

#include <cstdlib>
#include <string>
#include <vector>

#include "CommandExecutor.h"
#include "Handle.h"
#include "Protocol.h"
#include "Status.h"

using namespace fpe;

namespace {

bool readExact(HANDLE h, void* buf, DWORD n) {
  auto* p = static_cast<uint8_t*>(buf);
  while (n > 0) {
    DWORD got = 0;
    if (!ReadFile(h, p, n, &got, nullptr) || got == 0) return false;
    p += got;
    n -= got;
  }
  return true;
}

bool writeExact(HANDLE h, const void* buf, DWORD n) {
  auto* p = static_cast<const uint8_t*>(buf);
  while (n > 0) {
    DWORD put = 0;
    if (!WriteFile(h, p, n, &put, nullptr) || put == 0) return false;
    p += put;
    n -= put;
  }
  return true;
}

bool sendReply(HANDLE h, uint32_t jobId, const Reply& r) {
  MsgHeader hdr{kMagic, (uint32_t)MsgType::Reply, jobId, sizeof(Reply)};
  return writeExact(h, &hdr, sizeof hdr) && writeExact(h, &r, sizeof r);
}

}  // namespace

int wmain(int argc, wchar_t** argv) {
  // 크래시 시 Windows 오류 대화상자가 떠서 멈추지 않도록
  SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
  _set_abort_behavior(0, _WRITE_ABORT_MSG | _CALL_REPORTFAULT);

  std::wstring pipeName;
  for (int i = 1; i + 1 < argc; ++i)
    if (std::wstring(argv[i]) == L"--pipe") pipeName = argv[i + 1];
  if (pipeName.empty()) return 2;

  UniqueHandle pipe(CreateFileW(pipeName.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr));
  if (!pipe) return 2;

  CommandExecutor exec;
  std::vector<uint8_t> body;
  for (;;) {
    MsgHeader hdr{};
    if (!readExact(pipe.get(), &hdr, sizeof hdr) || hdr.magic != kMagic) return 1;
    body.resize(hdr.bodyLen);
    if (hdr.bodyLen && !readExact(pipe.get(), body.data(), hdr.bodyLen)) return 1;

    Reply r{};
    switch ((MsgType)hdr.type) {
      case MsgType::Hello: {
        if (hdr.bodyLen != sizeof(HelloReq)) return 1;
        auto* req = reinterpret_cast<HelloReq*>(body.data());
        req->dllPath[kPathChars - 1] = 0;
        std::string err;
        r = exec.load(req->dllPath, err) ? makeReply((int)Status::OK)
                                          : makeReply((int)Status::InternalError, err.c_str());
        break;
      }
      case MsgType::Enroll:
      case MsgType::Attempt: {
        if (hdr.bodyLen != sizeof(ExtractReq)) return 1;
        auto* req = reinterpret_cast<ExtractReq*>(body.data());
        req->rawPath[kPathChars - 1] = req->outPath[kPathChars - 1] = 0;
        r = exec.extract((MsgType)hdr.type == MsgType::Enroll, *req);
        break;
      }
      case MsgType::Compare: {
        if (hdr.bodyLen != sizeof(CompareReq)) return 1;
        auto* req = reinterpret_cast<CompareReq*>(body.data());
        req->enrollPath[kPathChars - 1] = req->attemptPath[kPathChars - 1] = 0;
        r = exec.compare(*req);
        break;
      }
      case MsgType::Quit:
        return 0;
      default:
        return 1;
    }
    if (!sendReply(pipe.get(), hdr.jobId, r)) return 1;
    // 예외가 난 DLL은 내부 상태를 믿을 수 없으므로 프로세스를 재사용하지 않는다
    if (r.status == (int)Status::Crash) return 3;
  }
}
