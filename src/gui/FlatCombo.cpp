#include "pch.h"

#include "FlatCombo.h"

#include <algorithm>

namespace {
const wchar_t kGlyphChevronDown = L'';
}  // namespace

BEGIN_MESSAGE_MAP(CFlatCombo, CComboBox)
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_MOUSEMOVE()
  ON_WM_MOUSELEAVE()
  ON_WM_SETFOCUS()
  ON_WM_KILLFOCUS()
  ON_WM_ENABLE()
  ON_MESSAGE(CB_SETCURSEL, &CFlatCombo::OnSetCurSel)
  ON_CONTROL_REFLECT_EX(CBN_SELCHANGE, &CFlatCombo::OnSelChange)
  ON_CONTROL_REFLECT_EX(CBN_DROPDOWN, &CFlatCombo::OnDropDown)
  ON_CONTROL_REFLECT_EX(CBN_CLOSEUP, &CFlatCombo::OnCloseUp)
END_MESSAGE_MAP()

void CFlatCombo::SubclassCombos(CWnd* parent, COLORREF parentBg, std::vector<std::unique_ptr<CFlatCombo>>& owned) {
  for (CWnd* child = parent->GetWindow(GW_CHILD); child; child = child->GetWindow(GW_HWNDNEXT)) {
    wchar_t cls[16] = {};
    ::GetClassNameW(child->m_hWnd, cls, 16);
    if (_wcsicmp(cls, L"ComboBox") != 0) continue;
    if ((child->GetStyle() & 0x3) != CBS_DROPDOWNLIST) continue;  // 편집 가능한 콤보는 그대로
    if (CWnd::FromHandlePermanent(child->m_hWnd)) continue;
    auto combo = std::make_unique<CFlatCombo>(parentBg);
    if (!combo->SubclassWindow(child->m_hWnd)) continue;
    combo->Invalidate();
    child = combo.get();
    owned.push_back(std::move(combo));
  }
}

void CFlatCombo::OnPaint() {
  CPaintDC paintDc(this);
  CRect rc;
  GetClientRect(&rc);
  const int dpi = static_cast<int>(GetDpiForWindow(m_hWnd));
  auto px = [dpi](int v) { return MulDiv(v, dpi, 96); };
  if (m_iconDpi != static_cast<UINT>(dpi)) {
    theme::CreateIconFont(m_iconFont, 8, dpi);
    m_iconDpi = dpi;
  }

  CDC dc;
  dc.CreateCompatibleDC(&paintDc);
  CBitmap bmp;
  bmp.CreateCompatibleBitmap(&paintDc, rc.Width(), rc.Height());
  CBitmap* oldBmp = dc.SelectObject(&bmp);
  dc.SetBkMode(TRANSPARENT);

  const bool disabled = !IsWindowEnabled();
  const bool focused = GetFocus() == this;
  const COLORREF face = disabled ? theme::kCard : m_dropped ? theme::kSelected : m_hover ? theme::kHover : theme::kPanel;
  const COLORREF border = disabled ? theme::kBorder : (focused || m_dropped) ? theme::kAccent : theme::kControlBorder;

  dc.FillSolidRect(rc, m_parentBg);
  const int radius = std::max(6, rc.Height() / 4);
  theme::FillRoundRect(&dc, rc, radius, face, border);
  if (focused && !disabled) {  // 포커스는 아래쪽 강조선으로 (Windows 11 입력 상자와 같은 표시)
    CRect line(rc.left + radius / 2, rc.bottom - px(2), rc.right - radius / 2, rc.bottom - 1);
    dc.FillSolidRect(line, theme::kAccent);
  }

  // 아래 화살표
  CRect arrow(rc.right - px(28), rc.top, rc.right - px(6), rc.bottom);
  CFont* oldFont = dc.SelectObject(&m_iconFont);
  dc.SetTextColor(disabled ? theme::kTextDisabled : theme::kTextSecondary);
  const wchar_t glyph[2] = {kGlyphChevronDown, 0};
  dc.DrawText(glyph, arrow, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  // 선택 항목
  CString text;
  const int sel = GetCurSel();
  if (sel >= 0) GetLBText(sel, text);
  CFont* font = GetFont();
  if (font) dc.SelectObject(font);
  dc.SetTextColor(disabled ? theme::kTextDisabled : theme::kText);
  CRect textRc(rc.left + px(10), rc.top, arrow.left, rc.bottom);
  dc.DrawText(text, textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);

  dc.SelectObject(oldFont);
  paintDc.BitBlt(0, 0, rc.Width(), rc.Height(), &dc, 0, 0, SRCCOPY);
  dc.SelectObject(oldBmp);
}

void CFlatCombo::OnMouseMove(UINT flags, CPoint pt) {
  if (!m_hover) {
    m_hover = true;
    TRACKMOUSEEVENT tme{sizeof tme, TME_LEAVE, m_hWnd, 0};
    TrackMouseEvent(&tme);
    Invalidate(FALSE);
  }
  CComboBox::OnMouseMove(flags, pt);
}

void CFlatCombo::OnMouseLeave() {
  m_hover = false;
  Invalidate(FALSE);
  CComboBox::OnMouseLeave();
}

void CFlatCombo::OnSetFocus(CWnd* old) {
  CComboBox::OnSetFocus(old);
  Invalidate(FALSE);
}

void CFlatCombo::OnKillFocus(CWnd* next) {
  CComboBox::OnKillFocus(next);
  Invalidate(FALSE);
}

void CFlatCombo::OnEnable(BOOL enable) {
  CComboBox::OnEnable(enable);
  Invalidate(FALSE);
}

BOOL CFlatCombo::OnSelChange() {
  Invalidate(FALSE);
  return FALSE;
}

LRESULT CFlatCombo::OnSetCurSel(WPARAM, LPARAM) {
  const LRESULT r = Default();
  Invalidate(FALSE);
  return r;
}

BOOL CFlatCombo::OnDropDown() {
  m_dropped = true;
  Invalidate(FALSE);
  return FALSE;
}

BOOL CFlatCombo::OnCloseUp() {
  m_dropped = false;
  Invalidate(FALSE);
  return FALSE;
}
