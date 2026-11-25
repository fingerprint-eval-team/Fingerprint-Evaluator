#include "pch.h"

#include "MainDlg.h"

#include <algorithm>
#include <cmath>

#include "Theme.h"
#include "plan/EvalPlan.h"

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
  ON_MESSAGE(WM_APP_NAV, &CMainDlg::OnNav)
  ON_MESSAGE(WM_APP_NAV_TOGGLE, &CMainDlg::OnNavToggle)
  ON_MESSAGE(WM_APP_NAV_HOME, &CMainDlg::OnNavHome)
  ON_MESSAGE(WM_APP_HOME_CMD, &CMainDlg::OnHomeCmd)
  ON_BN_CLICKED(IDC_START, &CMainDlg::OnStart)
  ON_BN_CLICKED(IDC_STOP, &CMainDlg::OnStop)
  ON_BN_CLICKED(IDC_ABORT, &CMainDlg::OnAbort)
  ON_BN_CLICKED(IDC_RESUME, &CMainDlg::OnResume)
  ON_BN_CLICKED(IDC_REPORT_ONLY, &CMainDlg::OnReportOnly)
  ON_COMMAND(IDC_SAVE_CFG, &CMainDlg::OnSaveConfig)  // 사이드바 동작 항목
  ON_COMMAND(IDC_LOAD_CFG, &CMainDlg::OnLoadConfig)
  ON_COMMAND(IDM_ABOUT, &CMainDlg::OnAbout)
  ON_BN_CLICKED(IDC_OPEN_REPORT, &CMainDlg::OnOpenReport)
  ON_BN_CLICKED(IDC_OPEN_FOLDER, &CMainDlg::OnOpenFolder)
  ON_WM_CLOSE()
  ON_WM_SIZE()
  ON_WM_ERASEBKGND()
  ON_WM_CTLCOLOR()
  ON_WM_GETMINMAXINFO()
  ON_MESSAGE(WM_DPICHANGED, &CMainDlg::OnDpiChanged)
  ON_MESSAGE(WM_APP_PHASE, &CMainDlg::OnPhase)
  ON_MESSAGE(WM_APP_PROGRESS, &CMainDlg::OnProgress)
  ON_MESSAGE(WM_APP_LOG, &CMainDlg::OnLog)
  ON_MESSAGE(WM_APP_DONE, &CMainDlg::OnDone)
  ON_MESSAGE(WM_APP_DB_CHANGED, &CMainDlg::OnDbChanged)
  ON_MESSAGE(WM_APP_RULES_CHANGED, &CMainDlg::OnRulesChanged)
END_MESSAGE_MAP()

void CMainDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_NAV, m_nav);
  DDX_Control(pDX, IDC_START, m_start);
  DDX_Control(pDX, IDC_PROGRESS, m_progress);
}

namespace {
// 기본 테스트 DB: exe 옆 또는 상위(개발 빌드의 bin\..)의 testdb\FVC* 중 이름순 첫 폴더. 없으면 빈 문자열.
std::wstring findDefaultDb() {
  const std::wstring roots[] = {fpe::joinPath(fpe::exeDir(), L"testdb"),
                                fpe::joinPath(fpe::dirOf(fpe::exeDir()), L"testdb")};
  for (const auto& root : roots) {
    std::vector<std::wstring> dirs;
    WIN32_FIND_DATAW fd;
    HANDLE h = FindFirstFileW(fpe::joinPath(root, L"FVC*").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) continue;
    do {
      if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) dirs.push_back(fpe::joinPath(root, fd.cFileName));
    } while (FindNextFileW(h, &fd));
    FindClose(h);
    std::sort(dirs.begin(), dirs.end());
    if (!dirs.empty()) return dirs.front();
  }
  return L"";
}

// DB 폴더 이름으로 iType 추정 (과제 자료 p6, 대상 탭 안내와 같음: FVC2000=1, FVC2002=2, FVC2004=3)
int guessIType(const std::wstring& dbPath, int fallback) {
  std::wstring name = fpe::fileNameOf(dbPath);
  if (name.find(L"FVC2000") != std::wstring::npos) return 1;
  if (name.find(L"FVC2002") != std::wstring::npos) return 2;
  if (name.find(L"FVC2004") != std::wstring::npos) return 3;
  return fallback;
}
}  // namespace

// 설정 파일은 문서\FPEval에 둔다 (설치판의 Program Files는 쓰기 불가)
std::wstring CMainDlg::LastConfigPath() const { return fpe::joinPath(fpe::userDataDir(), L"last_config.json"); }

// ------------------------------------------------------------ 초기화

