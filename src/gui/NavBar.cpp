#include "pch.h"

#include "NavBar.h"

#include <algorithm>

#include "Theme.h"

namespace {
const wchar_t kGlyphMenu = L'\uE700';  // GlobalNavigationButton (☰)
}

BEGIN_MESSAGE_MAP(CNavBar, CWnd)
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_NCHITTEST()
  ON_WM_LBUTTONDOWN()
  ON_WM_MOUSEMOVE()
  ON_WM_MOUSELEAVE()
  ON_WM_GETDLGCODE()
  ON_WM_KEYDOWN()
  ON_WM_SETFOCUS()
  ON_WM_KILLFOCUS()
  ON_WM_SIZE()
  ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &CNavBar::OnDpiChangedAfterParent)
END_MESSAGE_MAP()

// ------------------------------------------------------------ 항목 구성

void CNavBar::AddPage(int page, wchar_t glyph, LPCTSTR text, bool separatorBefore) {
  EnsureFixedItems();
  Item item{Kind::Page, glyph, text};
  item.page = page;
  item.separatorBefore = separatorBefore;
  m_items.push_back(item);
  Layout();
}

void CNavBar::AddAction(wchar_t glyph, LPCTSTR text, UINT cmd) {
  EnsureFixedItems();
  Item item{Kind::Action, glyph, text};
  item.cmd = cmd;
  m_items.push_back(item);
  Layout();
}

void CNavBar::EnsureFixedItems() {
  // 맨 위 고정 항목: 접기 버튼(☰)과 앱 로고(누르면 홈 화면)
  if (!m_items.empty()) return;
  m_items.push_back(Item{Kind::Toggle, kGlyphMenu, L"탐색 창 접기/펼치기"});
  m_items.push_back(Item{Kind::Home, 0, L"홈으로 · 지문인식 알고리즘 평가"});
}

void CNavBar::SetAppIcon(HICON icon) {
  m_appIcon = icon;
  if (m_hWnd) Invalidate(FALSE);
}

void CNavBar::SetSelected(int page) {
  m_selected = page;
  for (size_t i = 0; i < m_items.size(); ++i) {
    if (m_items[i].kind == Kind::Page && m_items[i].page == page) m_focus = static_cast<int>(i);
  }
  if (m_hWnd) Invalidate(FALSE);
}

void CNavBar::EnableAction(UINT cmd, bool enable) {
  for (auto& it : m_items) {
    if (it.kind == Kind::Action && it.cmd == cmd && it.enabled != enable) {
      it.enabled = enable;
      if (m_hWnd) InvalidateRect(it.rect, FALSE);
    }
  }
}

void CNavBar::EnablePage(int page, bool enable) {
  for (auto& it : m_items) {
    if (it.kind == Kind::Page && it.page == page && it.enabled != enable) {
      it.enabled = enable;
      if (m_hWnd) InvalidateRect(it.rect, FALSE);
    }
  }
}

void CNavBar::SetBadge(int page, const CString& badge) {
  for (auto& it : m_items) {
    if (it.kind == Kind::Page && it.page == page && it.badge != badge) {
      it.badge = badge;
      if (m_hWnd) InvalidateRect(it.rect, FALSE);
    }
  }
}

void CNavBar::SetCompact(bool compact) {
  m_compact = compact;
  Layout();
  if (m_hWnd) Invalidate(FALSE);
}

// ------------------------------------------------------------ 배치

int CNavBar::Dpi() const { return m_hWnd ? static_cast<int>(GetDpiForWindow(m_hWnd)) : 96; }

void CNavBar::EnsureInit() {
  const UINT dpi = static_cast<UINT>(Dpi());
  if (m_fontDpi != dpi) {
    m_fontDpi = dpi;
    theme::CreateUiFont(m_textFont, 10, FW_NORMAL, dpi);
    theme::CreateUiFont(m_titleFont, 12, FW_BOLD, dpi);
    theme::CreateUiFont(m_smallFont, 8, FW_NORMAL, dpi);
    theme::CreateIconFont(m_iconFont, 11, dpi);
  }
  if (m_hWnd && !m_tip.GetSafeHwnd()) {
    m_tip.Create(this, TTS_ALWAYSTIP | TTS_NOPREFIX);
    UpdateTips();
  }
}

