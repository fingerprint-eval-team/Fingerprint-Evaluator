#pragma once
#include <windows.h>

#include <atomic>
#include <functional>
#include <string>
#include <vector>

#include "Handle.h"
#include "Protocol.h"
#include "Status.h"

namespace fpe {

struct WorkItem {
  uint32_t jobId = 0;
  MsgType type = MsgType::Enroll;
  std::vector<uint8_t> body;
};

struct WorkResult {
  Status status = Status::OK;
  Reply reply{};
};

// Worker 풀로 작업 목록을 병렬 실행한다. timeout·crash 시 Worker를 재시작한다.
class EvalScheduler {
 public:
  struct Options {
    std::wstring workerExe;
    std::wstring dllPath;
    int workers = 1;
    DWORD timeoutMs = 10000;
  };

  explicit EvalScheduler(Options opt);
  ~EvalScheduler();

  using MakeFn = std::function<WorkItem(size_t index)>;
  using DoneFn = std::function<void(size_t index, const WorkResult& r)>;

  // count개의 작업 실행. 치명적 오류(DLL 로드 실패 등)면 false.
  bool run(size_t count, const MakeFn& make, const DoneFn& done, std::string& fatalErr);

  // Worker 1개로 DLL 로드만 확인
  bool probe(std::string& err);

  void requestStop() { stop_ = true; }        // 새 작업 배분 중단, 진행 중 작업은 마침
  void abort();                                // 진행 중 작업까지 즉시 종료
  bool stopRequested() const { return stop_; }
  bool aborted() const { return aborted_; }

 private:
  void slotLoop(int slot, size_t count, const MakeFn& make, const DoneFn& done);

  Options opt_;
  UniqueHandle job_, cancel_;
  std::atomic<size_t> next_{0};
  std::atomic<bool> stop_{false}, aborted_{false}, fatal_{false};
  std::string fatalMsg_;
};

}  // namespace fpe
