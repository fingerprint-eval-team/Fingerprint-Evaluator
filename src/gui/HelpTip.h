#pragma once

#include "pch.h"

// 마우스와 키보드 포커스를 가져가지 않는 도움말 카드.
class CHelpPopup : public CWnd {
 public:
  ~CHelpPopup() override;
  void Track(CWnd& anchor, const CRect& bounds, const CString& description, bool immediate = false);
  void Hide();

 private:
  void ShowAt(const CRect& anchor);
  afx_msg void OnTimer(UINT_PTR timer);
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg LRESULT OnNcHitTest(CPoint) { return HTTRANSPARENT; }
  afx_msg int OnMouseActivate(CWnd*, UINT, UINT) { return MA_NOACTIVATE; }
  DECLARE_MESSAGE_MAP()

  HWND m_anchor = nullptr, m_owner = nullptr;
  CRect m_bounds{0, 0, 0, 0}, m_previousAnchor{0, 0, 0, 0};
  CString m_description;
  CFont m_font;
  UINT m_dpi = 96;
  ULONGLONG m_showAfter = 0;
  bool m_allowFocus = false;
};