void CNavBar::Layout() {
  if (!m_hWnd) return;
  CRect client;
  GetClientRect(&client);
  const int left = Px(6), right = client.right - Px(6);
  const int firstPage = m_compact ? Px(100) : Px(64);
  const int footer = Px(10) + static_cast<int>(std::count_if(m_items.begin(), m_items.end(),
      [](const Item& it) { return it.kind == Kind::Action; })) * Px(36);
  const int pageCount = static_cast<int>(std::count_if(m_items.begin(), m_items.end(),
      [](const Item& it) { return it.kind == Kind::Page; }));
  const int separators = static_cast<int>(std::count_if(m_items.begin(), m_items.end(),
      [](const Item& it) { return it.kind == Kind::Page && it.separatorBefore; }));
  const int pageGap = Px(2), sectionGap = Px(14);
  const int itemH = pageCount == 0 ? Px(36) : std::clamp(
      (client.Height() - firstPage - footer - separators * sectionGap - Px(6)) / pageCount - pageGap,
      Px(24), Px(36));
  for (auto& it : m_items) {
    if (it.kind == Kind::Toggle) it.rect.SetRect(left, Px(10), left + Px(40), Px(10) + itemH);
    // 로고: 펼친 상태는 접기 버튼 오른쪽 머리글 전체, 접힌 상태는 접기 버튼 아래 아이콘 한 칸
    if (it.kind == Kind::Home) {
      if (m_compact) {
        it.rect.SetRect(left, Px(52), left + Px(40), Px(52) + itemH);
      } else {
        it.rect.SetRect(left + Px(42), Px(8), right, Px(56));
      }
    }
  }
  // 페이지 항목: 머리글 아래부터 위→아래
  int y = firstPage;
  for (auto& it : m_items) {
    if (it.kind != Kind::Page) continue;
    if (it.separatorBefore) y += sectionGap;
    it.rect.SetRect(left, y, right, y + itemH);
    y += itemH + Px(2);
  }
  // 동작 항목: 맨 아래부터 위로
  int bottom = client.bottom - Px(10);
  for (auto it = m_items.rbegin(); it != m_items.rend(); ++it) {
    if (it->kind != Kind::Action) continue;
    it->rect.SetRect(left, bottom - Px(34), right, bottom);
    bottom -= Px(34) + Px(2);
  }
  UpdateTips();
}

void CNavBar::UpdateTips() {
  if (!m_tip.GetSafeHwnd()) return;
  // 항목마다 영역 하나씩. 접힌 상태에서만 이름을 보여 준다 (펼친 상태는 글자가 보이므로 접기 버튼만)
  for (size_t i = 0; i < m_items.size(); ++i) m_tip.DelTool(this, static_cast<UINT_PTR>(i + 1));  // 도구 ID = 항목 번호 + 1
  for (size_t i = 0; i < m_items.size(); ++i) {
    const Item& it = m_items[i];
    if (!m_compact && it.kind != Kind::Toggle && it.kind != Kind::Home) continue;
    CString tip = it.kind == Kind::Toggle ? CString(m_compact ? L"탐색 창 펼치기" : L"탐색 창 접기") : it.text;
    m_tip.AddTool(this, tip, it.rect, static_cast<UINT_PTR>(i + 1));
  }
}

BOOL CNavBar::PreTranslateMessage(MSG* msg) {
  if (m_tip.GetSafeHwnd()) m_tip.RelayEvent(msg);
  return CWnd::PreTranslateMessage(msg);
}

void CNavBar::OnSize(UINT type, int cx, int cy) {
  CWnd::OnSize(type, cx, cy);
  Layout();
  Invalidate(FALSE);
}

LRESULT CNavBar::OnDpiChangedAfterParent(WPARAM, LPARAM) {
  EnsureInit();
  Layout();
  Invalidate(FALSE);
  return 0;
}

