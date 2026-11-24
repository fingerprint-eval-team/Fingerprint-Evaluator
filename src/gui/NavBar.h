#pragma once
#include "pch.h"

#include <vector>

// 왼쪽 사이드바: 접기·홈, 페이지 항목, 설정 저장·정보.
// 리소스의 Static 컨트롤(SS_NOTIFY | WS_TABSTOP)을 DDX_Control로 서브클래싱해서 쓴다.
// - 페이지 항목을 고르면 부모에 WM_APP_NAV(wParam = 페이지 번호)를 보낸다.
// - 동작 항목을 누르면 부모에 WM_COMMAND(명령 ID)를 보낸다. 버튼과 같은 처리기를 탄다.
// - 접기 버튼을 누르면 부모에 WM_APP_NAV_TOGGLE을 보낸다. 부모가 폭을 바꾸고 SetCompact를 부른다.
// - 앱 로고를 누르면 부모에 WM_APP_NAV_HOME을 보낸다.
// 접힌 상태(compact)에서는 아이콘만 그리고, 항목 이름은 도구 설명(툴팁)으로 보여 준다.
// 키보드: 위/아래 화살표로 이동(페이지는 바로 선택), Enter/Space로 실행.
class CNavBar : public CWnd {
 public:
  void SetAppIcon(HICON icon);
  void AddPage(int page, wchar_t glyph, LPCTSTR text, bool separatorBefore = false);
  void AddAction(wchar_t glyph, LPCTSTR text, UINT cmd);
  void SetSelected(int page);
  int Selected() const { return m_selected; }
  void EnableAction(UINT cmd, bool enable);
  void EnablePage(int page, bool enable);
  void SetBadge(int page, const CString& badge);  // 항목 오른쪽 작은 글자 (예: 실행 진행률)
  void SetCompact(bool compact);
  bool IsCompact() const { return m_compact; }

 protected:
  BOOL PreTranslateMessage(MSG* msg) override;
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg LRESULT OnNcHitTest(CPoint) { return HTCLIENT; }
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
  enum class Kind { Toggle, Home, Page, Action };
  struct Item {
    Kind kind;
    wchar_t glyph;
    CString text;
    UINT cmd = 0;   // 동작 항목의 명령 ID
    int page = -1;  // 페이지 항목의 번호
    bool separatorBefore = false;
    bool enabled = true;
    CString badge;
    CRect rect;
  };
  void EnsureFixedItems();  // 맨 위 접기 버튼·로고 항목
  void EnsureInit();        // 글꼴·툴팁 준비
  void Layout();
  void UpdateTips();
  int HitTest(CPoint pt) const;
  void Activate(int index);
  void MoveFocus(int step);
  int Dpi() const;
  int Px(int value96) const { return MulDiv(value96, Dpi(), 96); }

  std::vector<Item> m_items;
  int m_selected = 0;  // 선택된 페이지 번호
  int m_hot = -1;      // 마우스가 올라간 항목
  int m_focus = 2;     // 키보드 포커스 항목 (0은 접기 버튼, 1은 로고)
  bool m_tracking = false;
  bool m_hasFocus = false;
  bool m_compact = false;
  HICON m_appIcon = nullptr;
  UINT m_fontDpi = 0;
  CFont m_textFont, m_iconFont, m_titleFont, m_smallFont;
  CToolTipCtrl m_tip;
};
