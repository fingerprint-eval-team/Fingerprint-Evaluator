#pragma once
#include "pch.h"

#include <chrono>
#include <memory>
#include <thread>

#include "AccentButton.h"
#include "DashPage.h"
#include "Evaluator.h"
#include "HomeView.h"
#include "NavBar.h"
#include "Pages.h"
#include "ProgressBridge.h"
#include "ResultsPage.h"
#include "db/FingerprintDB.h"

// 메인 대화상자: 왼쪽 사이드바(CNavBar), 오른쪽 페이지, 아래 실행 막대.
// 처음에는 홈 화면(CHomeView)이 클라이언트 전체를 덮는다. 홈에서 항목을 고르거나 ShowPage를 부르면 홈이 닫히고,
// 사이드바의 앱 로고를 누르면 다시 홈으로 온다. 평가는 홈에 있는 동안에도 계속 진행된다.
// 평가는 백그라운드 스레드에서 실행하고, 진행 상황은 CProgressBridge의 PostMessage로 받는다.
// 창 크기 조절은 SetupLayout()의 MFC 동적 레이아웃이 맡고, 페이지와 홈 화면만 OnSize에서 맞춘다.
class CMainDlg : public CDialogEx {
 public:
  CMainDlg() : CDialogEx(IDD_FPEVAL) {}
  ~CMainDlg() override;

 protected:
  void DoDataExchange(CDataExchange* pDX) override;
  BOOL OnInitDialog() override;
  void OnOK() override {}      // Enter로 닫히지 않게
  void OnCancel() override {}  // Esc로 닫히지 않게 (닫기는 OnClose)

  afx_msg LRESULT OnNav(WPARAM page, LPARAM);
  afx_msg LRESULT OnNavToggle(WPARAM, LPARAM);
  afx_msg LRESULT OnNavHome(WPARAM, LPARAM);
  afx_msg LRESULT OnHomeCmd(WPARAM cmd, LPARAM);
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
  afx_msg void OnAbout();
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg BOOL OnEraseBkgnd(CDC* dc);
  afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor);
  afx_msg void OnGetMinMaxInfo(MINMAXINFO* mmi);
  afx_msg LRESULT OnDpiChanged(WPARAM, LPARAM);
  afx_msg LRESULT OnPhase(WPARAM, LPARAM);
  afx_msg LRESULT OnProgress(WPARAM, LPARAM);
  afx_msg LRESULT OnLog(WPARAM, LPARAM);
  afx_msg LRESULT OnDone(WPARAM, LPARAM);
  afx_msg LRESULT OnDbChanged(WPARAM, LPARAM);
  afx_msg LRESULT OnRulesChanged(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

 private:
  static const int kRunOptionsPage = 2;
  static const int kDashPage = 4;  // 실행 현황 페이지 번호
  static const int kResultsPage = 5;
  static const int kPageCount = 6;
  static const int kResumePage = 6;  // 같은 목록 화면을 이어하기 모드로 표시
  static const int kConfigPage = 7;

  bool GatherConfig(fpe::EvalConfig& c);
  void ApplyConfig(const fpe::EvalConfig& c);
  void ShowPage(int index);  // 홈 화면이 떠 있으면 닫고 설정·실행 화면으로 돌아온다
  void ShowHome();
  void ApplyConfigFile(const std::wstring& path);
  void SetRunning(bool running);
  void SetStatus(const CString& s);
  void AppendLog(const CString& s);
  void StartRun(const std::wstring& resumeDir);
  void JoinWorker();
  std::wstring LastConfigPath() const;
  CWnd* Page(int index);
  void LayoutPages();
  CRect PanelRect();          // 흰 패널 영역 (클라이언트 좌표)
  void CreateDpiResources();  // 제목 글꼴·아이콘 (DPI가 바뀌면 다시 만든다)
  void DestroyIcons();
  CSize MinClientSize(bool compact);  // 창을 줄일 수 있는 최소 클라이언트 크기 (픽셀)
  void SetupLayout();         // 창 크기를 바꿀 때 컨트롤이 움직이는 방식 (MFC 동적 레이아웃, 컨트롤 ID 기준)
  void SetCompact(bool compact);  // 사이드바 접기/펼치기: 오른쪽 컨트롤을 사이드바 폭만큼 옮긴다
  void FitToScreen();         // 화면보다 창이 크면 사이드바를 접고 창을 화면 안으로 줄인다

  CNavBar m_nav;
  CAccentButton m_start;
  std::vector<std::unique_ptr<CAccentButton>> m_buttons;  // 중지·이어하기 등 나머지 버튼 (흰 바탕 둥근 버튼)
  CFont m_titleFont;
  HICON m_navIcon = nullptr, m_bigIcon = nullptr, m_smallIcon = nullptr;
  CProgressCtrl m_progress;
  CTargetPage m_target;
  CRulePage m_rules;
  CRunPage m_run;
  CReportPage m_report;
  CPageBase* m_pages[4] = {&m_target, &m_rules, &m_run, &m_report};
  CDashPage m_dash;
  CResultsPage m_results;
  CHomeView m_home;
  bool m_homeShown = false;
  int m_page = 0;

  fpe::FingerprintDB m_db;
  bool m_dbOk = false;
  std::unique_ptr<CProgressBridge> m_bridge;
  std::unique_ptr<fpe::Evaluator> m_eval;
  std::thread m_worker;
  bool m_running = false;
  std::chrono::steady_clock::time_point m_phaseStarted;
  CString m_phase, m_lastRunDir, m_lastReport;  // m_lastReport: 생성된 리포트(HTML 우선, 없으면 Word)
};
