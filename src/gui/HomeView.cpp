#include "pch.h"

#include "HomeView.h"

#include <algorithm>

#include "Theme.h"

namespace {
const wchar_t kGlyphPlay = L'';        // Play
const wchar_t kGlyphRefresh = L'';     // Refresh (이어서 하기)
const wchar_t kGlyphOpenFile = L'';    // OpenFile
const wchar_t kGlyphReport = L'';      // ReportDocument
const wchar_t kGlyphDiagnostic = L'';  // Diagnostic (실행 현황과 같은 글리프)
const wchar_t kGlyphInfo = L'';        // Info
const wchar_t kGlyphChevron = L'';     // ChevronRight
const wchar_t kNewTestHelp[] = L"평가 대상·시험 규칙 설정 및 새 평가 시작";
}  // namespace

BEGIN_MESSAGE_MAP(CHomeView, CWnd)
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_LBUTTONDOWN()
  ON_WM_MOUSEMOVE()
  ON_WM_MOUSELEAVE()
  ON_WM_GETDLGCODE()
  ON_WM_KEYDOWN()
  ON_WM_SETFOCUS()
  ON_WM_KILLFOCUS()
  ON_WM_SIZE()
  ON_MESSAGE(WM_DPICHANGED_AFTERPARENT, &CHomeView::OnDpiChangedAfterParent)
END_MESSAGE_MAP()

CHomeView::~CHomeView() {
  if (m_icon) DestroyIcon(m_icon);
}

BOOL CHomeView::Create(CWnd* parent, UINT id) {
  m_items = {
      {kCmdRunning, kGlyphDiagnostic, L"평가 진행 중", L"실행 현황 및 로그 확인"},
      {kCmdNewTest, kGlyphPlay, L"새 테스트", kNewTestHelp},
      {kCmdResume, kGlyphRefresh, L"이어서 하기", L"저장된 실행 선택 및 평가 재개"},
      {kCmdLoadConfig, kGlyphOpenFile, L"설정 불러오기", L"저장된 평가 설정 확인 및 적용"},
      {kCmdResults, kGlyphReport, L"결과 보기", L"이전 평가 결과·실행 정보 확인 및 리포트 열람"},
      {kCmdAbout, kGlyphInfo, L"FPEval 정보", L""},
  };
  m_items[0].visible = false;
  const CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW));
  return CWnd::Create(cls, L"", WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), parent, id);
}

void CHomeView::SetRunning(bool running, const CString& progress) {
  m_items[0].visible = running;
  m_items[0].title = progress.IsEmpty() ? CString(L"평가 진행 중") : L"평가 진행 중 · " + progress;
  for (auto& it : m_items) {
    if (it.cmd == kCmdNewTest || it.cmd == kCmdResume || it.cmd == kCmdLoadConfig) it.enabled = !running;
  }
  m_items[1].desc = running ? L"현재 평가 종료 후 새 평가 시작 가능" : kNewTestHelp;
  if (!m_items[m_focus].visible || !m_items[m_focus].enabled) m_focus = running ? 0 : 1;
  Layout();
  if (m_hWnd) Invalidate(FALSE);
}

// ------------------------------------------------------------ 배치

void CHomeView::EnsureResources() {
  const int dpi = static_cast<int>(GetDpiForWindow(m_hWnd));
  if (dpi == m_dpi) return;
  m_dpi = dpi;
  const UINT udpi = static_cast<UINT>(dpi);
  theme::CreateUiFont(m_brandFont, 22, FW_BOLD, udpi);
  theme::CreateUiFont(m_headFont, 16, FW_BOLD, udpi);
  theme::CreateUiFont(m_titleFont, 11, FW_BOLD, udpi);
  theme::CreateUiFont(m_textFont, 9, FW_NORMAL, udpi);
  theme::CreateUiFont(m_smallFont, 9, FW_NORMAL, udpi);
  theme::CreateIconFont(m_iconFont, 15, udpi);
  theme::CreateIconFont(m_chevronFont, 9, udpi);
  if (m_icon) DestroyIcon(m_icon);
  m_icon = static_cast<HICON>(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, Px(56),
                                        Px(56), LR_DEFAULTCOLOR));
}

