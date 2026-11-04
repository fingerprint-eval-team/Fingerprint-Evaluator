#pragma once
#include <windows.h>

#include <cstddef>
#include <cstdint>

namespace fpe {

// 데이터 바로 뒤에 PAGE_NOACCESS 가드 페이지를 둔 버퍼.
// DLL이 입력 범위를 1바이트라도 넘어 읽으면 즉시 Access Violation이 발생해 CRASH로 감지된다.
class GuardedBuffer {
 public:
  explicit GuardedBuffer(size_t size) : size_(size) {
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    size_t page = si.dwPageSize;
    size_t dataPages = size == 0 ? 1 : (size + page - 1) / page;
    total_ = (dataPages + 1) * page;
    base_ = static_cast<uint8_t*>(VirtualAlloc(nullptr, total_, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE));
    if (!base_) return;
    DWORD old = 0;
    VirtualProtect(base_ + dataPages * page, page, PAGE_NOACCESS, &old);
    data_ = base_ + dataPages * page - size;  // 데이터 끝 = 가드 페이지 시작
  }
  ~GuardedBuffer() {
    if (base_) VirtualFree(base_, 0, MEM_RELEASE);
  }
  GuardedBuffer(const GuardedBuffer&) = delete;
  GuardedBuffer& operator=(const GuardedBuffer&) = delete;

  bool ok() const { return data_ != nullptr; }
  uint8_t* data() { return data_; }
  size_t size() const { return size_; }

 private:
  uint8_t* base_ = nullptr;
  uint8_t* data_ = nullptr;
  size_t size_ = 0;
  size_t total_ = 0;
};

}  // namespace fpe
