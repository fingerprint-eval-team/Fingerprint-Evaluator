#include "pch.h"

#include "HomeView.h"

#include <algorithm>

#include "Theme.h"

namespace {
const wchar_t kGlyphPlay = L'';        // Play
const wchar_t kGlyphRefresh = L'';     // Refresh (이어서 하기)
const wchar_t kGlyphOpenFile = L'';    // OpenFile
const wchar_t kGlyphHistory = L'';     // History (결과 보기)
const wchar_t kGlyphDiagnostic = L'';  // Diagnostic (실행 현황과 같은 글리프)
const wchar_t kGlyphInfo = L'';        // Info
const wchar_t kGlyphChevron = L'';     // ChevronRight
const size_t kMaxRuns = 6;

// "2026-11-24 18:42:10" → 오늘이면 "오늘 18:42", 올해면 "11-24 18:42", 아니면 "2025-11-24"
CString shortDate(const CString& date) {
  if (date.GetLength() < 16) return date;
  SYSTEMTIME now{};
  GetLocalTime(&now);
  CString today;
  today.Format(L"%04d-%02d-%02d", now.wYear, now.wMonth, now.wDay);
  if (date.Left(10) == today) return L"오늘 " + date.Mid(11, 5);
  if (date.Left(4) == today.Left(4)) return date.Mid(5, 5) + L" " + date.Mid(11, 5);
  return date.Left(10);
}
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
  RebuildItems();
  const CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW));
  return CWnd::Create(cls, L"", WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS, CRect(0, 0, 0, 0), parent, id);
}

void CHomeView::SetRunning(bool running, const CString& progress) {
  m_running = running;
  m_progress = progress;
  RebuildItems();
  Layout();
  if (m_hWnd) Invalidate(FALSE);
}

void CHomeView::SetRunRoots(const std::vector<std::wstring>& roots) {
  m_roots = roots;
  ReloadRuns();
}

void CHomeView::ReloadRuns() {
  m_runs = ScanRecentRuns(m_roots, kMaxRuns);
  RebuildItems();
  Layout();
  if (m_hWnd) Invalidate(FALSE);
}

const RecentRun* CHomeView::RunAt(int index) const {
  return index >= 0 && index < static_cast<int>(m_runs.size()) ? &m_runs[index] : nullptr;
}

// 항목 목록을 현재 상태(실행 중 여부·최근 실행)로 다시 만든다. 순서가 곧 키보드 이동 순서다.
void CHomeView::RebuildItems() {
  // 포커스는 같은 항목(명령·종류·실행 번호)에 남긴다
  Cmd focusCmd = kCmdNewTest;
  Kind focusKind = Kind::Primary;
  int focusRun = -1;
  if (m_focus >= 0 && m_focus < static_cast<int>(m_items.size())) {
    focusCmd = m_items[m_focus].cmd;
    focusKind = m_items[m_focus].kind;
    focusRun = m_items[m_focus].run;
  }

  m_items.clear();
  Item banner{kCmdRunning, Kind::Banner, kGlyphDiagnostic};
  banner.title = m_progress.IsEmpty() ? CString(L"평가 진행 중") : L"평가 진행 중 · " + m_progress;
  banner.desc = L"실행 현황 보기";
  banner.visible = m_running;
  m_items.push_back(banner);

  Item primary{kCmdNewTest, Kind::Primary, kGlyphPlay, L"새 테스트"};
  primary.desc = m_running ? L"현재 평가가 끝나면 시작" : L"DLL과 DB를 골라 평가";
  primary.enabled = !m_running;
  m_items.push_back(primary);
  Item resume{kCmdResume, Kind::Tile, kGlyphRefresh, L"이어서 하기", L"중단된 실행 재개"};
  resume.enabled = !m_running;
  m_items.push_back(resume);
  Item load{kCmdLoadConfig, Kind::Tile, kGlyphOpenFile, L"설정 불러오기", L"저장된 JSON 설정"};
  load.enabled = !m_running;
  m_items.push_back(load);
  m_items.push_back({kCmdResults, Kind::Tile, kGlyphHistory, L"결과 보기", L"리포트와 실행 기록"});

  m_items.push_back({kCmdResults, Kind::Link, kGlyphChevron, L"모두 보기"});
  for (int i = 0; i < static_cast<int>(m_runs.size()); ++i) {
    Item row{kCmdOpenRun, Kind::Run};
    row.run = i;
    m_items.push_back(row);
  }
  m_items.push_back({kCmdAbout, Kind::Footer, kGlyphInfo, L"FPEval 정보"});

  m_focus = -1;
  for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
    const Item& it = m_items[i];
    if (it.cmd == focusCmd && it.kind == focusKind && it.run == focusRun && it.visible && it.enabled) m_focus = i;
  }
  if (m_focus < 0) m_focus = FindItem(m_running ? kCmdRunning : kCmdNewTest, m_running ? Kind::Banner : Kind::Primary);
}

