#pragma once
#include <atomic>
#include <mutex>
#include <string>

#include "config/EvalConfig.h"
#include "metrics/MetricsCalculator.h"

namespace fpe {

class EvalScheduler;

// 진행 상황 통지 (옵저버). Core는 GUI를 모르고 이 인터페이스로만 알린다.
class IProgressListener {
 public:
  virtual ~IProgressListener() = default;
  virtual void onPhase(const std::string& phase, size_t total) = 0;
  virtual void onProgress(size_t done, size_t total, size_t failures) = 0;
  virtual void onLog(const std::string& line) = 0;
};

struct RunOutcome {
  bool ok = false;
  bool stopped = false;
  std::string message;
  std::wstring runDir;
  std::wstring htmlPath, docxPath;  // 실제로 생성된 리포트만 채운다
  std::wstring logPath;
  size_t incompleteJobs = 0;        // 계획 중 아직 결과가 없는 작업 수 (중단된 평가)
  MetricsResult metrics;
};

// 평가 전체 흐름: STEP0 검증 → 1 이미지 캐시 → 2 계획 → 3 특징 추출 → 4 비교 → 5 지표·리포트
class Evaluator {
 public:
  Evaluator(EvalConfig cfg, IProgressListener& listener);
  ~Evaluator();

  RunOutcome run(const std::wstring& resumeDir = L"");

  // 결과 폴더의 로그만으로 리포트 재생성. reportCfg가 있으면 그 리포트 항목·수식 설정을 사용.
  static RunOutcome reportOnly(const std::wstring& runDir, const EvalConfig* reportCfg, IProgressListener& l);

  void requestStop();
  void abort();

 private:
  EvalConfig cfg_;
  IProgressListener& l_;
  std::mutex mu_;
  EvalScheduler* sched_ = nullptr;
  std::atomic<bool> stop_{false}, abort_{false};
};

}  // namespace fpe
