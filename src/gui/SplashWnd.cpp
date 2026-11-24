#include "pch.h"

#include "SplashWnd.h"

#include "Theme.h"

namespace {
const UINT_PTR kTimerStep = 1;
const UINT kStepMs = 220;  // 점검 한 단계를 보여 주는 최소 시간
const UINT kHoldMs = 450;  // 모두 끝난 뒤 결과를 보여 주는 시간
const wchar_t kGlyphCheck = L'';    // CheckMark
const wchar_t kGlyphWarning = L'';  // Warning
}  // namespace

BEGIN_MESSAGE_MAP(CSplashWnd, CWnd)
  ON_WM_PAINT()
  ON_WM_ERASEBKGND()
  ON_WM_TIMER()
  ON_WM_LBUTTONDOWN()
  ON_WM_KEYDOWN()
END_MESSAGE_MAP()

void CSplashWnd::Run() {
  m_steps = {{L"평가 엔진 준비"},
             {L"FPWorker64 확인"},
             {L"FPWorker32 확인"},
             {L"최근 설정 확인"},
             {L"사용자 데이터 폴더 확인"}};
  const CString cls = AfxRegisterWndClass(CS_HREDRAW | CS_VREDRAW, ::LoadCursor(nullptr, IDC_ARROW));
  if (!CreateEx(WS_EX_TOOLWINDOW, cls, L"FPEval", WS_POPUP, CRect(0, 0, 1, 1), nullptr, 0)) return;

  // 마우스가 있는 모니터 가운데에, 그 모니터 배율에 맞는 크기로 띄운다
  POINT cursor{};
  GetCursorPos(&cursor);
  MONITORINFO mi{sizeof mi};
  GetMonitorInfo(MonitorFromPoint(cursor, MONITOR_DEFAULTTOPRIMARY), &mi);
  const CRect work(mi.rcWork);
  SetWindowPos(nullptr, work.left, work.top, 1, 1, SWP_NOZORDER | SWP_NOACTIVATE);  // 그 모니터로 옮겨 DPI를 맞춘다
  m_dpi = static_cast<int>(GetDpiForWindow(m_hWnd));
  const int w = Px(520), h = Px(260);
  SetWindowPos(nullptr, work.left + (work.Width() - w) / 2, work.top + (work.Height() - h) / 2, w, h,
               SWP_NOZORDER | SWP_NOACTIVATE);
  // 둥근 모서리 (Windows 10에는 테두리 없는 창의 기본 둥근 모서리가 없다)
  const int r = Px(16);
  SetWindowRgn(CreateRoundRectRgn(0, 0, w + 1, h + 1, r, r), FALSE);

  theme::CreateUiFont(m_titleFont, 20, FW_BOLD, m_dpi);
  theme::CreateUiFont(m_textFont, 10, FW_NORMAL, m_dpi);
  theme::CreateUiFont(m_smallFont, 8, FW_NORMAL, m_dpi);
  theme::CreateIconFont(m_iconFont, 9, m_dpi);
  m_icon = static_cast<HICON>(LoadImage(AfxGetResourceHandle(), MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, Px(56),
                                        Px(56), LR_DEFAULTCOLOR));

  m_steps[0].state = State::Running;
  ShowWindow(SW_SHOW);
  UpdateWindow();
  SetForegroundWindow();
  SetFocus();
  SetTimer(kTimerStep, kStepMs, nullptr);

  // 점검이 끝날 때까지 이 창의 메시지만 처리한다 (메인 창은 아직 없다)
  MSG msg;
  while (!m_done && ::GetMessage(&msg, nullptr, 0, 0) > 0) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
  KillTimer(kTimerStep);
  DestroyWindow();
  if (m_icon) DestroyIcon(m_icon);
  m_icon = nullptr;
}

void CSplashWnd::RunStep(int index) {
  Step& s = m_steps[index];
  bool ok = true;
  switch (index) {
    case 0:  // 엔진은 FPEval.exe에 정적으로 링크되어 있다. 시험 대상 기본 DLL이 있는지만 본다
      ok = true;
      if (!fpe::fileExists(fpe::joinPath(fpe::exeDir(), L"plugins\\x64\\FPAlgo.dll"))) s.result = L"기본 DLL 없음";
      break;
    case 1:
    case 2: {
      const wchar_t* exe = index == 1 ? L"FPWorker64.exe" : L"FPWorker32.exe";
      ok = fpe::fileExists(fpe::joinPath(fpe::exeDir(), exe));
      if (!ok) s.result = L"찾을 수 없음";
      break;
    }
    case 3:
      s.result = fpe::fileExists(fpe::joinPath(fpe::userDataDir(), L"last_config.json")) ? L"최근 설정 사용"
                                                                                            : L"첫 실행 · 기본 설정";
      break;
    case 4: {
      const std::wstring results = fpe::joinPath(fpe::userDataDir(), L"results");
      ok = fpe::makeDirs(results) || fpe::dirExists(results);
      s.result = ok ? CString(L"문서\\FPEval") : CString(L"만들 수 없음");
      break;
    }
    default:
      break;
  }
  s.state = ok ? State::Ok : State::Warn;
}

void CSplashWnd::OnTimer(UINT_PTR id) {
  if (id != kTimerStep) return CWnd::OnTimer(id);
  if (m_current >= static_cast<int>(m_steps.size())) {  // 결과를 잠깐 보여 준 뒤 닫는다
    m_done = true;
    PostMessage(WM_NULL);  // GetMessage 루프를 깨운다
    return;
  }
  RunStep(m_current);
  ++m_current;
  if (m_current < static_cast<int>(m_steps.size())) {
    m_steps[m_current].state = State::Running;
  } else {
    SetTimer(kTimerStep, kHoldMs, nullptr);
  }
  Invalidate(FALSE);
}

