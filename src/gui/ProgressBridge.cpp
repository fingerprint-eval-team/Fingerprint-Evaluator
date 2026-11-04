#include "pch.h"

#include "ProgressBridge.h"

// 메시지 받는 쪽(CMainDlg)이 lParam의 객체를 delete 한다
void CProgressBridge::onPhase(const std::string& phase, size_t total) {
  ::PostMessage(m_target, WM_APP_PHASE, (WPARAM)total, (LPARAM) new std::string(phase));
}

void CProgressBridge::onProgress(size_t done, size_t total, size_t failures) {
  ::PostMessage(m_target, WM_APP_PROGRESS, 0, (LPARAM) new Progress{done, total, failures});
}

void CProgressBridge::onLog(const std::string& line) {
  ::PostMessage(m_target, WM_APP_LOG, 0, (LPARAM) new std::string(line));
}