int CNavBar::HitTest(CPoint pt) const {
  for (size_t i = 0; i < m_items.size(); ++i) {
    if (m_items[i].rect.PtInRect(pt)) return static_cast<int>(i);
  }
  return -1;
}

// ------------------------------------------------------------ 그리기

void CNavBar::OnPaint() {
  CPaintDC paintDc(this);
  EnsureInit();
  CRect client;
  GetClientRect(&client);
  // 깜박임 없게 메모리 DC에 그린 뒤 한 번에 복사
  CDC dc;
  dc.CreateCompatibleDC(&paintDc);
  CBitmap bmp;
  bmp.CreateCompatibleBitmap(&paintDc, client.Width(), client.Height());
  CBitmap* oldBmp = dc.SelectObject(&bmp);
  dc.FillSolidRect(client, theme::kWindowBg);
  dc.SetBkMode(TRANSPARENT);
  CFont* oldFont = dc.SelectObject(&m_titleFont);

  for (size_t i = 0; i < m_items.size(); ++i) {
    const Item& it = m_items[i];
    const bool selected = it.kind == Kind::Page && it.page == m_selected;
    const bool hot = static_cast<int>(i) == m_hot && it.enabled;
    if (it.separatorBefore) {
      const int sy = it.rect.top - Px(7);
      dc.FillSolidRect(it.rect.left + Px(8), sy, it.rect.Width() - Px(16), 1, theme::kBorder);
    }
    if (selected || hot) {
      const COLORREF fill = selected ? theme::kSelected : theme::kHover;
      theme::FillRoundRect(&dc, it.rect, Px(8), fill, fill);
    }
    if (selected) {  // 선택 표시: 왼쪽 강조색 막대
      const int barH = Px(16);
      CRect bar(it.rect.left, it.rect.CenterPoint().y - barH / 2, it.rect.left + Px(3), it.rect.CenterPoint().y + barH / 2);
      theme::FillRoundRect(&dc, bar, Px(3), theme::kAccent, theme::kAccent);
    }
    if (it.kind == Kind::Home) {
      const int iconSize = m_compact ? Px(24) : Px(28);
      const int iy = it.rect.CenterPoint().y - iconSize / 2;
      const int ix = m_compact ? it.rect.CenterPoint().x - iconSize / 2 : it.rect.left + Px(4);
      if (m_appIcon) DrawIconEx(dc.m_hDC, ix, iy, m_appIcon, iconSize, iconSize, 0, nullptr, DI_NORMAL);
      if (!m_compact) {
        dc.SelectObject(&m_titleFont);
        dc.SetTextColor(theme::kText);
        CRect label(ix + Px(36), it.rect.top, it.rect.right - Px(4), it.rect.bottom);
        dc.DrawText(L"FPEval", label, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
      }
    } else {
      const COLORREF fg = !it.enabled ? theme::kTextDisabled : theme::kText;
      CRect iconRc(it.rect.left + Px(8), it.rect.top, it.rect.left + Px(32), it.rect.bottom);
      dc.SelectObject(&m_iconFont);
      dc.SetTextColor(selected ? theme::kAccent : fg);
      const wchar_t glyph[2] = {it.glyph, 0};
      dc.DrawText(glyph, iconRc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      if (m_compact) {
        if (!it.badge.IsEmpty()) {  // 접힌 상태: 진행 중 표시는 작은 점
          const int d = Px(6);
          CRect dot(iconRc.right - d / 2, it.rect.top + Px(7), iconRc.right + d / 2, it.rect.top + Px(7) + d);
          theme::FillRoundRect(&dc, dot, d, theme::kAccent, theme::kAccent);
        }
      } else if (it.kind != Kind::Toggle) {
        CRect textRc(it.rect.left + Px(42), it.rect.top, it.rect.right - Px(8), it.rect.bottom);
        if (!it.badge.IsEmpty()) {
          dc.SelectObject(&m_smallFont);
          dc.SetTextColor(theme::kAccent);
          dc.DrawText(it.badge, textRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        }
        dc.SelectObject(&m_textFont);
        dc.SetTextColor(fg);
        dc.DrawText(it.text, textRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
      }
    }
    if (m_hasFocus && static_cast<int>(i) == m_focus) {
      CRect focus(it.rect);
      focus.DeflateRect(Px(2), Px(2));
      dc.SetTextColor(theme::kText);
      dc.DrawFocusRect(focus);
    }
  }
  dc.SelectObject(oldFont);
  paintDc.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
  dc.SelectObject(oldBmp);
}

// ------------------------------------------------------------ 마우스·키보드

void CNavBar::Activate(int index) {
  if (index < 0 || index >= static_cast<int>(m_items.size())) return;
  const Item& it = m_items[index];
  if (!it.enabled) return;
  m_focus = index;
  switch (it.kind) {
    case Kind::Toggle:
      Invalidate(FALSE);
      GetParent()->PostMessage(WM_APP_NAV_TOGGLE);
      break;
    case Kind::Home:
      Invalidate(FALSE);
      GetParent()->PostMessage(WM_APP_NAV_HOME);
      break;
    case Kind::Page:
      SetSelected(it.page);
      GetParent()->SendMessage(WM_APP_NAV, static_cast<WPARAM>(it.page));
      break;
    case Kind::Action:
      Invalidate(FALSE);
      GetParent()->PostMessage(WM_COMMAND, MAKEWPARAM(it.cmd, 0), 0);
      break;
  }
}

void CNavBar::OnLButtonDown(UINT flags, CPoint pt) {
  SetFocus();
  Activate(HitTest(pt));
  CWnd::OnLButtonDown(flags, pt);
}

void CNavBar::OnMouseMove(UINT flags, CPoint pt) {
  if (!m_tracking) {
    TRACKMOUSEEVENT tme{sizeof tme, TME_LEAVE, m_hWnd, 0};
    TrackMouseEvent(&tme);
    m_tracking = true;
  }
  const int hot = HitTest(pt);
  if (hot != m_hot) {
    m_hot = hot;
    Invalidate(FALSE);
  }
  CWnd::OnMouseMove(flags, pt);
}

void CNavBar::OnMouseLeave() {
  m_tracking = false;
  m_hot = -1;
  Invalidate(FALSE);
  CWnd::OnMouseLeave();
}

UINT CNavBar::OnGetDlgCode() {
  // 화살표와 Enter는 대화상자가 가로채지 않고 이 컨트롤로 오게 한다 (Tab은 대화상자가 처리)
  UINT code = DLGC_WANTARROWS | DLGC_WANTCHARS;
  const MSG* msg = reinterpret_cast<const MSG*>(GetCurrentMessage()->lParam);
  if (msg && msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN) code |= DLGC_WANTMESSAGE;
  return code;
}

void CNavBar::MoveFocus(int step) {
  const int n = static_cast<int>(m_items.size());
  for (int k = 1; k <= n; ++k) {
    const int i = ((m_focus + step * k) % n + n) % n;
    if (!m_items[i].enabled) continue;
    m_focus = i;
    if (m_items[i].kind == Kind::Page) {
      Activate(i);  // 페이지는 화살표로 옮기면 바로 보여 준다
    } else {
      Invalidate(FALSE);
    }
    return;
  }
}

void CNavBar::OnKeyDown(UINT key, UINT repeat, UINT flags) {
  switch (key) {
    case VK_UP:
      MoveFocus(-1);
      return;
    case VK_DOWN:
      MoveFocus(1);
      return;
    case VK_RETURN:
    case VK_SPACE:
      Activate(m_focus);
      return;
    default:
      CWnd::OnKeyDown(key, repeat, flags);
  }
}

void CNavBar::OnSetFocus(CWnd* old) {
  CWnd::OnSetFocus(old);
  m_hasFocus = true;
  Invalidate(FALSE);
}

void CNavBar::OnKillFocus(CWnd* next) {
  CWnd::OnKillFocus(next);
  m_hasFocus = false;
  Invalidate(FALSE);
}