void CSplashWnd::OnLButtonDown(UINT, CPoint) {  // 클릭하면 남은 점검을 바로 끝내고 닫는다
  while (m_current < static_cast<int>(m_steps.size())) RunStep(m_current++);
  m_done = true;
  PostMessage(WM_NULL);
}

void CSplashWnd::OnKeyDown(UINT key, UINT repeat, UINT flags) {
  if (key == VK_ESCAPE || key == VK_RETURN || key == VK_SPACE) {
    OnLButtonDown(0, CPoint());
    return;
  }
  CWnd::OnKeyDown(key, repeat, flags);
}

void CSplashWnd::OnPaint() {
  CPaintDC paintDc(this);
  CRect client;
  GetClientRect(&client);
  CDC dc;
  dc.CreateCompatibleDC(&paintDc);
  CBitmap bmp;
  bmp.CreateCompatibleBitmap(&paintDc, client.Width(), client.Height());
  CBitmap* oldBmp = dc.SelectObject(&bmp);
  dc.SetBkMode(TRANSPARENT);
  dc.FillSolidRect(client, theme::kPanel);

  // 왼쪽 강조색 패널
  CRect brand(client.left, client.top, client.left + Px(190), client.bottom);
  dc.FillSolidRect(brand, theme::kBrand);
  const int pad = Px(22);
  if (m_icon) {  // 아이콘은 흰 원 배경 위에 (파란 바탕에서 지문 아이콘이 묻히지 않게)
    CRect badge(brand.left + pad, brand.top + pad, brand.left + pad + Px(64), brand.top + pad + Px(64));
    theme::FillRoundRect(&dc, badge, Px(16), theme::kPanel, theme::kPanel);
    DrawIconEx(dc.m_hDC, badge.left + Px(4), badge.top + Px(4), m_icon, Px(56), Px(56), 0, nullptr, DI_NORMAL);
  }
  CFont* oldFont = dc.SelectObject(&m_titleFont);
  dc.SetTextColor(theme::kOnAccent);
  dc.TextOut(brand.left + pad, brand.top + pad + Px(76), L"FPEval");
  dc.SelectObject(&m_textFont);
  dc.SetTextColor(theme::kOnAccentSub);
  dc.TextOut(brand.left + pad, brand.top + pad + Px(110), L"지문인식 알고리즘 평가");
  dc.SelectObject(&m_smallFont);
  CString version;
  version.Format(L"v%s · SME3007", theme::kAppVersion);
  dc.TextOut(brand.left + pad, brand.bottom - pad - Px(14), version);

  // 오른쪽: 시작 준비 목록
  const int x = brand.right + Px(26);
  LOGFONT lf{};
  m_textFont.GetLogFont(&lf);
  lf.lfWeight = FW_BOLD;
  CFont bold;
  bold.CreateFontIndirect(&lf);
  dc.SelectObject(&bold);
  dc.SetTextColor(theme::kText);
  dc.TextOut(x, Px(28), L"시작 준비");

  int y = Px(62);
  const int rowH = Px(28);
  for (const Step& s : m_steps) {
    CRect mark(x, y, x + Px(18), y + rowH);
    switch (s.state) {
      case State::Ok:
      case State::Warn: {
        dc.SelectObject(&m_iconFont);
        dc.SetTextColor(s.state == State::Ok ? theme::kSuccess : theme::kDanger);
        const wchar_t g[2] = {s.state == State::Ok ? kGlyphCheck : kGlyphWarning, 0};
        dc.DrawText(g, mark, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        break;
      }
      case State::Running:
      case State::Pending: {  // 진행 중은 강조색 점, 대기는 회색 테두리 점
        const int d = Px(8);
        CRect dot(mark.CenterPoint().x - d / 2, mark.CenterPoint().y - d / 2, mark.CenterPoint().x + d / 2,
                  mark.CenterPoint().y + d / 2);
        if (s.state == State::Running) {
          theme::FillRoundRect(&dc, dot, d, theme::kAccent, theme::kAccent);
        } else {
          theme::FillRoundRect(&dc, dot, d, theme::kPanel, theme::kTextDisabled);
        }
        break;
      }
    }
    dc.SelectObject(&m_textFont);
    dc.SetTextColor(s.state == State::Pending ? theme::kTextDisabled : theme::kText);
    CRect text(x + Px(28), y, client.right - Px(20), y + rowH);
    CString line = s.text;
    if (s.state == State::Running) line += L"…";
    dc.DrawText(line, text, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    if (!s.result.IsEmpty() && (s.state == State::Ok || s.state == State::Warn)) {
      dc.SelectObject(&m_smallFont);
      dc.SetTextColor(s.state == State::Warn ? theme::kDanger : theme::kTextSecondary);
      dc.DrawText(s.result, text, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);
    }
    y += rowH;
  }

  // 진행 막대
  const int total = static_cast<int>(m_steps.size());
  CRect track(x, client.bottom - Px(30), client.right - Px(20), client.bottom - Px(26));
  dc.FillSolidRect(track, theme::kBorder);
  CRect fill(track);
  fill.right = track.left + MulDiv(track.Width(), m_current, total);
  dc.FillSolidRect(fill, theme::kAccent);

  dc.SelectObject(oldFont);
  paintDc.BitBlt(0, 0, client.Width(), client.Height(), &dc, 0, 0, SRCCOPY);
  dc.SelectObject(oldBmp);
}
