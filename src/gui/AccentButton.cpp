#include "pch.h"

#include "AccentButton.h"

#include <algorithm>

#include "Theme.h"

namespace {
const COLORREF kAccentDisabledFace = RGB(222, 224, 232);  // 중립 회색 (꺼진 핵심 버튼은 보라를 쓰지 않는다)
}  // namespace

BEGIN_MESSAGE_MAP(CAccentButton, CButton)
  ON_WM_MOUSEMOVE()
  ON_WM_MOUSELEAVE()
END_MESSAGE_MAP()

void CAccentButton::SubclassPushButtons(CWnd* parent, COLORREF parentBg,
                                        std::vector<std::unique_ptr<CAccentButton>>& owned) {
  for (CWnd* child = parent->GetWindow(GW_CHILD); child; child = child->GetWindow(GW_HWNDNEXT)) {
    wchar_t cls[16] = {};
    ::GetClassNameW(child->m_hWnd, cls, 16);
    if (_wcsicmp(cls, L"Button") != 0) continue;
    const DWORD type = child->GetStyle() & BS_TYPEMASK;
    if (type != BS_PUSHBUTTON && type != BS_DEFPUSHBUTTON) continue;  // 체크박스·라디오·그룹 상자·이미 직접 그리는 버튼은 그대로
    if (CWnd::FromHandlePermanent(child->m_hWnd)) continue;           // 이미 다른 MFC 객체에 연결됨
    auto button = std::make_unique<CAccentButton>(Style::Secondary, parentBg);
    if (!button->SubclassWindow(child->m_hWnd)) continue;
    button->ModifyStyle(BS_TYPEMASK, BS_OWNERDRAW);
    button->Invalidate();
    child = button.get();
    owned.push_back(std::move(button));
  }
}

void CAccentButton::OnMouseMove(UINT flags, CPoint pt) {
  if (!m_hover) {
    m_hover = true;
    TRACKMOUSEEVENT tme{sizeof tme, TME_LEAVE, m_hWnd, 0};
    TrackMouseEvent(&tme);
    Invalidate(FALSE);
  }
  CButton::OnMouseMove(flags, pt);
}

void CAccentButton::OnMouseLeave() {
  m_hover = false;
  Invalidate(FALSE);
  CButton::OnMouseLeave();
}

void CAccentButton::DrawItem(LPDRAWITEMSTRUCT dis) {
  CDC* dc = CDC::FromHandle(dis->hDC);
  CRect rc(dis->rcItem);
  const bool disabled = (dis->itemState & ODS_DISABLED) != 0;
  const bool pressed = (dis->itemState & ODS_SELECTED) != 0;
  const bool accent = m_style == Style::Accent;

  COLORREF face, border, text;
  if (accent) {
    face = disabled ? kAccentDisabledFace : pressed ? theme::kAccentPressed : m_hover ? theme::kAccentHover : theme::kAccent;
    border = face;
    text = disabled ? theme::kTextSecondary : theme::kOnAccent;
  } else {
    face = disabled ? theme::kCard : pressed ? theme::kPressed : m_hover ? theme::kHover : theme::kPanel;
    border = disabled ? theme::kBorder : theme::kControlBorder;
    text = disabled ? theme::kTextDisabled : theme::kText;
  }

  // 모서리 바깥은 부모 배경색으로 채운 뒤 둥근 사각형을 그린다
  dc->FillSolidRect(rc, m_parentBg);
  const int radius = std::max(6, rc.Height() / 4);
  theme::FillRoundRect(dc, rc, radius, face, border);

  CString label;
  GetWindowText(label);
  CFont* font = GetFont();
  CFont* oldFont = font ? dc->SelectObject(font) : nullptr;
  dc->SetBkMode(TRANSPARENT);
  dc->SetTextColor(text);
  CRect textRc(rc);
  if (pressed && !accent) textRc.OffsetRect(0, 1);  // 눌렀을 때 글자가 살짝 내려간다
  dc->DrawText(label, textRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
  if (oldFont) dc->SelectObject(oldFont);

  // 키보드 포커스 표시 (Tab 이동 시)
  if ((dis->itemState & ODS_FOCUS) && !(dis->itemState & ODS_NOFOCUSRECT)) {
    CRect focus(rc);
    focus.DeflateRect(3, 3);
    dc->SetTextColor(accent ? theme::kOnAccent : theme::kText);
    dc->DrawFocusRect(focus);
  }
}
