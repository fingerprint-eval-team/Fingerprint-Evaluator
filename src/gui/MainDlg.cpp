#include "pch.h"

#include "MainDlg.h"

#include <algorithm>

#include "plan/EvalPlan.h"

BEGIN_MESSAGE_MAP(CMainDlg, CDialogEx)
  ON_NOTIFY(TCN_SELCHANGE, IDC_TAB, &CMainDlg::OnTabChange)
  ON_BN_CLICKED(IDC_START, &CMainDlg::OnStart)
  ON_BN_CLICKED(IDC_STOP, &CMainDlg::OnStop)
  ON_BN_CLICKED(IDC_ABORT, &CMainDlg::OnAbort)
  ON_BN_CLICKED(IDC_RESUME, &CMainDlg::OnResume)
  ON_BN_CLICKED(IDC_REPORT_ONLY, &CMainDlg::OnReportOnly)
  ON_BN_CLICKED(IDC_SAVE_CFG, &CMainDlg::OnSaveConfig)
  ON_BN_CLICKED(IDC_LOAD_CFG, &CMainDlg::OnLoadConfig)
  ON_BN_CLICKED(IDC_OPEN_REPORT, &CMainDlg::OnOpenReport)
  ON_BN_CLICKED(IDC_OPEN_FOLDER, &CMainDlg::OnOpenFolder)
  ON_WM_CLOSE()
  ON_MESSAGE(WM_APP_PHASE, &CMainDlg::OnPhase)
  ON_MESSAGE(WM_APP_PROGRESS, &CMainDlg::OnProgress)
  ON_MESSAGE(WM_APP_LOG, &CMainDlg::OnLog)
  ON_MESSAGE(WM_APP_DONE, &CMainDlg::OnDone)
  ON_MESSAGE(WM_APP_DB_CHANGED, &CMainDlg::OnDbChanged)
  ON_MESSAGE(WM_APP_RULES_CHANGED, &CMainDlg::OnRulesChanged)
END_MESSAGE_MAP()

void CMainDlg::DoDataExchange(CDataExchange* pDX) {
  CDialogEx::DoDataExchange(pDX);
  DDX_Control(pDX, IDC_TAB, m_tab);
  DDX_Control(pDX, IDC_PROGRESS, m_progress);
  DDX_Control(pDX, IDC_LOG, m_log);
}

// 설정 파일은 문서\FPEval에 둔다 (설치판의 Program Files는 쓰기 불가)
std::wstring CMainDlg::LastConfigPath() const { return fpe::joinPath(fpe::userDataDir(), L"last_config.json"); }

// ------------------------------------------------------------ 초기화

BOOL CMainDlg::OnInitDialog() {
  CDialogEx::OnInitDialog();
  SetIcon(AfxGetApp()->LoadStandardIcon(IDI_APPLICATION), TRUE);

  const wchar_t* names[] = {L"1. 대상", L"2. 시험 규칙", L"3. 실행 옵션", L"4. 리포트"};
  for (int i = 0; i < 4; ++i) m_tab.InsertItem(i, names[i]);

  // 탭 페이지 생성: 메인 대화상자의 자식으로 만들고 탭 표시 영역에 겹쳐 배치
  const UINT ids[] = {IDD_PAGE_TARGET, IDD_PAGE_RULES, IDD_PAGE_RUN, IDD_PAGE_REPORT};
  CRect rc;
  m_tab.GetWindowRect(&rc);
  ScreenToClient(&rc);
  m_tab.AdjustRect(FALSE, &rc);
  for (int i = 0; i < 4; ++i) {
    m_pages[i]->Create(ids[i], this);
    m_pages[i]->SetWindowPos(&wndTop, rc.left, rc.top, rc.Width(), rc.Height(), SWP_HIDEWINDOW);
  }
  m_progress.SetRange32(0, 1000);
  m_log.SetHorizontalExtent(2000);

  // 마지막 설정 불러오기, 없으면 기본값
  fpe::EvalConfig c;
  std::string err;
  if (!fpe::EvalConfig::load(LastConfigPath(), c, err) &&
      !fpe::EvalConfig::load(fpe::joinPath(fpe::exeDir(), L"last_config.json"), c, err)) {  // 이전 버전 위치
    // 첫 실행: 기본값. 결과 폴더는 쓰기 가능한 문서\FPEval\results
    std::wstring algo = fpe::joinPath(fpe::exeDir(), L"plugins\\x64\\FPAlgo.dll");
    if (fpe::fileExists(algo)) c.dllPath = algo;
    c.outputDir = fpe::joinPath(fpe::userDataDir(), L"results");
    SYSTEM_INFO si;
    GetSystemInfo(&si);
    c.workers = std::max(1, std::min<int>(4, (int)si.dwNumberOfProcessors));
  }
  if (c.items.empty())
    for (auto& s : fpe::allReportItems()) c.items.insert(s);
  ApplyConfig(c);

  ShowPage(0);
  SetRunning(false);
  return TRUE;
}

void CMainDlg::ShowPage(int index) {
  for (int i = 0; i < 4; ++i) m_pages[i]->ShowWindow(i == index ? SW_SHOW : SW_HIDE);
}

void CMainDlg::OnTabChange(NMHDR*, LRESULT* result) {
  ShowPage(m_tab.GetCurSel());
  *result = 0;
}

