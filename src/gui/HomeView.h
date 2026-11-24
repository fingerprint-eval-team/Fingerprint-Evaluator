#pragma once
#include "pch.h"

#include <vector>

#include "HelpTip.h"

// 홈 화면: 메인 창 클라이언트 전체를 덮는 자식 창. 왼쪽은 강조색 브랜드 패널(시작 화면과 같은 모양),
// 오른쪽은 큰 메뉴 목록(새 테스트·이어서 하기·설정 불러오기·결과 보기)과 아래쪽 정보 링크.
// 항목을 누르면 부모에 WM_APP_HOME_CMD(wParam = Cmd)를 보낸다. 실제 동작은 부모(CMainDlg)가 한다.
// 평가가 진행 중이면 맨 위에 "평가 진행 중" 항목이 생기고(누르면 실행 현황), 새 실행을 시작하는 항목은 잠긴다.
// 키보드: 위/아래 화살표로 이동, Enter/Space로 실행.
class CHomeView : public CWnd {
 public:
  enum Cmd { kCmdRunning, kCmdNewTest, kCmdResume, kCmdLoadConfig, kCmdResults, kCmdAbout };

  ~CHomeView() override;
  BOOL Create(CWnd* parent, UINT id);
  void SetRunning(bool running, const CString& progress);  // progress: "42%" 같은 진행 표시

 protected:
  BOOL PreTranslateMessage(MSG* msg) override;
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg void OnLButtonDown(UINT flags, CPoint pt);
  afx_msg void OnMouseMove(UINT flags, CPoint pt);
  afx_msg void OnMouseLeave();
  afx_msg UINT OnGetDlgCode();
  afx_msg void OnKeyDown(UINT key, UINT repeat, UINT flags);
  afx_msg void OnSetFocus(CWnd* old);
  afx_msg void OnKillFocus(CWnd* next);
  afx_msg void OnSize(UINT type, int cx, int cy);
  afx_msg LRESULT OnDpiChangedAfterParent(WPARAM, LPARAM);
  DECLARE_MESSAGE_MAP()

 private:
  struct Item {
    Cmd cmd;
    wchar_t glyph;
    CString title, desc;
    bool enabled = true;
    bool visible = true;
    CRect rect, help;
  };
  void EnsureResources();
  void Layout();
  int HitTest(CPoint pt) const;
  void Activate(int index);
  void MoveFocus(int step);
  int Px(int value96) const { return MulDiv(value96, m_dpi, 96); }

  std::vector<Item> m_items;
  int m_hot = -1;
  int m_focus = 1;  // 처음에는 "새 테스트"
  bool m_tracking = false;
  bool m_hasFocus = false;
  int m_dpi = 0;
  HICON m_icon = nullptr;
  CFont m_brandFont, m_headFont, m_titleFont, m_textFont, m_smallFont, m_iconFont, m_chevronFont;
  CHelpPopup m_helpPopup;
  int m_helpHot = -1;
};