int CHomeView::FindItem(Cmd cmd, Kind kind) const {
  for (int i = 0; i < static_cast<int>(m_items.size()); ++i) {
    if (m_items[i].cmd == cmd && m_items[i].kind == kind) return i;
  }
  return 0;
}

// ------------------------------------------------------------ 배치

void CHomeView::EnsureResources() {
  const int dpi = static_cast<int>(GetDpiForWindow(m_hWnd));
  if (dpi == m_dpi) return;
  m_dpi = dpi;
  const UINT udpi = static_cast<UINT>(dpi);
  theme::CreateUiFont(m_brandFont, 13, FW_BOLD, udpi);
  theme::CreateUiFont(m_headFont, 18, FW_BOLD, udpi);
  theme::CreateUiFont(m_titleFont, 11, FW_BOLD, udpi);
  theme::CreateUiFont(m_textFont, 9, FW_NORMAL, udpi);
  theme::CreateUiFont(m_smallFont, 8, FW_NORMAL, udpi);
  theme::CreateIconFont(m_iconFont, 15, udpi);
  theme::CreateIconFont(m_chevronFont, 8, udpi);
  if (m_icon) DestroyIcon(m_icon);
  m_icon = static_cast<HICON>(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, Px(36),
                                        Px(36), LR_DEFAULTCOLOR));
}

