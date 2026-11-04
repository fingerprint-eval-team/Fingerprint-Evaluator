#pragma once
#include <windows.h>

#include <string>

#include "Handle.h"
#include "Protocol.h"

namespace fpe {

// Worker 프로세스 하나와 Named Pipe 연결을 관리한다 (RAII: 소멸 시 종료)
class WorkerProcess {
 public:
  enum class Io { Ok, Timeout, Broken, Cancelled };

  WorkerProcess(std::wstring workerExe, HANDLE job, int slot);
  ~WorkerProcess();
  WorkerProcess(const WorkerProcess&) = delete;
  WorkerProcess& operator=(const WorkerProcess&) = delete;

  bool start(std::string& err, DWORD connectTimeoutMs = 10000);
  bool running() const { return proc_.valid(); }

  // 요청 1개를 보내고 응답 1개를 받는다. timeout/cancel 시 Worker는 그대로 두므로 호출자가 terminate().
  Io request(MsgType type, uint32_t jobId, const void* body, uint32_t len, Reply& out, DWORD timeoutMs,
             HANDLE cancelEvent);

  void terminate();          // 즉시 강제 종료
  void quit();               // QUIT 전송 후 정상 종료 대기
  DWORD exitCode() const { return lastExit_; }

 private:
  Io transfer(bool write, void* buf, DWORD len, ULONGLONG deadline, HANDLE cancelEvent);

  std::wstring exe_;
  HANDLE job_;
  int slot_;
  int gen_ = 0;
  UniqueHandle pipe_, proc_, ioEvent_;
  DWORD lastExit_ = 0;
};

}  // namespace fpe
