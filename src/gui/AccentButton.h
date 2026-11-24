#pragma once
#include "pch.h"

#include <memory>
#include <vector>

#include "Theme.h"

// 둥근 사각형으로 직접 그리는 버튼 (BS_OWNERDRAW). ID·클릭 알림(BN_CLICKED)·키보드 동작은 일반 버튼과 같다.
// - Accent: 주요 동작(평가 시작). 강조색 바탕 + 흰 글자. 리소스에서 BS_OWNERDRAW로 두고 DDX_Control로 서브클래싱한다.
// - Secondary: 나머지 버튼(중지·찾아보기 등). 흰 바탕 + 회색 테두리. SubclassPushButtons로 한꺼번에 바꾼다.
class CAccentButton : public CButton {
 public:
  enum class Style { Accent, Secondary };
  explicit CAccentButton(Style style = Style::Accent, COLORREF parentBg = theme::kWindowBg)
      : m_style(style), m_parentBg(parentBg) {}
  void DrawItem(LPDRAWITEMSTRUCT dis) override;

  // parent의 직속 자식 중 일반 누름 버튼(BS_PUSHBUTTON/BS_DEFPUSHBUTTON)을 모두 Secondary 버튼으로 바꾼다.
  // parentBg는 둥근 모서리 바깥을 칠할 부모 배경색. 만든 객체는 owned가 가진다 (부모 대화상자의 멤버로 둔다).
  static void SubclassPushButtons(CWnd* parent, COLORREF parentBg, std::vector<std::unique_ptr<CAccentButton>>& owned);

 protected:
  afx_msg void OnMouseMove(UINT flags, CPoint pt);
  afx_msg void OnMouseLeave();
  DECLARE_MESSAGE_MAP()

 private:
  Style m_style;
  COLORREF m_parentBg;
  bool m_hover = false;
};