namespace {
int BrandWidth(const CRect& client, int px240) { return std::min(px240, client.Width() / 3); }
}  // namespace

void CHomeView::Layout() {
  if (!m_hWnd) return;
  EnsureResources();
  m_helpPopup.Hide();
  CRect client;
  GetClientRect(&client);
  const int x = client.left + BrandWidth(client, Px(240)) + Px(40);
  const int width = std::max(Px(200), std::min(Px(520), static_cast<int>(client.right) - x - Px(40)));
  const int itemH = Px(54), gap = Px(6), headH = Px(45);
  int menuItems = 0;
  for (const auto& it : m_items) {
    if (it.visible && it.cmd != kCmdAbout) ++menuItems;
  }
  // 제목 + 메뉴 묶음을 세로 가운데에 (아래쪽 정보 링크 자리는 뺀다)
  const int blockH = headH + menuItems * (itemH + gap);
  int y = std::max(Px(24), (client.Height() - Px(48) - blockH) / 2) + headH;
  for (auto& it : m_items) {
    it.help.SetRectEmpty();
    if (it.cmd == kCmdAbout) {
      it.rect.SetRect(x, client.bottom - Px(48), x + Px(130), client.bottom - Px(16));
    } else if (it.visible) {
      it.rect.SetRect(x, y, x + width, y + itemH);
      it.help.SetRect(it.rect.right - Px(64), y, it.rect.right - Px(39), y + itemH);
      y += itemH + gap;
    } else {
      it.rect.SetRectEmpty();
    }
  }
}

BOOL CHomeView::PreTranslateMessage(MSG* msg) {
  if (msg->message == WM_KEYDOWN && msg->wParam == VK_ESCAPE &&
      m_helpPopup.GetSafeHwnd() && m_helpPopup.IsWindowVisible()) {
    m_helpPopup.Hide();
    return TRUE;
  }
  return CWnd::PreTranslateMessage(msg);
}

void CHomeView::OnSize(UINT type, int cx, int cy) {
  CWnd::OnSize(type, cx, cy);
  Layout();
  Invalidate(FALSE);
}

LRESULT CHomeView::OnDpiChangedAfterParent(WPARAM, LPARAM) {
  Layout();
  Invalidate(FALSE);
  return 0;
}

int CHomeView::HitTest(CPoint pt) const {
  for (size_t i = 0; i < m_items.size(); ++i) {
    if (m_items[i].visible && m_items[i].rect.PtInRect(pt)) return static_cast<int>(i);
  }
  return -1;
}

// ------------------------------------------------------------ 그리기