// ------------------------------------------------------------ 설정 ↔ 페이지

bool CMainDlg::GatherConfig(fpe::EvalConfig& c) {
  for (int i = 0; i < 4; ++i) {
    if (!m_pages[i]->SaveTo(c)) {  // DDV 검증 실패 → 해당 탭으로 이동
      m_tab.SetCurSel(i);
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
    m_rules.SetEstimate(L"예상 시도 수: DB 폴더를 먼저 선택하세요");
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
  const int idle[] = {IDC_START, IDC_RESUME, IDC_REPORT_ONLY, IDC_SAVE_CFG, IDC_LOAD_CFG};
  for (int id : idle) GetDlgItem(id)->EnableWindow(!running);
  GetDlgItem(IDC_STOP)->EnableWindow(running);
  GetDlgItem(IDC_ABORT)->EnableWindow(running);
  GetDlgItem(IDC_OPEN_REPORT)->EnableWindow(!running && !m_lastHtml.IsEmpty());
  GetDlgItem(IDC_OPEN_FOLDER)->EnableWindow(!running && !m_lastRunDir.IsEmpty());
  for (auto* p : m_pages) p->EnableWindow(!running);  // 실행 중에는 설정 잠금
}

void CMainDlg::AppendLog(const CString& s) {
  int n = m_log.AddString(s);
  if (n > 5000) m_log.DeleteString(0);
  m_log.SetTopIndex(m_log.GetCount() - 1);
}

void CMainDlg::JoinWorker() {
  if (m_worker.joinable()) m_worker.join();
}

void CMainDlg::StartRun(const std::wstring& resumeDir) {
  fpe::EvalConfig cfg;
  if (!GatherConfig(cfg)) return;
  cfg.save(LastConfigPath());
  m_log.ResetContent();
  m_lastHtml.Empty();
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
  SetDlgItemText(IDC_STATUS, L"중지 요청됨 — 진행 중인 작업을 마치는 중");
}

void CMainDlg::OnAbort() {
  if (m_eval) m_eval->abort();
  SetDlgItemText(IDC_STATUS, L"강제 종료 중");
}

void CMainDlg::OnResume() {
  CString p = BrowseFolder(this);
  if (!p.IsEmpty()) StartRun(toW(p));
}

void CMainDlg::OnReportOnly() {
  CString p = BrowseFolder(this);
  if (p.IsEmpty()) return;
  fpe::EvalConfig cfg;
  if (!GatherConfig(cfg)) return;
  m_log.ResetContent();
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
  CString p = BrowseFile(this, L"설정 (*.json)|*.json||", TRUE, L"json");
  if (!p.IsEmpty() && !c.save(toW(p))) AfxMessageBox(L"설정을 저장할 수 없습니다.", MB_ICONERROR);
}

void CMainDlg::OnLoadConfig() {
  CString p = BrowseFile(this, L"설정 (*.json)|*.json||");
  if (p.IsEmpty()) return;
  fpe::EvalConfig c;
  std::string err;
  if (fpe::EvalConfig::load(toW(p), c, err)) {
    if (c.items.empty())
      for (auto& s : fpe::allReportItems()) c.items.insert(s);
    ApplyConfig(c);
  } else {
    AfxMessageBox(L"설정 불러오기 실패: " + toCS(err), MB_ICONERROR);
  }
}

void CMainDlg::OnOpenReport() {
  if (!m_lastHtml.IsEmpty()) ShellExecute(m_hWnd, L"open", m_lastHtml, nullptr, nullptr, SW_SHOWNORMAL);
}

void CMainDlg::OnOpenFolder() {
  if (!m_lastRunDir.IsEmpty()) ShellExecute(m_hWnd, L"open", m_lastRunDir, nullptr, nullptr, SW_SHOWNORMAL);
}

void CMainDlg::OnClose() {
  if (m_running) {
    if (AfxMessageBox(L"평가가 진행 중입니다. 강제 종료하고 닫을까요?", MB_YESNO | MB_ICONQUESTION) != IDYES) return;
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
  SetDlgItemText(IDC_STATUS, m_phase);
  AppendLog(L"── " + m_phase);
  m_progress.SetPos(0);
  return 0;
}

LRESULT CMainDlg::OnProgress(WPARAM, LPARAM l) {
  std::unique_ptr<CProgressBridge::Progress> p(reinterpret_cast<CProgressBridge::Progress*>(l));
  m_progress.SetPos(p->total ? (int)(p->done * 1000 / p->total) : 1000);
  CString s;
  s.Format(L"%s  |  %zu / %zu  (실패 %zu)", m_phase.GetString(), p->done, p->total, p->failures);
  SetDlgItemText(IDC_STATUS, s);
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
  m_lastHtml = toCS(o->htmlPath);
  CString msg = toCS(o->message);
  if (o->ok) {
    msg.Format(L"완료 — EER %.2f%%, FTE %.2f%%", o->metrics.eer * 100, o->metrics.fte * 100);
    m_progress.SetPos(1000);
  }
  SetDlgItemText(IDC_STATUS, msg);
  AppendLog(L"■ " + msg);
  if (!o->runDir.empty()) AppendLog(L"결과 폴더: " + m_lastRunDir);
  SetRunning(false);
  return 0;
}