namespace {
// 페이지 제목과 한 줄 설명 (사이드바 항목 순서와 같다)
const wchar_t* const kPageTitles[] = {L"평가 대상", L"시험 규칙", L"실행 옵션", L"리포트", L"실행 현황", L"이전 결과",
                                     L"이어서 하기", L"설정 불러오기"};
const wchar_t* const kPageDescs[] = {
    L"알고리즘 DLL·테스트 DB·결과 저장 위치 선택",
    L"등록·본인비교·타인비교 조합 규칙 및 재현용 seed 설정",
    L"동시 수행 수·시간 제한·규칙 유추 차단 설정",
    L"리포트 지표·그래프·파일 형식 선택",
    L"단계별 진행 상황·실패 건수·실행 로그 확인",
    L"이전 평가 실행 정보 확인 및 HTML·Word 리포트 열람",
    L"저장된 실행 선택 및 평가 재개",
    L"저장된 설정 확인 및 평가 설정에 적용",
};
}  // namespace

CMainDlg::~CMainDlg() { DestroyIcons(); }

BOOL CMainDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();
  SetBackgroundColor(theme::kWindowBg);  // 라벨도 같은 배경으로 그린다 (CDialogEx::OnCtlColor)
  CAccentButton::SubclassPushButtons(this, theme::kWindowBg, m_buttons);
  SetupLayout();

  // 글리프는 Segoe Fluent Icons / MDL2 Assets
  m_nav.AddPage(0, L'\uE928', kPageTitles[0]);  // Fingerprint
  m_nav.AddPage(1, L'\uE8B1', kPageTitles[1]);  // Shuffle
  m_nav.AddPage(kRunOptionsPage, L'\uE950', kPageTitles[kRunOptionsPage]);  // Processor (CPU)
  m_nav.AddPage(3, L'\uE9F9', kPageTitles[3]);  // ReportDocument
  m_nav.AddPage(kDashPage, L'\uE9D9', kPageTitles[kDashPage], true);  // Diagnostic
  m_nav.AddPage(kResumePage, L'\uE768', kPageTitles[kResumePage]);  // Play
  m_nav.AddPage(kConfigPage, L'\uE8E5', kPageTitles[kConfigPage]);  // OpenFile
  m_nav.AddPage(kResultsPage, L'\uE81C', kPageTitles[kResultsPage]);  // History
  m_nav.AddAction(L'\uE74E', L"설정 저장…", IDC_SAVE_CFG);      // Save
  m_nav.AddAction(L'\uE946', L"FPEval 정보", IDM_ABOUT);         // Info
  CreateDpiResources();

  // 페이지 생성: 메인 대화상자의 자식으로 만들고 IDC_PAGE_AREA 자리에 겹쳐 둔다.
  // Z 순서를 사이드바 바로 뒤로 두어 Tab 이동이 사이드바 → 페이지 → 실행 버튼 순서가 되게 한다.
  const UINT ids[] = {IDD_PAGE_TARGET, IDD_PAGE_RULES, IDD_PAGE_RUN, IDD_PAGE_REPORT};
  for (int i = 0; i < 4; ++i) m_pages[i]->Create(ids[i], this);
  m_dash.Create(IDD_PAGE_DASH, this);
  m_results.Create(IDD_PAGE_RESULTS, this);
  m_results.IncludeRoot(fpe::joinPath(fpe::userDataDir(), L"results"));
  m_results.IncludeConfigRoot(fpe::userDataDir());
  m_results.IncludeConfigRoot(fpe::exeDir());
  for (int i = 0; i < kPageCount; ++i)
    Page(i)->SetWindowPos(&m_nav, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_HIDEWINDOW);
  // 홈 화면: 다른 컨트롤 위(Z 순서 맨 앞)에 클라이언트 전체 크기로 둔다
  m_home.Create(this, IDC_HOME);
  m_home.SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);
  LayoutPages();
  m_progress.SetRange32(0, 1000);
  SetWindowTheme(m_progress.GetSafeHwnd(), L"", L"");
  m_progress.SetBarColor(theme::kAccent);
  m_progress.SetBkColor(theme::kAccentTint);

  // 마지막 설정 불러오기, 없으면 기본값
  fpe::EvalConfig c;
  std::string err;
  if (!fpe::EvalConfig::load(LastConfigPath(), c, err) &&
      !fpe::EvalConfig::load(fpe::joinPath(fpe::exeDir(), L"last_config.json"), c, err)) {  // 이전 버전 위치
    // 첫 실행: 기본값. 결과 폴더는 쓰기 가능한 문서\FPEval\results
    std::wstring algo = fpe::joinPath(fpe::exeDir(), L"plugins\\x64\\FPAlgo.dll");
    if (fpe::fileExists(algo)) c.dllPath = algo;
    // 저장 폴더는 모두 쓰기 가능한 문서\FPEval 아래에 둔다
    const std::wstring data = fpe::userDataDir();
    c.outputDir = fpe::joinPath(data, L"results");
    c.templateDir = fpe::joinPath(data, L"templates");
    c.reportDir = fpe::joinPath(data, L"reports");
    c.logDir = fpe::joinPath(data, L"logs");
    c.dbPath = findDefaultDb();
    c.iType = guessIType(c.dbPath, c.iType);
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    c.workers = std::max(1, std::min<int>(4, (int)si.dwNumberOfProcessors));
  } else if (c.dbPath.empty()) {
    // 저장된 설정에 DB 폴더가 없으면(빈 값은 평가할 수 없음) 기본 DB로 채운다
    c.dbPath = findDefaultDb();
    c.iType = guessIType(c.dbPath, c.iType);
  }
  ApplyConfig(c);

  ShowPage(0);
  SetRunning(false);
  FitToScreen();
  ShowHome();
  return FALSE;  // 포커스는 ShowHome이 홈 화면에 두었다
}