void CHomeView::Layout() {
  if (!m_hWnd) return;
  EnsureResources();
  CRect client;
  GetClientRect(&client);
  // 가운데 정렬한 본문 열
  const int width = std::max(Px(360), std::min(Px(880), static_cast<int>(client.Width()) - Px(80)));
  const int left = client.left + std::max(Px(16), (static_cast<int>(client.Width()) - width) / 2);
  const int right = left + width;
  int y = Px(148);  // 머리글(로고) + 인사말 아래

  Item& banner = m_items[FindItem(kCmdRunning, Kind::Banner)];
  if (banner.visible) {
    banner.rect.SetRect(left, y, right, y + Px(48));
    y += Px(48) + Px(12);
  } else {
    banner.rect.SetRectEmpty();
  }

  // 타일: 새 테스트 1.4, 나머지 1
  const int gap = Px(12), tileH = Px(112);
  const int unit = (width - 3 * gap) * 10 / 44;
  int x = left;
  for (auto& it : m_items) {
    if (it.kind != Kind::Primary && it.kind != Kind::Tile) continue;
    const int w = it.kind == Kind::Primary ? (width - 3 * gap) - 3 * unit : unit;
    it.rect.SetRect(x, y, x + w, y + tileH);
    x += w + gap;
  }
  y += tileH + Px(28);

  // 최근 실행 제목줄과 "모두 보기"
  m_recentHead.SetRect(left, y, right, y + Px(28));
  Item& link = m_items[FindItem(kCmdResults, Kind::Link)];
  link.rect.SetRect(right - Px(92), y, right, y + Px(28));
  y += Px(34);

  // 최근 실행 줄: 아래 정보 링크 자리를 빼고 들어가는 만큼만
  const int rowH = Px(44);
  const int footerTop = client.bottom - Px(44);
  for (auto& it : m_items) {
    if (it.kind != Kind::Run) continue;
    it.visible = y + rowH <= footerTop - Px(8);
    it.rect = it.visible ? CRect(left, y, right, y + rowH) : CRect();
    if (it.visible) y += rowH;
  }
  Item& footer = m_items[FindItem(kCmdAbout, Kind::Footer)];
  footer.rect.SetRect(left - Px(6), footerTop + Px(6), left + Px(124), client.bottom - Px(10));
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

void CHomeView::DrawRun(CDC& dc, const Item& it, bool hot) {
  const RecentRun& run = m_runs[it.run];
  const CRect& rc = it.rect;
  if (hot) theme::FillRoundRect(&dc, rc, Px(8), theme::kHover, theme::kHover);
  dc.FillSolidRect(rc.left, rc.bottom - 1, rc.Width(), 1, theme::kBorder);

  // 오른쪽부터: 날짜, 상태 표시, EER
  CRect date(rc.right - Px(104), rc.top, rc.right - Px(10), rc.bottom);
  dc.SelectObject(&m_textFont);
  dc.SetTextColor(theme::kTextSecondary);
  dc.DrawText(shortDate(run.date), date, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

  CString status;
  COLORREF chipBg, chipFg;
  if (run.complete) {
    status = L"완료";
    chipBg = theme::kSuccessTint;
    chipFg = theme::kSuccess;
  } else if (!run.hasSummary && m_running && it.run == 0) {
    status = L"진행 중";
    chipBg = theme::kAccentTint;
    chipFg = theme::kAccent;
  } else {
    status = run.hasSummary ? L"미완료" : L"중단";
    chipBg = theme::kWarningTint;
    chipFg = theme::kWarning;
  }
  dc.SelectObject(&m_smallFont);
  const int chipW = dc.GetTextExtent(status).cx + Px(18);
  const int chipH = Px(22);
  CRect chip(date.left - Px(12) - chipW, rc.CenterPoint().y - chipH / 2, date.left - Px(12), rc.CenterPoint().y + chipH / 2);
  theme::FillRoundRect(&dc, chip, Px(8), chipBg, chipBg);
  dc.SetTextColor(chipFg);
  dc.DrawText(status, chip, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

  CRect eer(chip.left - Px(120), rc.top, chip.left - Px(16), rc.bottom);
  dc.SelectObject(&m_textFont);
  dc.SetTextColor(run.eer.IsEmpty() ? theme::kTextDisabled : theme::kText);
  CString eerText = L"EER —";
  if (!run.eer.IsEmpty()) eerText.Format(L"EER %.2f%%", _wtof(run.eer));  // summary.csv는 6.583333처럼 길다
  dc.DrawText(eerText, eer, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

  CRect name(rc.left + Px(10), rc.top, eer.left - Px(12), rc.bottom);
  CString dll = run.dll.IsEmpty() ? CString(L"(DLL 정보 없음)") : run.dll;
  dc.SelectObject(&m_titleFont);
  dc.SetTextColor(theme::kText);
  const int dllW = std::min(static_cast<int>(dc.GetTextExtent(dll).cx), name.Width() * 3 / 5);
  CRect dllRc(name.left, name.top, name.left + dllW, name.bottom);
  dc.DrawText(dll, dllRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
  if (!run.db.IsEmpty()) {
    CRect dbRc(dllRc.right + Px(10), name.top, name.right, name.bottom);
    dc.SelectObject(&m_textFont);
    dc.SetTextColor(theme::kTextSecondary);
    dc.DrawText(run.db, dbRc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS | DT_NOPREFIX);
  }
}

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

  const Item& primary = m_items[FindItem(kCmdNewTest, Kind::Primary)];
  const int left = primary.rect.left;
  const int right = m_recentHead.right;

  // 머리글: 로고 + 이름 + 버전
  const int headY = Px(24);
  if (m_icon) DrawIconEx(dc.m_hDC, left, headY, m_icon, Px(36), Px(36), 0, nullptr, DI_NORMAL);
  CFont* oldFont = dc.SelectObject(&m_brandFont);
  dc.SetTextColor(theme::kText);
  dc.TextOut(left + Px(46), headY - Px(1), L"FPEval");
  dc.SelectObject(&m_smallFont);
  dc.SetTextColor(theme::kTextSecondary);
  dc.TextOut(left + Px(46), headY + Px(22), L"지문인식 알고리즘 평가");
  CString version;
  version.Format(L"v%s · SME3007", theme::kAppVersion);
  CRect versionRc(right - Px(200), headY, right, headY + Px(36));
  dc.DrawText(version, versionRc, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

  // 인사말과 마지막 실행 요약
  dc.SelectObject(&m_headFont);
  dc.SetTextColor(theme::kText);
  dc.TextOut(left, Px(80), L"무엇을 평가할까요?");
  dc.SelectObject(&m_textFont);
  dc.SetTextColor(theme::kTextSecondary);
  CString last = L"아직 실행 기록이 없습니다";
  if (!m_runs.empty()) {
    last = L"마지막 실행 · " + shortDate(m_runs[0].date);
    if (!m_runs[0].db.IsEmpty()) last += L" · " + m_runs[0].db;
  }
  dc.TextOut(left, Px(116), last);

  for (size_t i = 0; i < m_items.size(); ++i) {
    const Item& it = m_items[i];
    if (!it.visible || it.rect.IsRectEmpty()) continue;
    const bool hot = static_cast<int>(i) == m_hot && it.enabled;
    const wchar_t glyph[2] = {it.glyph, 0};
    switch (it.kind) {
      case Kind::Banner: {
        theme::FillRoundRect(&dc, it.rect, Px(10), theme::kAccentTint, hot ? theme::kAccent : theme::kAccentTint);
        CRect icon(it.rect.left + Px(12), it.rect.top, it.rect.left + Px(44), it.rect.bottom);
        dc.SelectObject(&m_iconFont);
        dc.SetTextColor(theme::kAccent);
        dc.DrawText(glyph, icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        CRect title(icon.right + Px(8), it.rect.top, it.rect.right - Px(160), it.rect.bottom);
        dc.SelectObject(&m_titleFont);
        dc.SetTextColor(theme::kText);
        dc.DrawText(it.title, title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        CRect go(it.rect.right - Px(160), it.rect.top, it.rect.right - Px(34), it.rect.bottom);
        dc.SelectObject(&m_textFont);
        dc.SetTextColor(theme::kAccent);
        dc.DrawText(it.desc, go, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        CRect chevron(it.rect.right - Px(32), it.rect.top, it.rect.right - Px(12), it.rect.bottom);
        dc.SelectObject(&m_chevronFont);
        const wchar_t c[2] = {kGlyphChevron, 0};
        dc.DrawText(c, chevron, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        break;
      }
      case Kind::Primary:
      case Kind::Tile: {
        const bool primaryTile = it.kind == Kind::Primary;
        COLORREF fill, border, ink, sub;
        if (!it.enabled) {
          fill = border = theme::kCard;
          ink = sub = theme::kTextDisabled;
        } else if (primaryTile) {  // 핵심 동작: 보라
          fill = border = hot ? theme::kAccentHover : theme::kAccent;
          ink = theme::kOnAccent;
          sub = theme::kOnAccentSub;
        } else {  // 보조 동작: 흰 타일 + 중립 테두리
          fill = hot ? theme::kHover : theme::kPanel;
          border = hot ? theme::kControlBorder : theme::kBorder;
          ink = theme::kText;
          sub = theme::kTextSecondary;
        }
        theme::FillRoundRect(&dc, it.rect, Px(12), fill, border);
        const int pad = Px(16);
        CRect icon(it.rect.left + pad, it.rect.top + pad, it.rect.left + pad + Px(28), it.rect.top + pad + Px(28));
        dc.SelectObject(&m_iconFont);
        dc.SetTextColor(ink);
        dc.DrawText(glyph, icon, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        CRect title(it.rect.left + pad, it.rect.bottom - Px(58), it.rect.right - Px(8), it.rect.bottom - Px(34));
        dc.SelectObject(&m_titleFont);
        dc.DrawText(it.title, title, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        CRect desc(it.rect.left + pad, it.rect.bottom - Px(34), it.rect.right - Px(8), it.rect.bottom - Px(12));
        dc.SelectObject(&m_textFont);
        dc.SetTextColor(sub);
        dc.DrawText(it.desc, desc, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
        break;
      }
      case Kind::Link: {
        dc.SelectObject(&m_titleFont);
        dc.SetTextColor(theme::kText);
        dc.DrawText(L"최근 실행", m_recentHead, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        if (hot) theme::FillRoundRect(&dc, it.rect, Px(6), theme::kHover, theme::kHover);
        CRect chevron(it.rect.right - Px(22), it.rect.top, it.rect.right - Px(4), it.rect.bottom);
        CRect label(it.rect.left, it.rect.top, chevron.left, it.rect.bottom);
        dc.SelectObject(&m_textFont);
        dc.SetTextColor(theme::kAccent);
        dc.DrawText(it.title, label, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
        dc.SelectObject(&m_chevronFont);
        dc.DrawText(glyph, chevron, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        break;
      }
      case Kind::Run:
        DrawRun(dc, it, hot);
        break;
      case Kind::Footer: {
        if (hot) theme::FillRoundRect(&dc, it.rect, Px(6), theme::kHover, theme::kHover);
        CRect icon(it.rect.left + Px(6), it.rect.top, it.rect.left + Px(26), it.rect.bottom);
        dc.SelectObject(&m_chevronFont);
        dc.SetTextColor(theme::kTextSecondary);
        dc.DrawText(glyph, icon, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        dc.SelectObject(&m_textFont);
        CRect label(icon.right + Px(4), it.rect.top, it.rect.right, it.rect.bottom);
        dc.DrawText(it.title, label, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
        break;
      }
    }
    if (m_hasFocus && static_cast<int>(i) == m_focus) {
      CRect focus(it.rect);
      focus.DeflateRect(Px(3), Px(3));
      dc.SetTextColor(it.kind == Kind::Primary && it.enabled ? theme::kOnAccent : theme::kText);
      dc.SetBkColor(it.kind == Kind::Primary && it.enabled ? theme::kAccent : theme::kPanel);
      dc.DrawFocusRect(focus);
    }
  }

  // 최근 실행이 없을 때 안내
  if (m_runs.empty()) {
    CRect empty(left, m_recentHead.bottom + Px(6), right, m_recentHead.bottom + Px(62));
    theme::FillRoundRect(&dc, empty, Px(10), theme::kCard, theme::kCard);
    dc.SelectObject(&m_textFont);
    dc.SetTextColor(theme::kTextSecondary);
    dc.DrawText(L"아직 실행 기록이 없습니다. 새 테스트로 첫 평가를 시작하세요.", empty,
                DT_CENTER | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS);
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
  GetParent()->PostMessage(WM_APP_HOME_CMD, static_cast<WPARAM>(it.cmd), static_cast<LPARAM>(it.run));
}

void CHomeView::OnLButtonDown(UINT flags, CPoint pt) {
  SetFocus();
  Activate(HitTest(pt));
  CWnd::OnLButtonDown(flags, pt);
}

void CHomeView::OnMouseMove(UINT flags, CPoint pt) {
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

void CHomeView::OnMouseLeave() {
  m_tracking = false;
  m_hot = -1;
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
    if (!m_items[i].visible || !m_items[i].enabled || m_items[i].rect.IsRectEmpty()) continue;
    m_focus = i;
    Invalidate(FALSE);
    return;
  }
}

void CHomeView::OnKeyDown(UINT key, UINT repeat, UINT flags) {
  switch (key) {
    case VK_UP:
    case VK_LEFT:
      MoveFocus(-1);
      return;
    case VK_DOWN:
    case VK_RIGHT:
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
