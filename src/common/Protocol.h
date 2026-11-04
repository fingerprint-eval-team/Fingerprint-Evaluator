#pragma once
// GUI(FPEval) <-> Worker(FPWorker32/64) Named Pipe 메시지 규격.
// x86/x64 프로세스가 같은 바이트 배치를 쓰도록 고정 크기 타입과 pack(1)만 사용한다.
#include <cstdint>

namespace fpe {

constexpr uint32_t kMagic = 0x56455046;          // 'FPEV'
constexpr int32_t kMaxTemplateSize = 1000000;    // K-NBTC 규격: 1,000,000 byte 미만
constexpr int kPathChars = 520;

enum class MsgType : uint32_t { Hello = 1, Enroll = 2, Attempt = 3, Compare = 4, Quit = 9, Reply = 100 };

#pragma pack(push, 1)
struct MsgHeader {
  uint32_t magic;
  uint32_t type;
  uint32_t jobId;
  uint32_t bodyLen;
};

struct HelloReq {
  wchar_t dllPath[kPathChars];
};

struct ExtractReq {
  int32_t iType;
  int32_t width;
  int32_t height;
  wchar_t rawPath[kPathChars];
  wchar_t outPath[kPathChars];
};

struct CompareReq {
  wchar_t enrollPath[kPathChars];
  wchar_t attemptPath[kPathChars];
};

struct Reply {
  int32_t status;      // fpe::Status
  int32_t retVal;      // DLL 반환값 그대로
  int32_t errCode;     // piErrorCode
  char errMsg[256];    // arErrorMessage (항상 NUL 종료되도록 Worker가 보정)
  int32_t outSize;     // 추출된 템플릿 크기
  double score;        // 비교 점수
  int64_t elapsedUs;   // DLL 함수 호출 구간만 측정
  uint32_t sehCode;    // CRASH일 때 예외 코드
  int32_t workerBits;  // 32 / 64
};
#pragma pack(pop)

static_assert(sizeof(wchar_t) == 2, "Windows only");

}  // namespace fpe
