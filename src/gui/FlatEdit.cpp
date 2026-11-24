#include "pch.h"

#include "FlatEdit.h"

#include <algorithm>

BEGIN_MESSAGE_MAP(CFlatEdit, CEdit)
  ON_WM_NCCALCSIZE()
  ON_WM_NCPAINT()
  ON_WM_SETFOCUS()
  ON_WM_KILLFOCUS()
  ON_WM_ENABLE()
  ON_MESSAGE(WM_SETFONT, &CFlatEdit::OnSetFont)
END_MESSAGE_MAP()

void CFlatEdit::SubclassEdits(CWnd* parent, COLORREF parentBg, std::vector<std::unique_ptr<CFlatEdit>>& owned) {
  for (CWnd* child = parent->GetWindow(GW_CHILD); child; child = child->GetWindow(GW_HWNDNEXT)) {
    wchar_t cls[16] = {};
    ::GetClassNameW(child->m_hWnd, cls, 16);
    if (_wcsicmp(cls, L"Edit") != 0) continue;
    if (child->GetStyle() & ES_MULTILINE) continue;  // 여러 줄 입력 상자는 그대로
    if (CWnd::FromHandlePermanent(child->m_hWnd)) continue;
    auto edit = std::make_unique<CFlatEdit>(parentBg);
    if (!edit->SubclassWindow(child->m_hWnd)) continue;
    // 기본 3D 테두리를 빼고, 비클라이언트 영역을 다시 계산하게 한다 (OnNcCalcSize)
    edit->ModifyStyleEx(WS_EX_CLIENTEDGE, 0);
    edit->SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
    child = edit.get();
    owned.push_back(std::move(edit));
  }
}

int CFlatEdit::Px(int v) const { return MulDiv(v, static_cast<int>(GetDpiForWindow(m_hWnd)), 96); }

void CFlatEdit::RedrawFrame() { RedrawWindow(nullptr, nullptr, RDW_FRAME | RDW_INVALIDATE | RDW_NOCHILDREN); }

void CFlatEdit::OnNcCalcSize(BOOL, NCCALCSIZE_PARAMS* params) {
  // 기본 처리(WS_BORDER 1px)를 쓰지 않고 직접 여백을 정한다: 좌우 Px(8), 위아래는 글자 높이에 맞춰 가운데
  RECT& rc = params->rgrc[0];
  int textH = Px(16);
  if (CFont* font = GetFont()) {
    CClientDC dc(this);
    CFont* old = dc.SelectObject(font);
    TEXTMETRIC tm{};
    dc.GetTextMetrics(&tm);
    dc.SelectObject(old);
    textH = tm.tmHeight;
  }
  const int w = static_cast<int>(rc.right - rc.left);
  const int h = static_cast<int>(rc.bottom - rc.top);
  const int padY = std::max(1, (h - textH) / 2);
  const int padX = std::min(Px(8), std::max(1, w / 4));
  rc.left += padX;
  rc.right -= padX;
  rc.top += padY;
  rc.bottom = std::max(rc.top, rc.bottom - padY);
}

void CFlatEdit::OnNcPaint() {
  CWindowDC dc(this);
  CRect window;
  GetWindowRect(&window);
  CRect client;
  GetClientRect(&client);
  ClientToScreen(&client);
  client.OffsetRect(-window.TopLeft());
  window.OffsetRect(-window.TopLeft());

  const bool disabled = !IsWindowEnabled() || (GetStyle() & ES_READONLY);
  const bool focused = GetFocus() == this;
  const COLORREF face = disabled ? theme::kCard : theme::kPanel;
  const COLORREF border = disabled ? theme::kBorder : focused ? theme::kAccent : theme::kControlBorder;

  // 클라이언트 영역(글자 부분)은 기본 처리가 그리므로 제외하고 바깥 테두리 영역만 칠한다
  dc.ExcludeClipRect(client);
  dc.FillSolidRect(window, m_parentBg);
  const int radius = std::max(6, window.Height() / 4);
  theme::FillRoundRect(&dc, window, radius, face, border);
  if (focused && !disabled) {  // 아래쪽 강조선
    CRect line(window.left + radius / 2, window.bottom - Px(2), window.right - radius / 2, window.bottom - 1);
    dc.FillSolidRect(line, theme::kAccent);
  }
}

void CFlatEdit::OnSetFocus(CWnd* old) {
  CEdit::OnSetFocus(old);
  RedrawFrame();
}

void CFlatEdit::OnKillFocus(CWnd* next) {
  CEdit::OnKillFocus(next);
  RedrawFrame();
}

void CFlatEdit::OnEnable(BOOL enable) {
  CEdit::OnEnable(enable);
  RedrawFrame();
}

LRESULT CFlatEdit::OnSetFont(WPARAM, LPARAM) {
  const LRESULT r = Default();
  SetWindowPos(nullptr, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
  return r;
}
