#include "exec/WorkerProcess.h"

#include <vector>

namespace fpe {

WorkerProcess::WorkerProcess(std::wstring workerExe, HANDLE job, int slot)
    : exe_(std::move(workerExe)), job_(job), slot_(slot) {
  ioEvent_.reset(CreateEventW(nullptr, TRUE, FALSE, nullptr));
}

WorkerProcess::~WorkerProcess() { terminate(); }

bool WorkerProcess::start(std::string& err, DWORD connectTimeoutMs) {
  terminate();
  ++gen_;
  wchar_t name[128];
  swprintf(name, 128, L"\\\\.\\pipe\\fpeval_%lu_%d_%d", GetCurrentProcessId(), slot_, gen_);
  pipe_.reset(CreateNamedPipeW(name, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
                               PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 1 << 16, 1 << 16, 0, nullptr));
  if (!pipe_) {
    err = "CreateNamedPipe failed";
    return false;
  }
  OVERLAPPED ov{};
  ov.hEvent = ioEvent_.get();
  ResetEvent(ov.hEvent);
  BOOL connected = ConnectNamedPipe(pipe_.get(), &ov);
  DWORD ce = GetLastError();
  if (connected || ce == ERROR_PIPE_CONNECTED) SetEvent(ov.hEvent);
  else if (ce != ERROR_IO_PENDING) {
    err = "ConnectNamedPipe failed";
    return false;
  }

  std::wstring cmd = L"\"" + exe_ + L"\" --pipe " + name;
  std::vector<wchar_t> cmdBuf(cmd.begin(), cmd.end());
  cmdBuf.push_back(0);
  STARTUPINFOW si{sizeof si};
  PROCESS_INFORMATION pi{};
  if (!CreateProcessW(exe_.c_str(), cmdBuf.data(), nullptr, nullptr, FALSE, CREATE_NO_WINDOW | CREATE_SUSPENDED,
                      nullptr, nullptr, &si, &pi)) {
    CancelIoEx(pipe_.get(), &ov);
    err = "cannot start worker (error " + std::to_string(GetLastError()) + ")";
    return false;
  }
  // GUI가 죽으면 Worker도 함께 종료되도록 Job Object에 넣은 뒤 실행
  if (job_) AssignProcessToJobObject(job_, pi.hProcess);
  ResumeThread(pi.hThread);
  CloseHandle(pi.hThread);
  proc_.reset(pi.hProcess);

  HANDLE waits[2] = {ov.hEvent, proc_.get()};
  DWORD w = WaitForMultipleObjects(2, waits, FALSE, connectTimeoutMs);
  if (w != WAIT_OBJECT_0) {
    CancelIoEx(pipe_.get(), &ov);
    DWORD dummy;
    GetOverlappedResult(pipe_.get(), &ov, &dummy, TRUE);
    err = w == WAIT_OBJECT_0 + 1 ? "worker exited before connecting" : "worker connect timeout";
    terminate();
    return false;
  }
  return true;
}

WorkerProcess::Io WorkerProcess::transfer(bool write, void* buf, DWORD len, ULONGLONG deadline,
                                          HANDLE cancelEvent) {
  auto* p = static_cast<uint8_t*>(buf);
  while (len > 0) {
    OVERLAPPED ov{};
    ov.hEvent = ioEvent_.get();
    ResetEvent(ov.hEvent);
    DWORD n = 0;
    BOOL ok = write ? WriteFile(pipe_.get(), p, len, nullptr, &ov) : ReadFile(pipe_.get(), p, len, nullptr, &ov);
    if (!ok && GetLastError() != ERROR_IO_PENDING) return Io::Broken;

    ULONGLONG now = GetTickCount64();
    DWORD wait = deadline > now ? (DWORD)(deadline - now) : 0;
    HANDLE hs[2] = {ov.hEvent, cancelEvent};
    DWORD w = WaitForMultipleObjects(cancelEvent ? 2 : 1, hs, FALSE, wait);
    if (w != WAIT_OBJECT_0) {
      CancelIoEx(pipe_.get(), &ov);
      GetOverlappedResult(pipe_.get(), &ov, &n, TRUE);
      return w == WAIT_OBJECT_0 + 1 ? Io::Cancelled : Io::Timeout;
    }
    if (!GetOverlappedResult(pipe_.get(), &ov, &n, FALSE) || n == 0) return Io::Broken;
    p += n;
    len -= n;
  }
  return Io::Ok;
}

WorkerProcess::Io WorkerProcess::request(MsgType type, uint32_t jobId, const void* body, uint32_t len, Reply& out,
                                         DWORD timeoutMs, HANDLE cancelEvent) {
  if (!pipe_) return Io::Broken;
  ULONGLONG deadline = GetTickCount64() + timeoutMs;
  MsgHeader hdr{kMagic, (uint32_t)type, jobId, len};
  Io r = transfer(true, &hdr, sizeof hdr, deadline, cancelEvent);
  if (r == Io::Ok && len) r = transfer(true, const_cast<void*>(body), len, deadline, cancelEvent);
  if (r != Io::Ok) return r;
  MsgHeader rh{};
  r = transfer(false, &rh, sizeof rh, deadline, cancelEvent);
  if (r != Io::Ok) return r;
  if (rh.magic != kMagic || rh.type != (uint32_t)MsgType::Reply || rh.bodyLen != sizeof(Reply)) return Io::Broken;
  return transfer(false, &out, sizeof out, deadline, cancelEvent);
}

void WorkerProcess::terminate() {
  if (proc_) {
    DWORD code = STILL_ACTIVE;
    GetExitCodeProcess(proc_.get(), &code);
    if (code == STILL_ACTIVE) {
      TerminateProcess(proc_.get(), 0xDEAD);
      WaitForSingleObject(proc_.get(), 5000);
    }
    GetExitCodeProcess(proc_.get(), &lastExit_);
  }
  proc_.reset();
  pipe_.reset();
}

void WorkerProcess::quit() {
  if (pipe_ && proc_) {
    Reply dummy{};
    MsgHeader hdr{kMagic, (uint32_t)MsgType::Quit, 0, 0};
    transfer(true, &hdr, sizeof hdr, GetTickCount64() + 1000, nullptr);
    WaitForSingleObject(proc_.get(), 2000);
    (void)dummy;
  }
  terminate();
}

}  // namespace fpe
