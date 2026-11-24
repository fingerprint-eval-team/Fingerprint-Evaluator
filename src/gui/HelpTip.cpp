#include "pch.h"

#include "HelpTip.h"

#include <algorithm>

#include "Theme.h"

namespace {
constexpr UINT_PTR kHelpTimer = 1;
constexpr UINT kTextFlags = DT_LEFT | DT_WORDBREAK | DT_NOPREFIX | DT_EDITCONTROL;
}

BEGIN_MESSAGE_MAP(CHelpPopup, CWnd)
  ON_WM_TIMER()
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_NCHITTEST()
  ON_WM_MOUSEACTIVATE()
END_MESSAGE_MAP()

CHelpPopup::~CHelpPopup() {
  if (GetSafeHwnd()) DestroyWindow();
}

void CHelpPopup::Track(CWnd& anchor, const CRect& bounds, const CString& description, bool immediate) {
  const HWND handle = anchor.GetSafeHwnd();
  if (!handle || bounds.IsRectEmpty() || description.IsEmpty()) return;
  if (!immediate && m_anchor == handle && m_bounds == bounds && m_description == description) return;

  const HWND owner = ::GetAncestor(handle, GA_ROOT);
  if (!owner) return;
  if (!GetSafeHwnd()) {
    const auto windowClass = AfxRegisterWndClass(CS_DROPSHADOW);
    if (!CreateEx(WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE, windowClass, L"", WS_POPUP, 0, 0, 0, 0, owner, nullptr))
      return;
  }
  Hide();
  m_anchor = handle;
  m_owner = owner;
  m_bounds = bounds;
  m_description = description;
  SetWindowText(m_description);
  m_allowFocus = immediate;
  m_showAfter = GetTickCount64() + (immediate ? 0 : 320);
  if (!SetTimer(kHelpTimer, 80, nullptr)) {
    Hide();
    return;
  }
  if (immediate) OnTimer(kHelpTimer);
}

void CHelpPopup::Hide() {
  if (GetSafeHwnd()) {
    KillTimer(kHelpTimer);
    if (IsWindowVisible()) ShowWindow(SW_HIDE);
  }
  m_anchor = nullptr;
  m_previousAnchor.SetRectEmpty();
}

void CHelpPopup::OnTimer(UINT_PTR timer) {
  if (timer != kHelpTimer) return;
  if (!::IsWindowVisible(m_anchor) || ::GetForegroundWindow() != m_owner || ::IsIconic(m_owner)) {
    Hide();
    return;
  }
  CRect screen(m_bounds);
  ::MapWindowPoints(m_anchor, nullptr, reinterpret_cast<POINT*>(&screen), 2);
  CPoint pointer;
  const bool hovered = ::GetCursorPos(&pointer) && screen.PtInRect(pointer);
  if (!hovered && !(m_allowFocus && ::GetFocus() == m_anchor)) {
    Hide();
    return;
  }
  if (GetTickCount64() >= m_showAfter && screen != m_previousAnchor) ShowAt(screen);
}

void CHelpPopup::ShowAt(const CRect& anchor) {
  m_dpi = ::GetDpiForWindow(m_anchor);
  const auto px = [this](int n) { return MulDiv(n, static_cast<int>(m_dpi), 96); };
  MONITORINFO monitor{sizeof monitor};
  if (!GetMonitorInfoW(MonitorFromRect(anchor, MONITOR_DEFAULTTONEAREST), &monitor)) {
    Hide();
    return;
  }
  const CRect work(monitor.rcWork);
  const int leftInset = px(43), rightInset = px(16), verticalInset = px(13);
  const int maxText = std::max(1, std::min(px(290), work.Width() - leftInset - rightInset));
  theme::CreateUiFont(m_font, 10, FW_NORMAL, m_dpi);
  CClientDC dc(this);
  CFont* previous = dc.SelectObject(&m_font);
  CRect measured(0, 0, maxText, 0);
  dc.DrawText(m_description, measured, kTextFlags | DT_CALCRECT);
  dc.SelectObject(previous);
  const int width = std::min(work.Width(), std::clamp(measured.Width(), std::min(maxText, px(130)), maxText) +
                                         leftInset + rightInset);
  const int height = std::min(work.Height(), measured.Height() + 2 * verticalInset);
  const int x = std::clamp(static_cast<int>(anchor.left) - px(10), static_cast<int>(work.left),
                           static_cast<int>(work.right) - width);
  int y = anchor.bottom + px(7);
  if (y + height > work.bottom) y = anchor.top - height - px(7);
  y = std::clamp(y, static_cast<int>(work.top), static_cast<int>(work.bottom) - height);

  CRgn shape;
  if (shape.CreateRoundRectRgn(0, 0, width, height, px(16), px(16)) && SetWindowRgn(shape, FALSE))
    shape.Detach();  // 성공하면 Windows가 영역의 소유권을 갖는다.
  SetWindowPos(&wndTop, x, y, width, height, SWP_NOACTIVATE | SWP_SHOWWINDOW);
  m_previousAnchor = anchor;
  Invalidate(FALSE);
}

void CHelpPopup::OnPaint() {
  CPaintDC dc(this);
  const auto px = [this](int n) { return MulDiv(n, static_cast<int>(m_dpi), 96); };
  CRect client;
  GetClientRect(&client);
  theme::FillRoundRect(&dc, client, px(16), theme::kCard, theme::kControlBorder);
  CRect icon(px(13), px(12), px(31), px(30));
  theme::DrawHelpBadge(&dc, icon, false, m_dpi);
  CRect text(px(43), px(13), client.right - px(16), client.bottom - px(12));
  CFont* previous = dc.SelectObject(&m_font);
  dc.SetBkMode(TRANSPARENT);
  dc.SetTextColor(theme::kText);
  dc.DrawText(m_description, text, kTextFlags);
  dc.SelectObject(previous);
}
