#pragma once
#include "pch.h"

#include "Evaluator.h"

// IProgressListener(작업 스레드에서 호출) → PostMessage로 UI 스레드에 전달.
// Core는 MFC를 모르고, UI 객체는 UI 스레드에서만 건드린다.
class CProgressBridge : public fpe::IProgressListener {
 public:
  struct Progress {
    size_t done, total, failures;
  };

  explicit CProgressBridge(HWND target) : m_target(target) {}
  void onPhase(const std::string& phase, size_t total) override;
  void onProgress(size_t done, size_t total, size_t failures) override;
  void onLog(const std::string& line) override;

 private:
  HWND m_target;
};
