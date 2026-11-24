#pragma once
#include "pch.h"

#include <vector>

// 시작 화면(스플래시): 테두리 없는 창. 왼쪽은 강조색 패널(아이콘·이름·버전), 오른쪽은 시작 점검 목록과 진행 막대.
// 점검은 실제로 한다 (Worker 실행 파일, 최근 설정, 사용자 데이터 폴더). 결과는 목록에 ✓/✗로 표시만 하고
// 프로그램 시작을 막지는 않는다 (Worker가 없으면 평가 시작 때 엔진이 다시 오류를 낸다).
// 사용: Run()이 창을 띄우고 점검이 끝날 때까지 자체 메시지 루프를 돌린 뒤 창을 닫는다. 클릭이나 Esc로 건너뛸 수 있다.
class CSplashWnd : public CWnd {
 public:
  void Run();

 protected:
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg void OnTimer(UINT_PTR id);
  afx_msg void OnLButtonDown(UINT flags, CPoint pt);
  afx_msg void OnKeyDown(UINT key, UINT repeat, UINT flags);
  DECLARE_MESSAGE_MAP()

 private:
  enum class State { Pending, Running, Ok, Warn };
  struct Step {
    CString text;    // 점검 중 문구
    CString result;  // 끝난 뒤 덧붙일 문구 (예: "없음")
    State state = State::Pending;
  };
  void RunStep(int index);
  int Px(int value96) const { return MulDiv(value96, m_dpi, 96); }

  std::vector<Step> m_steps;
  int m_current = 0;
  bool m_done = false;
  int m_dpi = 96;
  HICON m_icon = nullptr;
  CFont m_titleFont, m_textFont, m_smallFont, m_iconFont;
};
