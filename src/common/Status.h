#pragma once
#include <cstdint>
#include <string>

namespace fpe {

// 작업 하나의 결과 상태. 지표 계산 시 반영 방식은 MetricsCalculator 참고.
enum class Status : int32_t {
  OK = 0,
  Fail = 1,          // DLL이 0 이하 반환
  Timeout = 2,       // 제한 시간 초과 → Worker 강제 종료
  Crash = 3,         // 예외/비정상 종료
  Mismatch = 4,      // 반환 1인데 크기·점수가 규격 밖
  Overflow = 5,      // 출력 버퍼 canary 훼손
  SkipEF = 6,        // 등록 템플릿이 없어 비교하지 않음
  SkipAF = 7,        // 비교용(Attempt) 템플릿이 없어 비교하지 않음
  Aborted = 8,       // 사용자 강제 종료 순간 진행 중 → 집계 제외
  InternalError = 9  // 평가 프로그램 쪽 오류(파일 없음 등)
};

inline const char* toString(Status s) {
  switch (s) {
    case Status::OK: return "OK";
    case Status::Fail: return "FAIL";
    case Status::Timeout: return "TIMEOUT";
    case Status::Crash: return "CRASH";
    case Status::Mismatch: return "MISMATCH";
    case Status::Overflow: return "OVERFLOW";
    case Status::SkipEF: return "SKIP_EF";
    case Status::SkipAF: return "SKIP_AF";
    case Status::Aborted: return "ABORTED";
    case Status::InternalError: return "INTERNAL";
  }
  return "?";
}

inline Status statusFromString(const std::string& s) {
  for (int i = 0; i <= 9; ++i)
    if (s == toString(static_cast<Status>(i))) return static_cast<Status>(i);
  return Status::InternalError;
}

// DLL을 실제로 호출했고 실패한 경우
inline bool isCallFailure(Status s) {
  return s == Status::Fail || s == Status::Timeout || s == Status::Crash || s == Status::Mismatch ||
         s == Status::Overflow;
}

// 로그에 남아 "완료"로 취급되는 상태 (이어하기 시 건너뜀)
inline bool isFinal(Status s) { return s != Status::Aborted && s != Status::InternalError; }

}  // namespace fpe
