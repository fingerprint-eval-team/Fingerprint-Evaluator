#pragma once
#include <windows.h>

namespace fpe {

// HANDLE RAII 래퍼 (CloseHandle 자동 호출)
class UniqueHandle {
 public:
  UniqueHandle() = default;
  explicit UniqueHandle(HANDLE h) : h_(h) {}
  ~UniqueHandle() { reset(); }
  UniqueHandle(const UniqueHandle&) = delete;
  UniqueHandle& operator=(const UniqueHandle&) = delete;
  UniqueHandle(UniqueHandle&& o) noexcept : h_(o.release()) {}
  UniqueHandle& operator=(UniqueHandle&& o) noexcept {
    if (this != &o) reset(o.release());
    return *this;
  }

  HANDLE get() const { return h_; }
  bool valid() const { return h_ != nullptr && h_ != INVALID_HANDLE_VALUE; }
  explicit operator bool() const { return valid(); }
  HANDLE release() {
    HANDLE t = h_;
    h_ = nullptr;
    return t;
  }
  void reset(HANDLE h = nullptr) {
    if (valid()) ::CloseHandle(h_);
    h_ = h;
  }

 private:
  HANDLE h_ = nullptr;
};

}  // namespace fpe
