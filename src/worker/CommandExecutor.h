#pragma once
#include <windows.h>

#include <string>

#include "KisaApi.h"
#include "Protocol.h"

namespace fpe {

// 평가 대상 DLL을 로드하고 KISA_* 호출을 실행·검사하는 클래스 (Worker 프로세스 안에서만 사용)
class CommandExecutor {
 public:
  ~CommandExecutor();
  bool load(const std::wstring& dllPath, std::string& err);
  bool loaded() const { return enroll_ && attempt_ && compare_; }

  Reply extract(bool isEnroll, const ExtractReq& req);
  Reply compare(const CompareReq& req);

 private:
  HMODULE dll_ = nullptr;
  KisaExtractFn enroll_ = nullptr;
  KisaExtractFn attempt_ = nullptr;
  KisaCompareFn compare_ = nullptr;
};

Reply makeReply(int status, const char* msg = nullptr);

}  // namespace fpe