CWnd* CMainDlg::Page(int index) {
  if (index == kDashPage) return &m_dash;
  if (index == kResultsPage) return &m_results;
  return m_pages[index];
}

CRect CMainDlg::PanelRect() {
  CRect rc;
  GetDlgItem(IDC_PAGE_AREA)->GetWindowRect(&rc);
  ScreenToClient(&rc);
  if (m_page >= kResultsPage) {
    CRect client, margin(0, 0, 0, 8);
    GetClientRect(&client);
    MapDialogRect(&margin);
    rc.bottom = client.bottom - margin.bottom;
  }
  return rc;
}

void CMainDlg::LayoutPages() {
  if (!m_dash.GetSafeHwnd()) return;
  if (m_home.GetSafeHwnd()) {
    CRect client;
    GetClientRect(&client);
    m_home.SetWindowPos(nullptr, 0, 0, client.Width(), client.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
  }
  CRect rc = PanelRect();
  rc.DeflateRect(1, 1);  // 패널 테두리 안쪽
  for (int i = 0; i < kPageCount; ++i) {
    Page(i)->SetWindowPos(nullptr, rc.left, rc.top, rc.Width(), rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
  }
}

namespace {
// 홈 화면을 띄우면 숨기고, 닫으면 다시 보이는 메인 창 컨트롤 (페이지는 ShowPage가 따로 다룬다)
const int kChromeIds[] = {IDC_NAV, IDC_PAGE_TITLE, IDC_PAGE_DESC};
const int kRunBarIds[] = {IDC_START, IDC_STOP, IDC_ABORT, IDC_RESUME, IDC_REPORT_ONLY,
                         IDC_OPEN_REPORT, IDC_OPEN_FOLDER, IDC_PROGRESS, IDC_STATUS};
}  // namespace

void CMainDlg::ShowHome() {
  m_homeShown = true;
  // 홈이 창 전체를 덮지만, 뒤의 컨트롤이 겹쳐 그려지거나 Tab으로 잡히지 않게 숨겨 둔다
  for (int id : kChromeIds) GetDlgItem(id)->ShowWindow(SW_HIDE);
  for (int id : kRunBarIds) GetDlgItem(id)->ShowWindow(SW_HIDE);
  for (int i = 0; i < kPageCount; ++i) Page(i)->ShowWindow(SW_HIDE);
  // 최근 실행은 홈을 열 때마다 기본 결과 폴더와 지금 설정된 결과 폴더에서 다시 읽는다
  m_home.SetRunRoots({fpe::joinPath(fpe::userDataDir(), L"results"), toW(m_target.OutDir())});
  LayoutPages();
  m_home.ShowWindow(SW_SHOW);
  m_home.SetFocus();
}

void CMainDlg::ShowPage(int index) {
  if (m_homeShown) {
    m_homeShown = false;
    m_home.ShowWindow(SW_HIDE);
    for (int id : kChromeIds) GetDlgItem(id)->ShowWindow(SW_SHOW);
    m_nav.SetFocus();
  }
  m_page = index;
  const bool library = index >= kResultsPage;
  const int visiblePage = library ? kResultsPage : index;
  if (index == kRunOptionsPage) m_run.ShowWorkerForDll(m_target.DllPath());
  for (int id : kRunBarIds) GetDlgItem(id)->ShowWindow(library ? SW_HIDE : SW_SHOW);
  for (int i = 0; i < kPageCount; ++i) Page(i)->ShowWindow(i == visiblePage ? SW_SHOW : SW_HIDE);
  if (library) {
    m_results.IncludeRoot(toW(m_target.OutDir()));
    using Mode = CResultsPage::Mode;
    if (index == kResumePage) {
      m_results.ShowMode(Mode::Resume, [this](const auto& path) {
        if (!m_running) StartRun(path);
      });
    } else if (index == kConfigPage) {
      m_results.ShowMode(Mode::Configs, [this](const auto& path) { ApplyConfigFile(path); });
    } else {
      m_results.ShowMode(Mode::Reports);
    }
  }
  SetDlgItemText(IDC_PAGE_TITLE, kPageTitles[index]);
  SetDlgItemText(IDC_PAGE_DESC, kPageDescs[index]);
  LayoutPages();
  Invalidate();
  m_nav.SetSelected(index);
  if (library) m_results.GetDlgItem(IDC_RESULTS_LIST)->SetFocus();
}

LRESULT CMainDlg::OnNav(WPARAM page, LPARAM) {
  if (page <= kConfigPage && (!m_running || (page != kResumePage && page != kConfigPage)))
    ShowPage(static_cast<int>(page));
  return 0;
}

LRESULT CMainDlg::OnNavHome(WPARAM, LPARAM) {
  ShowHome();
  return 0;
}

LRESULT CMainDlg::OnHomeCmd(WPARAM cmd, LPARAM arg) {
  switch (cmd) {
    case CHomeView::kCmdRunning:
      ShowPage(kDashPage);
      break;
    case CHomeView::kCmdNewTest:
      ShowPage(0);
      break;
    case CHomeView::kCmdResume:
      OnResume();
      break;
    case CHomeView::kCmdLoadConfig:
      OnLoadConfig();
      break;
    case CHomeView::kCmdResults:
      ShowPage(kResultsPage);
      break;
    case CHomeView::kCmdOpenRun:  // 최근 실행: 끝난 실행은 리포트, 아니면 이어하기 목록
      if (const RecentRun* run = m_home.RunAt(static_cast<int>(arg))) {
        if (run->complete && !run->htmlPath.empty()) {
          ShellExecute(m_hWnd, L"open", run->htmlPath.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
        } else if (m_running) {
          ShowPage(kDashPage);
        } else {
          ShowPage(run->complete ? kResultsPage : kResumePage);
        }
      }
      break;
    case CHomeView::kCmdAbout:
      OnAbout();
      break;
    default:
      break;
  }
  return 0;
}

// ------------------------------------------------------------ 모양·창 크기·DPI

void CMainDlg::DestroyIcons() {
  for (HICON* icon : {&m_navIcon, &m_bigIcon, &m_smallIcon}) {
    if (*icon) {
      DestroyIcon(*icon);
      *icon = nullptr;
    }
  }
}

void CMainDlg::CreateDpiResources() {
  // 현재 모니터 DPI에 맞는 크기로 아이콘을 다시 읽는다 (FPEval.ico에 16~256px가 들어 있음)
  const UINT dpi = GetDpiForWindow(m_hWnd);
  const HINSTANCE inst = AfxGetResourceHandle();
  auto loadIcon = [&](int size) {
    return static_cast<HICON>(LoadImage(inst, MAKEINTRESOURCE(IDR_MAINFRAME), IMAGE_ICON, size, size, LR_DEFAULTCOLOR));
  };
  HICON oldBig = m_bigIcon, oldSmall = m_smallIcon, oldNav = m_navIcon;
  m_bigIcon = loadIcon(GetSystemMetricsForDpi(SM_CXICON, dpi));
  m_smallIcon = loadIcon(GetSystemMetricsForDpi(SM_CXSMICON, dpi));
  m_navIcon = loadIcon(MulDiv(32, static_cast<int>(dpi), 96));
  SetIcon(m_bigIcon, TRUE);
  SetIcon(m_smallIcon, FALSE);
  m_nav.SetAppIcon(m_navIcon);
  for (HICON icon : {oldBig, oldSmall, oldNav}) {
    if (icon) DestroyIcon(icon);
  }
  theme::CreateUiFont(m_titleFont, 15, FW_BOLD, dpi);
  GetDlgItem(IDC_PAGE_TITLE)->SetFont(&m_titleFont);
}

void CMainDlg::OnSize(UINT type, int cx, int cy) {
  CDialogEx::OnSize(type, cx, cy);  // 동적 레이아웃은 이 핸들러보다 먼저 컨트롤을 옮겨 둔다
  LayoutPages();
  if (m_hWnd) Invalidate();
}

BOOL CMainDlg::OnEraseBkgnd(CDC* dc) {
  if (m_homeShown) return TRUE;  // 홈 화면이 클라이언트 전체를 그린다
  CDialogEx::OnEraseBkgnd(dc);  // 회색 창 배경 (SetBackgroundColor)
  if (GetDlgItem(IDC_PAGE_AREA)) {  // 페이지 뒤 흰 패널의 둥근 테두리
    CRect panel = PanelRect();
    const int radius = MulDiv(8, static_cast<int>(GetDpiForWindow(m_hWnd)), 96);
    theme::FillRoundRect(dc, panel, radius, theme::kPanel, theme::kBorder);
  }
  return TRUE;
}

HBRUSH CMainDlg::OnCtlColor(CDC* dc, CWnd* wnd, UINT ctlColor) {
  HBRUSH brush = CDialogEx::OnCtlColor(dc, wnd, ctlColor);
  switch (wnd->GetDlgCtrlID()) {
    case IDC_PAGE_TITLE:
      dc->SetTextColor(theme::kText);
      break;
    case IDC_PAGE_DESC:
    case IDC_STATUS:
      dc->SetTextColor(theme::kTextSecondary);
      break;
    default:
      break;
  }
  return brush;
}

void CMainDlg::OnGetMinMaxInfo(MINMAXINFO* mmi) {
  CDialogEx::OnGetMinMaxInfo(mmi);
  if (!m_nav.GetSafeHwnd()) return;
  CRect rc(CPoint(0, 0), MinClientSize(m_nav.IsCompact()));
  AdjustWindowRectExForDpi(&rc, GetStyle(), GetMenu() != nullptr, GetExStyle(), GetDpiForWindow(m_hWnd));
  mmi->ptMinTrackSize = CPoint(rc.Width(), rc.Height());
}

namespace {
// 사이드바 폭 (대화상자 단위). 펼친 폭은 FPEval.rc의 IDC_NAV 폭과 같다.
const int kNavFullDlu = 112, kNavCompactDlu = 40;
}  // namespace

CSize CMainDlg::MinClientSize(bool compact) {
  // 대화상자 템플릿 크기(580×296), 사이드바를 접으면 그 폭만큼 좁아진다. 그보다 작으면 설정 페이지 내용이 잘린다.
  const int kMinWidthDlu = 580, kMinHeightDlu = 296;
  CRect rc(0, 0, kMinWidthDlu - (compact ? kNavFullDlu - kNavCompactDlu : 0), kMinHeightDlu);
  MapDialogRect(&rc);
  return rc.Size();
}

void CMainDlg::SetupLayout() {
  // 리소스의 AFX_DIALOG_LAYOUT은 자식 창 순서로 컨트롤을 짝짓는다. 페이지(자식 대화상자)를 사이드바 뒤에 끼워 넣으므로
  // 다시 읽으면 어긋난다. 그래서 컨트롤 ID로 직접 등록하고, 사이드바 접기·DPI 변경 뒤에도 이 함수로 다시 만든다.
  using L = CMFCDynamicLayout;
  EnableDynamicLayout(TRUE);
  CMFCDynamicLayout* layout = GetDynamicLayout();
  layout->Create(this);
  layout->AddItem(IDC_NAV, L::MoveNone(), L::SizeVertical(100));
  layout->AddItem(IDC_PAGE_TITLE, L::MoveNone(), L::SizeHorizontal(100));
  layout->AddItem(IDC_PAGE_DESC, L::MoveNone(), L::SizeHorizontal(100));
  layout->AddItem(IDC_PAGE_AREA, L::MoveNone(), L::SizeHorizontalAndVertical(100, 100));
  for (int id : {IDC_START, IDC_STOP, IDC_ABORT, IDC_RESUME}) layout->AddItem(id, L::MoveVertical(100), L::SizeNone());
  for (int id : {IDC_REPORT_ONLY, IDC_OPEN_REPORT, IDC_OPEN_FOLDER}) {
    layout->AddItem(id, L::MoveHorizontalAndVertical(100, 100), L::SizeNone());
  }
  layout->AddItem(IDC_PROGRESS, L::MoveVertical(100), L::SizeHorizontal(100));
  layout->AddItem(IDC_STATUS, L::MoveVertical(100), L::SizeHorizontal(100));
  layout->SetMinSize(MinClientSize(m_nav.IsCompact()));
}

void CMainDlg::SetCompact(bool compact) {
  if (compact == m_nav.IsCompact()) return;
  CRect widths(0, 0, kNavFullDlu - kNavCompactDlu, 0);
  MapDialogRect(&widths);
  const int d = compact ? -widths.Width() : widths.Width();  // 오른쪽 컨트롤이 움직일 거리
  if (!compact && !IsZoomed()) {
    // 펼칠 자리가 없으면 창을 먼저 넓힌다 (레이아웃이 켜져 있으므로 오른쪽 컨트롤이 함께 늘어난다)
    CRect client, win;
    GetClientRect(&client);
    const int need = MinClientSize(false).cx - client.Width();
    if (need > 0) {
      GetWindowRect(&win);
      SetWindowPos(nullptr, 0, 0, win.Width() + need, win.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
    }
  }
  EnableDynamicLayout(FALSE);
  auto rectOf = [this](int id) {
    CRect rc;
    GetDlgItem(id)->GetWindowRect(&rc);
    ScreenToClient(&rc);
    return rc;
  };
  CRect nav = rectOf(IDC_NAV);
  m_nav.SetWindowPos(nullptr, 0, 0, nav.Width() + d, nav.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
  for (int id : {IDC_START, IDC_STOP, IDC_ABORT, IDC_RESUME}) {  // 옮기기만
    CRect rc = rectOf(id);
    GetDlgItem(id)->SetWindowPos(nullptr, rc.left + d, rc.top, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
  }
  for (int id : {IDC_PAGE_TITLE, IDC_PAGE_DESC, IDC_PAGE_AREA, IDC_PROGRESS, IDC_STATUS}) {  // 왼쪽 끝만 옮긴다
    CRect rc = rectOf(id);
    GetDlgItem(id)->SetWindowPos(nullptr, rc.left + d, rc.top, rc.Width() - d, rc.Height(), SWP_NOZORDER | SWP_NOACTIVATE);
  }
  m_nav.SetCompact(compact);
  SetupLayout();
  LayoutPages();
  // 컨트롤을 옮긴 자리에 옛 그림이 남지 않게 창 전체(자식 포함)를 다시 그린다
  RedrawWindow(nullptr, nullptr, RDW_INVALIDATE | RDW_ERASE | RDW_ALLCHILDREN | RDW_UPDATENOW);
}

LRESULT CMainDlg::OnNavToggle(WPARAM, LPARAM) {
  SetCompact(!m_nav.IsCompact());
  return 0;
}

void CMainDlg::FitToScreen() {
  // 1024×768 프로젝터처럼 작은 화면에서는 창이 화면 밖으로 나가지 않게 사이드바를 접고 화면 크기로 줄인다
  MONITORINFO mi{sizeof mi};
  if (!GetMonitorInfo(MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST), &mi)) return;
  const CRect work(mi.rcWork);
  CRect win;
  GetWindowRect(&win);
  if (win.Width() <= work.Width() && win.Height() <= work.Height()) return;
  if (win.Width() > work.Width()) SetCompact(true);
  GetWindowRect(&win);
  const int w = std::min(win.Width(), work.Width()), h = std::min(win.Height(), work.Height());
  SetWindowPos(nullptr, work.left + (work.Width() - w) / 2, work.top + (work.Height() - h) / 2, w, h,
               SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CMainDlg::OnDpiChanged(WPARAM, LPARAM) {
  // 다른 배율의 모니터로 옮겨지면 Windows가 컨트롤과 대화상자 글꼴을 새 DPI로 다시 배치한다(Per-Monitor V2).
  // 그동안 동적 레이아웃이 옛 기준으로 컨트롤을 옮기지 않게 끄고, 끝나면 새 위치를 기준으로 다시 켠다.
  // 대화상자를 만드는 도중(컨트롤 생성·OnInitDialog 전)에도 이 메시지가 올 수 있다. 그때는 기본 처리만 한다.
  if (!m_nav.GetSafeHwnd()) return Default();
  EnableDynamicLayout(FALSE);
  const LRESULT result = Default();
  SetupLayout();
  CreateDpiResources();
  LayoutPages();
  Invalidate();
  return result;
}

// ------------------------------------------------------------ 설정 ↔ 페이지

bool CMainDlg::GatherConfig(fpe::EvalConfig& c) {
  for (int i = 0; i < 4; ++i) {
    if (!m_pages[i]->SaveTo(c)) {  // DDV 검증 실패 → 해당 페이지로 이동
      ShowPage(i);
      return false;
    }
  }
  return true;
}

void CMainDlg::ApplyConfig(const fpe::EvalConfig& c) {
  for (auto* p : m_pages) p->LoadFrom(c);
  OnDbChanged(0, 0);
}

LRESULT CMainDlg::OnDbChanged(WPARAM, LPARAM) {
  CString path = m_target.DbPath();
  m_dbOk = false;
  if (path.IsEmpty()) {
    m_target.SetDbInfo(L"");
  } else {
    std::string err;
    m_dbOk = m_db.scan(toW(path), err);
    CString info;
    if (m_dbOk) {
      info.Format(L"이미지 %zu장 · 손가락 %zu개", m_db.images().size(), m_db.fingers().size());
      if (!m_db.warnings().empty()) {
        CString w;
        w.Format(L" · 제외된 파일 %zu개", m_db.warnings().size());
        info += w;
      }
    } else {
      info = toCS(err);
    }
    m_target.SetDbInfo(info);
  }
  return OnRulesChanged(0, 0);
}

LRESULT CMainDlg::OnRulesChanged(WPARAM, LPARAM) {
  if (!m_dbOk) {
    m_rules.SetEstimate(L"예상 시도 수 · 테스트 DB 폴더 선택 필요");
    return 0;
  }
  fpe::EvalConfig c;
  if (!m_rules.SaveTo(c)) return 0;
  fpe::EvalPlan p;
  std::vector<std::string> w;
  std::string err;
  if (!fpe::EvalPlan::build(c, m_db, p, w, err)) {
    m_rules.SetEstimate(L"예상 시도 수: " + toCS(err));
    return 0;
  }
  CString s;
  s.Format(L"예상 시도 수 — 등록 %zu · 비교용 추출 %zu · 본인비교 %zu · 타인비교 %zu\n", p.nEnroll, p.nAttempt,
           p.nGenuine, p.nImpostor);
  m_rules.SetEstimate(s + toCS(p.genuineDesc) + L" / " + toCS(p.impostorDesc));
  return 0;
}

// ------------------------------------------------------------ 실행

void CMainDlg::SetRunning(bool running) {
  m_running = running;
  const int idle[] = {IDC_START, IDC_RESUME, IDC_REPORT_ONLY};
  for (int id : idle) GetDlgItem(id)->EnableWindow(!running);
  m_nav.EnableAction(IDC_SAVE_CFG, !running);
  m_nav.EnablePage(kConfigPage, !running);
  m_nav.EnablePage(kResumePage, !running);
  GetDlgItem(IDC_STOP)->EnableWindow(running);
  GetDlgItem(IDC_ABORT)->EnableWindow(running);
  GetDlgItem(IDC_OPEN_REPORT)->EnableWindow(!running && !m_lastReport.IsEmpty());
  GetDlgItem(IDC_OPEN_FOLDER)->EnableWindow(!running && !m_lastRunDir.IsEmpty());
  for (auto* p : m_pages) p->EnableWindow(!running);  // 실행 중에는 설정 잠금
  m_home.SetRunning(running, L"");
}

void CMainDlg::SetStatus(const CString& s) { SetDlgItemText(IDC_STATUS, s); }

void CMainDlg::AppendLog(const CString& s) { m_dash.AppendLog(s); }

void CMainDlg::JoinWorker() {
  if (m_worker.joinable()) m_worker.join();
}

void CMainDlg::StartRun(const std::wstring& resumeDir) {
  fpe::EvalConfig cfg;
  if (!GatherConfig(cfg)) return;
  cfg.save(LastConfigPath());
  m_dash.Begin(false);
  ShowPage(kDashPage);  // 실행하면 실행 현황 화면으로 넘어간다
  m_lastReport.Empty();
  SetRunning(true);
  JoinWorker();
  m_bridge = std::make_unique<CProgressBridge>(m_hWnd);
  m_eval = std::make_unique<fpe::Evaluator>(cfg, *m_bridge);
  fpe::Evaluator* ev = m_eval.get();
  HWND hw = m_hWnd;
  m_worker = std::thread([ev, hw, resumeDir] {
    auto* o = new fpe::RunOutcome(ev->run(resumeDir));
    ::PostMessage(hw, WM_APP_DONE, 0, (LPARAM)o);
  });
}

void CMainDlg::OnStart() { StartRun(L""); }

void CMainDlg::OnStop() {
  if (m_eval) m_eval->requestStop();
  SetStatus(L"중지 요청 · 진행 중인 작업 마무리 중");
  m_dash.SetStatusText(L"중지 요청 · 진행 중인 작업 마무리 중");
}

void CMainDlg::OnAbort() {
  if (m_eval) m_eval->abort();
  SetStatus(L"강제 종료 중");
  m_dash.SetStatusText(L"강제 종료 중");
}

void CMainDlg::OnResume() {
  if (!m_running) ShowPage(kResumePage);
}

void CMainDlg::OnReportOnly() {
  CString p = BrowseFolder(this, m_target.ResultsBrowseFolder());
  if (p.IsEmpty()) return;
  fpe::EvalConfig cfg;
  if (!GatherConfig(cfg)) return;
  m_dash.Begin(true);
  ShowPage(kDashPage);
  SetRunning(true);
  JoinWorker();
  m_bridge = std::make_unique<CProgressBridge>(m_hWnd);
  CProgressBridge* br = m_bridge.get();
  HWND hw = m_hWnd;
  std::wstring dir = toW(p);
  m_worker = std::thread([br, hw, dir, cfg] {
    auto* o = new fpe::RunOutcome(fpe::Evaluator::reportOnly(dir, &cfg, *br));
    ::PostMessage(hw, WM_APP_DONE, 0, (LPARAM)o);
  });
}

void CMainDlg::OnSaveConfig() {
  fpe::EvalConfig c;
  if (!GatherConfig(c)) return;
  CString p = BrowseFile(this, L"설정 (*.json)|*.json||", existingBrowseFolder(fpe::userDataDir()), TRUE, L"json");
  if (p.IsEmpty()) return;
  if (c.save(toW(p))) {
    m_results.IncludeConfigRoot(std::filesystem::path(toW(p)).parent_path().native());
  } else {
    AfxMessageBox(L"설정 파일 저장 실패", MB_ICONERROR);
  }
}

void CMainDlg::OnLoadConfig() {
  if (!m_running) ShowPage(kConfigPage);
}

void CMainDlg::ApplyConfigFile(const std::wstring& path) {
  if (m_running) return;
  fpe::EvalConfig c;
  std::string err;
  if (!fpe::EvalConfig::load(path, c, err)) {
    AfxMessageBox(L"설정 불러오기 실패: " + toCS(err), MB_ICONERROR);
    m_results.Refresh();
    return;
  }
  ApplyConfig(c);
  ShowPage(0);
}

void CMainDlg::OnOpenReport() {
  if (!m_lastReport.IsEmpty()) ShellExecute(m_hWnd, L"open", m_lastReport, nullptr, nullptr, SW_SHOWNORMAL);
}

void CMainDlg::OnOpenFolder() {
  if (!m_lastRunDir.IsEmpty()) ShellExecute(m_hWnd, L"open", m_lastRunDir, nullptr, nullptr, SW_SHOWNORMAL);
}

void CMainDlg::OnAbout() {
  CString text;
  text.Format(L"FPEval %s\n지문인식 알고리즘 평가 프로그램\n\n"
              L"평가 대상 DLL · K-NBTC 규격 준수(KISA_Enroll / KISA_Attempt / KISA_Compare)\n"
              L"별도 Worker 프로세스에서 실행\n\nSME3007 고급객체지향프로그래밍 팀 프로젝트",
              theme::kAppVersion);
  MessageBox(text, L"FPEval 정보", MB_OK | MB_ICONINFORMATION);
}

void CMainDlg::OnClose() {
  if (m_running) {
    if (AfxMessageBox(L"평가 진행 중\n\n현재 평가를 강제 종료하고 프로그램 닫기", MB_YESNO | MB_ICONQUESTION) != IDYES)
      return;
    if (m_eval) m_eval->abort();
    JoinWorker();
    // 작업 스레드가 남긴 WM_APP_* 메시지의 객체 정리
    MSG msg;
    while (PeekMessage(&msg, m_hWnd, WM_APP_PHASE, WM_APP_DONE, PM_REMOVE)) {
      if (msg.message == WM_APP_PROGRESS) {
        delete reinterpret_cast<CProgressBridge::Progress*>(msg.lParam);
      } else if (msg.message == WM_APP_DONE) {
        delete reinterpret_cast<fpe::RunOutcome*>(msg.lParam);
      } else {
        delete reinterpret_cast<std::string*>(msg.lParam);
      }
    }
  }
  fpe::EvalConfig c;
  if (GatherConfig(c)) c.save(LastConfigPath());
  EndDialog(IDCANCEL);
}

// ------------------------------------------------------------ 작업 스레드 → UI 메시지

LRESULT CMainDlg::OnPhase(WPARAM, LPARAM l) {
  std::unique_ptr<std::string> s(reinterpret_cast<std::string*>(l));
  m_phase = toCS(*s);
  m_phaseStarted = std::chrono::steady_clock::now();
  SetStatus(m_phase);
  m_dash.SetPhase(m_phase);
  AppendLog(L"── " + m_phase);
  m_progress.SetPos(0);
  return 0;
}

LRESULT CMainDlg::OnProgress(WPARAM, LPARAM l) {
  std::unique_ptr<CProgressBridge::Progress> p(reinterpret_cast<CProgressBridge::Progress*>(l));
  m_progress.SetPos(p->total ? (int)(p->done * 1000 / p->total) : 1000);
  CString s;
  s.Format(L"%s  |  %zu / %zu  (실패 %zu)", m_phase.GetString(), p->done, p->total, p->failures);
  const std::chrono::duration<double> elapsed = std::chrono::steady_clock::now() - m_phaseStarted;
  if (p->done > 0 && p->done < p->total && elapsed.count() >= 1.0) {
    const double seconds = std::ceil(elapsed.count() * static_cast<double>(p->total - p->done) /
                                     static_cast<double>(p->done));
    CString eta;
    eta.Format(L"  |  단계 남은 시간 약 %.0f초", seconds);
    s += eta;
  }
  SetStatus(s);
  m_dash.SetProgress(p->done, p->total, p->failures);
  CString badge;
  badge.Format(L"%d%%", m_dash.Percent());
  m_nav.SetBadge(kDashPage, badge);
  m_home.SetRunning(true, badge);
  return 0;
}

LRESULT CMainDlg::OnLog(WPARAM, LPARAM l) {
  std::unique_ptr<std::string> s(reinterpret_cast<std::string*>(l));
  AppendLog(toCS(*s));
  return 0;
}

LRESULT CMainDlg::OnDone(WPARAM, LPARAM l) {
  std::unique_ptr<fpe::RunOutcome> o(reinterpret_cast<fpe::RunOutcome*>(l));
  JoinWorker();
  m_eval.reset();
  m_lastRunDir = toCS(o->runDir);
  m_lastReport = toCS(!o->htmlPath.empty() ? o->htmlPath : o->docxPath);
  CString msg = toCS(o->message);
  if (o->ok) {
    auto pct = [](double v) {
      CString s;
      if (fpe::isNA(v)) {
        s = L"N/A";
      } else {
        s.Format(L"%.2f%%", v * 100);
      }
      return s;
    };
    msg = toCS(o->message) + L" — EER " + pct(o->metrics.eer) + L", FTE " + pct(o->metrics.fte);
    m_progress.SetPos(1000);
  }
  SetStatus(msg);
  m_dash.Finish(*o);
  m_nav.SetBadge(kDashPage, L"");
  AppendLog(L"■ " + msg);
  if (!o->runDir.empty()) AppendLog(L"결과 폴더: " + m_lastRunDir);
  if (!o->htmlPath.empty()) AppendLog(L"HTML 리포트: " + toCS(o->htmlPath));
  if (!o->docxPath.empty()) AppendLog(L"Word 리포트: " + toCS(o->docxPath));
  if (!o->logPath.empty()) AppendLog(L"로그 파일: " + toCS(o->logPath));
  SetRunning(false);
  if (!o->runDir.empty()) m_results.IncludeRoot(std::filesystem::path(o->runDir).parent_path().native());
  if (!m_homeShown && m_page == kResultsPage) m_results.Refresh();
  if (m_homeShown) m_home.ReloadRuns();
  return 0;
}