void CHomeView::OnPaint() {
  CPaintDC paintDc(this);
  EnsureResources();
  CRect client;
  GetClientRect(&client);
  CDC dc;
  dc.CreateCompatibleDC(&paintDc);
  CBitmap bmp;
  bmp.CreateCompatibleBitmap(&paintDc, client.Width(), client.Height());
  CBitmap* oldBmp = dc.SelectObject(&bmp);
  dc.SetBkMode(TRANSPARENT);
  dc.FillSolidRect(client, theme::kPanel);

  // 왼쪽 브랜드 패널 (시작 화면과 같은 구성)
  CRect brand(client.left, client.top, client.left + BrandWidth(client, Px(240)), client.bottom);
  dc.FillSolidRect(brand, theme::kBrand);
  const int pad = Px(28);
  CRect badge(brand.left + pad, brand.top + pad + Px(8), brand.left + pad + Px(64), brand.top + pad + Px(72));
  theme::FillRoundRect(&dc, badge, Px(16), theme::kPanel, theme::kPanel);
  if (m_icon) DrawIconEx(dc.m_hDC, badge.left + Px(4), badge.top + Px(4), m_icon, Px(56), Px(56), 0, nullptr, DI_NORMAL);
  CFont* oldFont = dc.SelectObject(&m_brandFont);
  dc.SetTextColor(theme::kOnAccent);
  CRect text(brand.left + pad, badge.bottom + Px(14), brand.right - Px(12), badge.bottom + Px(54));
  dc.DrawText(L"FPEval", text, DT_LEFT | DT_TOP | DT_SINGLELINE | DT_END_ELLIPSIS);
  dc.SelectObject(&m_textFont);
  dc.SetTextColor(theme::kOnAccentSub);
  text.SetRect(brand.left + pad, badge.bottom + Px(54), brand.right - Px(12), badge.bottom + Px(110));
  dc.DrawText(L"지문인식 알고리즘\n성능 평가 프로그램", text, DT_LEFT | DT_TOP | DT_WORDBREAK);
  CString version;
  version.Format(L"v%s · SME3007 팀 프로젝트", theme::kAppVersion);
  text.SetRect(brand.left + pad, brand.bottom - pad - Px(16), brand.right - Px(12), brand.bottom - pad);
  dc.DrawText(version, text, DT_LEFT | DT_BOTTOM | DT_SINGLELINE | DT_END_ELLIPSIS);

  // 오른쪽 제목
  const Item* first = nullptr;
  for (const auto& it : m_items) {
    if (it.visible && it.cmd != kCmdAbout) {
      first = &it;
      break;
    }
  }
  if (first) {
    dc.SelectObject(&m_headFont);
    dc.SetTextColor(theme::kText);
    dc.TextOut(first->rect.left, first->rect.top - Px(42), L"작업 선택");
  }

  for (size_t i = 0; i < m_items.size(); ++i) {
    const Item& it = m_items[i];
    if (!it.visible) continue;
    const bool hot = static_cast<int>(i) == m_hot && it.enabled;
    const COLORREF fg = it.enabled ? theme::kText : theme::kTextDisabled;
    if (it.cmd == kCmdAbout) {  // 아래쪽 작은 링크
      if (hot) theme::FillRoundRect(&dc, it.rect, Px(6), theme::kHover, theme::kHover);
      CRect icon(it.rect.left + Px(6), it.rect.top, it.rect.left + Px(26), it.rect.bottom);
      dc.SelectObject(&m_chevronFont);
      dc.SetTextColor(theme::kTextSecondary);
      const wchar_t g[2] = {it.glyph, 0};
      dc.DrawText(g, icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      dc.SelectObject(&m_smallFont);
      CRect label(icon.right + Px(4), it.rect.top, it.rect.right, it.rect.bottom);
      dc.DrawText(it.title, label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    } else {
      const bool live = it.cmd == kCmdRunning;  // 실행 중 항목은 항상 옅은 강조색 배경
      if (live || hot) {
        const COLORREF fill = live ? theme::kAccentTint : theme::kHover;
        theme::FillRoundRect(&dc, it.rect, Px(10), fill, live && hot ? theme::kAccent : fill);
      } else {
        theme::FillRoundRect(&dc, it.rect, Px(10), theme::kCard, theme::kBorder);
      }
      CRect icon(it.rect.left + Px(14), it.rect.top, it.rect.left + Px(50), it.rect.bottom);
      dc.SelectObject(&m_iconFont);
      dc.SetTextColor(it.enabled ? theme::kAccent : theme::kTextDisabled);
      const wchar_t g[2] = {it.glyph, 0};
      dc.DrawText(g, icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      CRect title(icon.right + Px(12), it.rect.top, it.help.left - Px(6), it.rect.bottom);
      dc.SelectObject(&m_titleFont);
      dc.SetTextColor(fg);
      dc.DrawText(it.title, title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
      theme::DrawHelpBadge(&dc, it.help, static_cast<int>(i) == m_helpHot, static_cast<UINT>(m_dpi));
      if (it.enabled) {
        CRect chevron(it.rect.right - Px(36), it.rect.top, it.rect.right - Px(14), it.rect.bottom);
        dc.SelectObject(&m_chevronFont);
        dc.SetTextColor(theme::kTextSecondary);
        const wchar_t c[2] = {kGlyphChevron, 0};
        dc.DrawText(c, chevron, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
      }
    }
    if (m_hasFocus && static_cast<int>(i) == m_focus) {
      CRect focus(it.rect);
      focus.DeflateRect(Px(3), Px(3));
      dc.SetTextColor(theme::kText);
      dc.DrawFocusRect(focus);
    }
  }
  dc.SelectObject(oldFont);
  paintDc.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
  dc.SelectObject(oldBmp);
}

// ------------------------------------------------------------ 마우스·키보드

void CHomeView::Activate(int index) {
  if (index < 0 || index >= static_cast<int>(m_items.size())) return;
  const Item& it = m_items[index];
  if (!it.visible || !it.enabled) return;
  m_focus = index;
  Invalidate(FALSE);
  GetParent()->PostMessage(WM_APP_HOME_CMD, static_cast<WPARAM>(it.cmd));
}

void CHomeView::OnLButtonDown(UINT flags, CPoint pt) {
  SetFocus();
  const int index = HitTest(pt);
  if (index >= 0 && m_items[index].help.PtInRect(pt)) {
    m_helpPopup.Track(*this, m_items[index].help, m_items[index].desc, true);
  } else {
    m_helpPopup.Hide();
    Activate(index);
  }
  CWnd::OnLButtonDown(flags, pt);
}

void CHomeView::OnMouseMove(UINT flags, CPoint pt) {
  if (!m_tracking) {
    TRACKMOUSEEVENT tme{sizeof tme, TME_LEAVE, m_hWnd, 0};
    TrackMouseEvent(&tme);
    m_tracking = true;
  }
  const int hot = HitTest(pt);
  const int help = hot >= 0 && m_items[hot].help.PtInRect(pt) ? hot : -1;
  if (hot != m_hot || help != m_helpHot) {
    m_hot = hot;
    m_helpHot = help;
    Invalidate(FALSE);
  }
  if (help >= 0) {
    m_helpPopup.Track(*this, m_items[help].help, m_items[help].desc);
  } else {
    m_helpPopup.Hide();
  }
  CWnd::OnMouseMove(flags, pt);
}

void CHomeView::OnMouseLeave() {
  m_tracking = false;
  m_hot = -1;
  m_helpHot = -1;
  m_helpPopup.Hide();
  Invalidate(FALSE);
  CWnd::OnMouseLeave();
}

UINT CHomeView::OnGetDlgCode() {
  UINT code = DLGC_WANTARROWS | DLGC_WANTCHARS;
  const MSG* msg = reinterpret_cast<const MSG*>(GetCurrentMessage()->lParam);
  if (msg && msg->message == WM_KEYDOWN && msg->wParam == VK_RETURN) code |= DLGC_WANTMESSAGE;
  return code;
}

void CHomeView::MoveFocus(int step) {
  const int n = static_cast<int>(m_items.size());
  for (int k = 1; k <= n; ++k) {
    const int i = ((m_focus + step * k) % n + n) % n;
    if (!m_items[i].visible || !m_items[i].enabled) continue;
    m_focus = i;
    Invalidate(FALSE);
    return;
  }
}

void CHomeView::OnKeyDown(UINT key, UINT repeat, UINT flags) {
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

void CHomeView::OnSetFocus(CWnd* old) {
  CWnd::OnSetFocus(old);
  m_hasFocus = true;
  Invalidate(FALSE);
}

void CHomeView::OnKillFocus(CWnd* next) {
  CWnd::OnKillFocus(next);
  m_hasFocus = false;
  Invalidate(FALSE);
}
