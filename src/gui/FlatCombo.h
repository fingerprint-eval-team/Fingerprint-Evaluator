#pragma once
#include "pch.h"

#include <memory>
#include <vector>

#include "Theme.h"

// 닫힌 상태를 직접 그리는 드롭다운 목록 콤보 상자 (CBS_DROPDOWNLIST 전용).
// 흰 바탕 + 회색 테두리 둥근 사각형에 선택 항목과 아래 화살표를 그려 CAccentButton의 Secondary 버튼과 짝을 맞춘다.
// 펼친 목록·키보드·CBN_* 알림은 기본 콤보 상자 그대로다.
class CFlatCombo : public CComboBox {
 public:
  explicit CFlatCombo(COLORREF parentBg = theme::kPanel) : m_parentBg(parentBg) {}

  // parent의 직속 자식 중 CBS_DROPDOWNLIST 콤보 상자를 모두 CFlatCombo로 바꾼다. 만든 객체는 owned가 가진다.
  static void SubclassCombos(CWnd* parent, COLORREF parentBg, std::vector<std::unique_ptr<CFlatCombo>>& owned);

 protected:
  afx_msg void OnPaint();
  afx_msg BOOL OnEraseBkgnd(CDC*) { return TRUE; }
  afx_msg void OnMouseMove(UINT flags, CPoint pt);
  afx_msg void OnMouseLeave();
  afx_msg void OnSetFocus(CWnd* old);
  afx_msg void OnKillFocus(CWnd* next);
  afx_msg void OnEnable(BOOL enable);
  // CBN_* 반사: 다시 그린 뒤 FALSE를 돌려 부모(페이지)도 같은 알림을 받게 한다
  afx_msg BOOL OnSelChange();
  afx_msg LRESULT OnSetCurSel(WPARAM, LPARAM);  // 코드에서 바꾼 선택(DDX)도 다시 그린다
  afx_msg BOOL OnDropDown();
  afx_msg BOOL OnCloseUp();
  DECLARE_MESSAGE_MAP()

 private:
  COLORREF m_parentBg;
  bool m_hover = false;
  bool m_dropped = false;
  UINT m_iconDpi = 0;
  CFont m_iconFont;
};
