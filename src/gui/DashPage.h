#pragma once
#include "pch.h"

#include <chrono>

#include "Evaluator.h"

// 실행 현황 페이지: 상태 문구 + 수치 카드 4개 + 단계(STEP 0~5) 진행 표시 + 실행 로그.
// 평가 중에는 현재 단계 진행률·완료 수·실패 수·경과 시간을, 완료 후에는 EER·FMR100·FTE·소요 시간을 보여 준다.
// 값은 CMainDlg가 CProgressBridge 메시지를 받아 UI 스레드에서 넘겨준다 (이 페이지는 작업 스레드를 모름).
class CDashPage : public CDialogEx {
 public:
  CDashPage() : CDialogEx(IDD_PAGE_DASH) {}

  void Begin(bool reportOnly);  // 평가·리포트 생성 시작: 표시값 초기화, 경과 시간 타이머 시작
  void SetPhase(const CString& phase);
  void SetProgress(size_t done, size_t total, size_t failures);
  void SetStatusText(const CString& text);  // 중지 요청 등 상태 문구만 바꿀 때
  void Finish(const fpe::RunOutcome& outcome);
  void AppendLog(const CString& line);
  int Percent() const;  // 현재 단계 진행률 (사이드바 표시용), 실행 중이 아니면 -1

 protected:
  void DoDataExchange(CDataExchange* pDX) override;
  BOOL OnInitDialog() override;
  void OnOK() override {}
  void OnCancel() override {}
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg void OnTimer(UINT_PTR id);
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor);
  afx_msg LRESULT OnDpiChangedAfterParent(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

 private:
  enum class State { Idle, Running, Done, Stopped, Failed };
  void CreateFonts();
  CRect Dlu(int l, int t, int r, int b);
  void DrawCard(CDC& dc, const CRect& rc, const CString& label, const CString& value, COLORREF valueColor);
  void DrawSteps(CDC& dc, const CRect& rc);
  CString Elapsed() const;

  CListBox m_log;
  CFont m_captionFont, m_labelFont, m_valueFont, m_stepFont, m_iconFont, m_sectionFont;
  State m_state = State::Idle;
  bool m_reportOnly = false;
  int m_step = -1;  // 현재 STEP 번호 (0~5)
  CString m_phase, m_status;
  size_t m_done = 0, m_total = 0, m_failures = 0;
  std::chrono::steady_clock::time_point m_start, m_end;
  double m_eer = 0, m_fmr100 = 0, m_fte = 0;
};
