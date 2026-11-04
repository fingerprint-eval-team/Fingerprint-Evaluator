#pragma once
#include "pch.h"

#include <memory>
#include <thread>

#include "Evaluator.h"
#include "Pages.h"
#include "ProgressBridge.h"
#include "db/FingerprintDB.h"

// 메인 대화상자: 탭 4개(대상 / 시험 규칙 / 실행 옵션 / 리포트) + 실행 영역(버튼·진행률·로그)
// 평가는 백그라운드 스레드에서 실행하고, 진행 상황은 CProgressBridge의 PostMessage로 받는다.
class CMainDlg : public CDialogEx {
 public:
  CMainDlg() : CDialogEx(IDD_FPEVAL) {}

 protected:
  void DoDataExchange(CDataExchange* pDX) override;
  BOOL OnInitDialog() override;
  void OnOK() override {}      // Enter로 닫히지 않게
  void OnCancel() override {}  // Esc로 닫히지 않게 (닫기는 OnClose)

  afx_msg void OnTabChange(NMHDR* nm, LRESULT* result);
  afx_msg void OnStart();
  afx_msg void OnStop();
  afx_msg void OnAbort();
  afx_msg void OnResume();
  afx_msg void OnReportOnly();
  afx_msg void OnSaveConfig();
  afx_msg void OnLoadConfig();
  afx_msg void OnOpenReport();
  afx_msg void OnOpenFolder();
  afx_msg void OnClose();
  afx_msg LRESULT OnPhase(WPARAM, LPARAM);
  afx_msg LRESULT OnProgress(WPARAM, LPARAM);
  afx_msg LRESULT OnLog(WPARAM, LPARAM);
  afx_msg LRESULT OnDone(WPARAM, LPARAM);
  afx_msg LRESULT OnDbChanged(WPARAM, LPARAM);
  afx_msg LRESULT OnRulesChanged(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

 private:
  bool GatherConfig(fpe::EvalConfig& c);
  void ApplyConfig(const fpe::EvalConfig& c);
  void ShowPage(int index);
  void SetRunning(bool running);
  void AppendLog(const CString& s);
  void StartRun(const std::wstring& resumeDir);
  void JoinWorker();
  std::wstring LastConfigPath() const;

  CTabCtrl m_tab;
  CProgressCtrl m_progress;
  CListBox m_log;
  CTargetPage m_target;
  CRulePage m_rules;
  CRunPage m_run;
  CReportPage m_report;
  CPageBase* m_pages[4] = {&m_target, &m_rules, &m_run, &m_report};

  fpe::FingerprintDB m_db;
  bool m_dbOk = false;
  std::unique_ptr<CProgressBridge> m_bridge;
  std::unique_ptr<fpe::Evaluator> m_eval;
  std::thread m_worker;
  bool m_running = false;
  CString m_phase, m_lastRunDir, m_lastHtml;
};
