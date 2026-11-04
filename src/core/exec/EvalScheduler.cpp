#include "exec/EvalScheduler.h"

#include <cstring>
#include <mutex>
#include <thread>

#include "exec/WorkerProcess.h"

namespace fpe {

namespace {
std::mutex g_fatalMu;

bool hello(WorkerProcess& wp, const std::wstring& dll, HANDLE cancel, std::string& err) {
  HelloReq req{};
  wcsncpy_s(req.dllPath, dll.c_str(), _TRUNCATE);
  Reply r{};
  auto io = wp.request(MsgType::Hello, 0, &req, sizeof req, r, 15000, cancel);
  if (io != WorkerProcess::Io::Ok) {
    err = io == WorkerProcess::Io::Timeout ? "DLL load timeout (DllMain hang?)" : "worker died while loading DLL";
    return false;
  }
  if (r.status != (int)Status::OK) {
    err = r.errMsg;
    return false;
  }
  return true;
}
}  // namespace

EvalScheduler::EvalScheduler(Options opt) : opt_(std::move(opt)) {
  job_.reset(CreateJobObjectW(nullptr, nullptr));
  if (job_) {
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION info{};
    info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    SetInformationJobObject(job_.get(), JobObjectExtendedLimitInformation, &info, sizeof info);
  }
  cancel_.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
}

EvalScheduler::~EvalScheduler() = default;

void EvalScheduler::abort() {
  stop_ = true;
  aborted_ = true;
  SetEvent(cancel_.get());
}

bool EvalScheduler::probe(std::string& err) {
  WorkerProcess wp(opt_.workerExe, job_.get(), 99);
  if (!wp.start(err)) return false;
  bool ok = hello(wp, opt_.dllPath, cancel_.get(), err);
  wp.quit();
  return ok;
}

bool EvalScheduler::run(size_t count, const MakeFn& make, const DoneFn& done, std::string& fatalErr) {
  next_ = 0;
  fatal_ = false;
  fatalMsg_.clear();
  int n = std::max(1, std::min<int>(opt_.workers, (int)std::max<size_t>(count, 1)));
  std::vector<std::thread> threads;
  for (int s = 0; s < n; ++s) threads.emplace_back([=, &make, &done] { slotLoop(s, count, make, done); });
  for (auto& t : threads) t.join();
  if (fatal_) {
    fatalErr = fatalMsg_;
    return false;
  }
  return true;
}

void EvalScheduler::slotLoop(int slot, size_t count, const MakeFn& make, const DoneFn& done) {
  WorkerProcess wp(opt_.workerExe, job_.get(), slot);
  bool needStart = true;

  while (!stop_ && !fatal_) {
    size_t idx = next_++;
    if (idx >= count) break;

    if (needStart) {
      // Worker 시작 + DLL 로드, 3번까지 재시도. 계속 실패하면 DLL 자체 문제로 보고 전체 중단.
      std::string err;
      bool ok = false;
      for (int t = 0; t < 3 && !aborted_ && !ok; ++t) ok = wp.start(err) && hello(wp, opt_.dllPath, cancel_.get(), err);
      if (!ok) {
        if (!aborted_) {
          std::lock_guard<std::mutex> lk(g_fatalMu);
          if (!fatal_) fatalMsg_ = err;
          fatal_ = true;
        }
        break;
      }
      needStart = false;
    }

    WorkItem item = make(idx);
    WorkResult res;
    auto io = wp.request(item.type, item.jobId, item.body.data(), (uint32_t)item.body.size(), res.reply,
                         opt_.timeoutMs, cancel_.get());
    switch (io) {
      case WorkerProcess::Io::Ok:
        res.status = (Status)res.reply.status;
        if (res.status == Status::Crash) {  // Worker가 SEH로 잡고 스스로 종료함
          wp.terminate();
          needStart = true;
        }
        break;
      case WorkerProcess::Io::Timeout:
        wp.terminate();
        needStart = true;
        res.status = Status::Timeout;
        res.reply.elapsedUs = (int64_t)opt_.timeoutMs * 1000;
        snprintf(res.reply.errMsg, sizeof res.reply.errMsg, "no response within %lu ms", opt_.timeoutMs);
        break;
      case WorkerProcess::Io::Broken:
        wp.terminate();
        needStart = true;
        res.status = Status::Crash;
        res.reply.sehCode = wp.exitCode();
        snprintf(res.reply.errMsg, sizeof res.reply.errMsg, "worker terminated (exit 0x%08lX)", wp.exitCode());
        break;
      case WorkerProcess::Io::Cancelled:
        wp.terminate();
        res.status = Status::Aborted;
        done(idx, res);
        return;
    }
    done(idx, res);
  }
  wp.quit();
}

}  // namespace fpe
