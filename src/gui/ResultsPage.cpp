#include "pch.h"

#include "ResultsPage.h"

#include <algorithm>
#include <sstream>

#include "result/Csv.h"

namespace {
namespace fs = std::filesystem;

bool samePath(const std::wstring& a, const std::wstring& b) {
  return CompareStringOrdinal(a.c_str(), -1, b.c_str(), -1, TRUE) == CSTR_EQUAL;
}

void includeRoot(std::vector<std::wstring>& roots, const std::wstring& path) {
  if (path.empty()) return;
  std::error_code error;
  auto root = fs::absolute(path, error).lexically_normal();
  if (error) return;
  if (!root.has_filename() && root.has_relative_path()) root = root.parent_path();
  if (std::none_of(roots.begin(), roots.end(), [&](const auto& saved) { return samePath(saved, root.native()); }))
    roots.push_back(root.native());
}

std::wstring findReportFile(const fs::path& reportDir, const fs::path& runDir, const wchar_t* name) {
  const auto configured = (reportDir / name).native();
  if (fpe::fileExists(configured)) return configured;
  const auto local = (runDir / name).native();
  return fpe::fileExists(local) ? local : std::wstring();
}

std::string readText(const fs::path& path) {
  std::vector<uint8_t> bytes;
  if (!fpe::readFile(path.native(), bytes)) return {};
  return std::string(bytes.begin(), bytes.end());
}

bool isTimestamp(const CString& stamp) {
  if (stamp.GetLength() != 15 || stamp[8] != L'_') return false;
  for (int i = 0; i < stamp.GetLength(); ++i) {
    if (i != 8 && (stamp[i] < L'0' || stamp[i] > L'9')) return false;
  }
  return true;
}

CString formatTimestamp(CString stamp) {
  if (!isTimestamp(stamp)) return stamp;
  return stamp.Left(4) + L"-" + stamp.Mid(4, 2) + L"-" + stamp.Mid(6, 2) + L" " +
         stamp.Mid(9, 2) + L":" + stamp.Mid(11, 2) + L":" + stamp.Mid(13, 2);
}

void appendDetail(CString& details, const wchar_t* label, const CString& value) {
  if (!value.IsEmpty()) details += CString(label) + L"\r\n" + value + L"\r\n\r\n";
}

bool appendSummary(CString& details, const fs::path& path, CString& status) {
  std::istringstream input(readText(path));
  std::string header, row;
  if (!std::getline(input, header) || !std::getline(input, row)) {
    status = L"완료 기록 없음";
    return false;
  }
  const auto keys = fpe::parseCsvLine(header);
  const auto values = fpe::parseCsvLine(row);
  auto field = [&](const char* key) {
    const auto index = static_cast<size_t>(std::find(keys.begin(), keys.end(), key) - keys.begin());
    return index < keys.size() && index < values.size() ? toCS(values[index]) : CString();
  };
  const auto incomplete = field("incomplete_jobs");
  if (!incomplete.IsEmpty()) {
    status = incomplete == L"0" ? CString(L"완료") : L"미완료 " + incomplete + L"건";
    appendDetail(details, L"평가 상태", status);
  } else {
    status = L"완료 기록 없음";
  }
  const auto eer = field("eer_percent");
  if (!eer.IsEmpty()) appendDetail(details, L"EER", eer + L"%");
  return incomplete == L"0";
}

CString modifiedTime(const std::wstring& path) {
  WIN32_FILE_ATTRIBUTE_DATA info{};
  SYSTEMTIME utc{}, local{};
  if (!GetFileAttributesExW(path.c_str(), GetFileExInfoStandard, &info) ||
      !FileTimeToSystemTime(&info.ftLastWriteTime, &utc) ||
      !SystemTimeToTzSpecificLocalTime(nullptr, &utc, &local)) return {};
  CString time;
  time.Format(L"%04d-%02d-%02d %02d:%02d:%02d", local.wYear, local.wMonth, local.wDay,
              local.wHour, local.wMinute, local.wSecond);
  return time;
}
}  // namespace

BEGIN_MESSAGE_MAP(CResultsPage, CPageBase)
  ON_WM_SIZE()
  ON_WM_PAINT()
  ON_WM_CTLCOLOR()
  ON_BN_CLICKED(IDC_RESULTS_ADD, &CResultsPage::OnAddRoot)
  ON_BN_CLICKED(IDC_RESULTS_REFRESH, &CResultsPage::Refresh)
  ON_BN_CLICKED(IDC_RESULTS_HTML, &CResultsPage::OnOpenHtml)
  ON_BN_CLICKED(IDC_RESULTS_DOCX, &CResultsPage::OnOpenDocx)
  ON_BN_CLICKED(IDC_RESULTS_SELECT, &CResultsPage::OnSelect)
  ON_BN_CLICKED(IDC_RESULTS_FILE, &CResultsPage::OnBrowseConfig)
  ON_BN_CLICKED(IDC_RESULTS_COMPLETED, &CResultsPage::Refresh)
  ON_NOTIFY(LVN_ITEMCHANGED, IDC_RESULTS_LIST, &CResultsPage::OnSelectionChanged)
  ON_NOTIFY(LVN_ITEMACTIVATE, IDC_RESULTS_LIST, &CResultsPage::OnListActivate)
  ON_NOTIFY(NM_CUSTOMDRAW, IDC_RESULTS_LIST, &CResultsPage::OnListDraw)
END_MESSAGE_MAP()

BOOL CResultsPage::OnInitDialog() {
  CPageBase::OnInitDialog();
  m_list.SubclassDlgItem(IDC_RESULTS_LIST, this);
  m_detailBg.CreateSolidBrush(theme::kCard);
  m_list.SetExtendedStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_LABELTIP);
  m_list.InsertColumn(0, L"실행 일시");
  m_list.InsertColumn(1, L"DLL");
  m_list.InsertColumn(2, L"테스트 DB");
  UpdateSelection();
  return TRUE;
}

void CResultsPage::OnSize(UINT type, int width, int height) {
  CPageBase::OnSize(type, width, height);
  if (!m_list.GetSafeHwnd()) return;
  LayoutContent(width, height);
}

void CResultsPage::LayoutContent(int width, int height) {
  CRect units(0, 0, 10, 10);
  MapDialogRect(&units);
  const int margin = units.Width(), gap = units.Height();
  const int top = 3 * gap;
  const int bottom = std::max(top, height - 3 * gap);
  const bool resume = m_mode == Mode::Resume;
  const int listWidth = std::max(0, resume ? width - 2 * margin : (width - margin * 3) * 3 / 5);
  const int detailX = 2 * margin + listWidth;
  const int detailWidth = std::max(0, width - detailX - margin);
  const int listBottom = resume ? std::max(top, bottom - 5 * gap) : bottom;
  m_listPanel.SetRect(margin, top, margin + listWidth, listBottom);
  if (resume) {
    m_detailPanel.SetRect(margin, listBottom + 2 * gap, margin + listWidth, bottom);
    GetDlgItem(IDC_RESULTS_HEADING)->MoveWindow(margin, listBottom + gap / 2, listWidth, gap);
  } else {
    m_detailPanel.SetRect(detailX, top, detailX + detailWidth, bottom);
    GetDlgItem(IDC_RESULTS_HEADING)->MoveWindow(detailX + 2, gap, std::max(0, detailWidth - 2), gap);
  }
  const int inset = std::max(2, MulDiv(7, static_cast<int>(GetDpiForWindow(m_hWnd)), 96));
  CRect list = m_listPanel, detail = m_detailPanel;
  list.DeflateRect(inset, inset);
  detail.DeflateRect(inset + 2, inset);
  m_list.MoveWindow(list);
  GetDlgItem(IDC_RESULTS_DETAIL)->MoveWindow(detail);
  const int buttonWidth = 7 * margin;
  const int docxX = width - margin - buttonWidth;
  const int htmlX = docxX - margin / 2 - buttonWidth;
  GetDlgItem(IDC_RESULTS_DOCX)->MoveWindow(docxX, bottom + gap / 2, buttonWidth, 2 * gap);
  GetDlgItem(IDC_RESULTS_HTML)->MoveWindow(htmlX, bottom + gap / 2, buttonWidth, 2 * gap);
  GetDlgItem(IDC_RESULTS_SELECT)->MoveWindow(docxX, bottom + gap / 2, buttonWidth, 2 * gap);
  const int actionX = m_mode == Mode::Reports ? htmlX : docxX;
  GetDlgItem(IDC_RESULTS_STATUS)->MoveWindow(margin, bottom + gap, std::max(0, actionX - 2 * margin), gap);
  SizeColumns();
  Invalidate();
}

void CResultsPage::OnPaint() {
  CPaintDC dc(this);
  const int radius = MulDiv(14, static_cast<int>(GetDpiForWindow(m_hWnd)), 96);
  theme::FillRoundRect(&dc, m_listPanel, radius, theme::kPanel, theme::kBorder);
  theme::FillRoundRect(&dc, m_detailPanel, radius, theme::kCard, theme::kBorder);
}

HBRUSH CResultsPage::OnCtlColor(CDC* dc, CWnd* wnd, UINT type) {
  if (wnd->GetDlgCtrlID() == IDC_RESULTS_DETAIL && m_detailBg.GetSafeHandle()) {
    dc->SetBkColor(theme::kCard);
    dc->SetTextColor(theme::kText);
    return static_cast<HBRUSH>(m_detailBg.GetSafeHandle());
  }
  return CPageBase::OnCtlColor(dc, wnd, type);
}

void CResultsPage::SizeColumns() {
  CRect listClient;
  m_list.GetClientRect(&listClient);
  const int usable = std::max(0, listClient.Width() - GetSystemMetricsForDpi(SM_CXVSCROLL, GetDpiForWindow(m_hWnd)));
  if (m_mode == Mode::Resume) {
    m_list.SetColumnWidth(0, usable * 28 / 100);
    m_list.SetColumnWidth(1, usable * 22 / 100);
    m_list.SetColumnWidth(2, usable * 26 / 100);
    m_list.SetColumnWidth(3, usable - m_list.GetColumnWidth(0) - m_list.GetColumnWidth(1) - m_list.GetColumnWidth(2));
  } else {
    m_list.SetColumnWidth(0, usable * (m_mode == Mode::Configs ? 36 : 41) / 100);
    m_list.SetColumnWidth(1, usable * (m_mode == Mode::Configs ? 36 : 26) / 100);
    m_list.SetColumnWidth(2, usable - m_list.GetColumnWidth(0) - m_list.GetColumnWidth(1));
  }
}

void CResultsPage::IncludeRoot(const std::wstring& path) {
  includeRoot(m_roots, path);
}

void CResultsPage::IncludeConfigRoot(const std::wstring& path) {
  includeRoot(m_configRoots, path);
}

void CResultsPage::ShowMode(Mode mode, std::function<void(const std::wstring&)> choose) {
  if (m_mode == Mode::Resume && mode != Mode::Resume) m_list.DeleteColumn(3);
  if (m_mode != Mode::Resume && mode == Mode::Resume) m_list.InsertColumn(3, L"상태");
  m_mode = mode;
  m_choose = std::move(choose);
  const bool reports = mode == Mode::Reports, configs = mode == Mode::Configs;
  const auto column = [&](int index, const wchar_t* label) {
    LVCOLUMN value{};
    value.mask = LVCF_TEXT;
    value.pszText = const_cast<LPWSTR>(label);
    m_list.SetColumn(index, &value);
  };
  column(0, configs ? L"설정 파일" : L"실행 일시");
  column(1, configs ? L"수정 일시" : L"DLL");
  column(2, configs ? L"DLL" : L"테스트 DB");
  SetDlgItemText(IDC_RESULTS_HEADING, reports ? L"선택한 리포트" : configs ? L"선택한 설정" : L"재개할 실행");
  SetDlgItemText(IDC_RESULTS_SELECT, configs ? L"설정 적용(&S)" : L"이어서 하기(&S)");
  GetDlgItem(IDC_RESULTS_HTML)->ShowWindow(reports ? SW_SHOW : SW_HIDE);
  GetDlgItem(IDC_RESULTS_DOCX)->ShowWindow(reports ? SW_SHOW : SW_HIDE);
  GetDlgItem(IDC_RESULTS_SELECT)->ShowWindow(reports ? SW_HIDE : SW_SHOW);
  GetDlgItem(IDC_RESULTS_FILE)->ShowWindow(configs ? SW_SHOW : SW_HIDE);
  GetDlgItem(IDC_RESULTS_COMPLETED)->ShowWindow(mode == Mode::Resume ? SW_SHOW : SW_HIDE);
  CRect rc;
  GetClientRect(&rc);
  LayoutContent(rc.Width(), rc.Height());
  Refresh();
}

bool CResultsPage::ReadRun(const std::wstring& path, Entry& report) const {
  const fs::path folder(path);
  fpe::EvalConfig config;
  std::string error;
  const bool hasConfig = fpe::EvalConfig::load((folder / L"config.json").native(), config, error);
  fpe::Json info;
  const bool hasInfo = fpe::Json::parse(readText(folder / L"run_info.json"), info, error);
  const auto name = folder.filename().native();
  if (m_mode == Mode::Resume) {
    if (!hasConfig || !hasInfo || !fpe::fileExists((folder / L"plan.csv").native())) return false;
  } else {
    if (!hasInfo && !hasConfig) return false;
    std::error_code pathError;
    const auto configuredDir = config.reportDir.empty() ? folder : fs::path(config.reportDir) / name;
    const auto reportDir = fs::absolute(configuredDir, pathError).lexically_normal();
    if (pathError) return false;
    report.htmlPath = findReportFile(reportDir, folder, L"report.html");
    report.docxPath = findReportFile(reportDir, folder, L"report.docx");
    if (report.htmlPath.empty() && report.docxPath.empty()) return false;
  }
  report.path = path;
  CString stamp = toCS(info.get("startedAt").asString());
  if (stamp.IsEmpty() && name.size() >= 15 && isTimestamp(toCS(name.substr(name.size() - 15))))
    stamp = toCS(name.substr(name.size() - 15));
  report.date = formatTimestamp(stamp);
  report.dll = toCS(info.get("dllName").asString(fpe::toUtf8(fpe::fileNameOf(config.dllPath))));
  report.db = toCS(info.get("dbName").asString(fpe::toUtf8(fpe::fileNameOf(config.dbPath))));
  CString dllInfo = report.dll;
  const int bits = info.get("dllBits").asInt();
  if (bits == 32 || bits == 64) {
    CString architecture;
    architecture.Format(L" · %d비트", bits);
    dllInfo += architecture;
  }
  appendDetail(report.details, L"실행 일시", report.date);
  appendDetail(report.details, L"알고리즘 DLL", dllInfo);
  appendDetail(report.details, L"테스트 DB", report.db);
  report.complete = appendSummary(report.details, folder / L"summary.csv", report.status);
  if (hasConfig) {
    CString options;
    options.Format(L"iType %d · seed %llu · 동시 수행 %d", config.iType,
                   static_cast<unsigned long long>(config.seed), config.workers);
    appendDetail(report.details, L"실행 설정", options);
    report.resumeDetails = report.status + L"  ·  " + options + L"\r\n" + toCS(path);
  }
  appendDetail(report.details, L"DLL 경로", toCS(config.dllPath));
  appendDetail(report.details, L"실행 폴더", toCS(folder.native()));
  appendDetail(report.details, L"HTML 리포트", toCS(report.htmlPath));
  appendDetail(report.details, L"Word 리포트", toCS(report.docxPath));
  return true;
}

bool CResultsPage::ReadConfig(const std::wstring& path, Entry& entry) const {
  fpe::Json json;
  std::string error;
  const fs::path file(path);
  if (!fpe::Json::parse(readText(file), json, error) || json.type() != fpe::Json::Type::Object ||
      (!json.has("dllPath") && !json.has("db") && !json.has("rules") && !json.has("run") && !json.has("report")))
    return false;
  const auto config = fpe::EvalConfig::fromJson(json);
  entry.path = path;
  entry.name = toCS(file.filename().native());
  entry.date = modifiedTime(path);
  entry.dll = toCS(fpe::fileNameOf(config.dllPath));
  appendDetail(entry.details, L"설정 파일", entry.name);
  appendDetail(entry.details, L"수정 일시", entry.date);
  appendDetail(entry.details, L"알고리즘 DLL", toCS(config.dllPath));
  appendDetail(entry.details, L"테스트 DB", toCS(config.dbPath));
  appendDetail(entry.details, L"결과 폴더", toCS(config.outputDir));
  appendDetail(entry.details, L"파일 경로", toCS(path));
  return true;
}

void CResultsPage::Refresh() {
  const auto* selected = SelectedEntry();
  const auto previous = selected ? selected->path : std::wstring();
  m_list.SetRedraw(FALSE);
  m_list.DeleteAllItems();
  m_entries.clear();

  const bool configs = m_mode == Mode::Configs;
  const bool resume = m_mode == Mode::Resume;
  const bool includeCompleted = resume && IsDlgButtonChecked(IDC_RESULTS_COMPLETED) == BST_CHECKED;
  const auto& roots = configs ? m_configRoots : m_roots;
  auto collect = [&](const fs::path& candidate) {
    const auto path = candidate.native();
    if (std::any_of(m_entries.begin(), m_entries.end(), [&](const auto& entry) { return samePath(path, entry.path); }))
      return;
    Entry entry;
    if ((configs ? ReadConfig(path, entry) : ReadRun(path, entry)) && (!resume || !entry.complete || includeCompleted))
      m_entries.push_back(std::move(entry));
  };
  for (const auto& root : roots) {
    if (!configs) collect(root);
    std::error_code error;
    for (fs::directory_iterator it(root, error), end; !error && it != end; it.increment(error)) {
      if (configs) {
        if (it->is_regular_file(error) && samePath(it->path().extension().native(), L".json")) collect(it->path());
      } else if (it->is_directory(error)) {
        collect(it->path());
      }
      error.clear();
    }
  }
  std::sort(m_entries.begin(), m_entries.end(), [this](const auto& a, const auto& b) {
    if (m_mode == Mode::Resume && a.complete != b.complete) return !a.complete;
    const int byDate = a.date.Compare(b.date);
    return byDate != 0 ? byDate > 0 : a.path < b.path;
  });

  int selection = 0;
  const auto display = [](const CString& value) { return value.IsEmpty() ? L"—" : value.GetString(); };
  for (size_t i = 0; i < m_entries.size(); ++i) {
    const auto& entry = m_entries[i];
    const int row = m_list.InsertItem(static_cast<int>(i), display(configs ? entry.name : entry.date));
    m_list.SetItemText(row, 1, display(configs ? entry.date : entry.dll));
    m_list.SetItemText(row, 2, display(configs ? entry.dll : entry.db));
    if (resume) m_list.SetItemText(row, 3, display(entry.status));
    if (samePath(previous, entry.path)) selection = row;
  }
  if (!m_entries.empty()) {
    m_list.SetItemState(selection, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
    m_list.EnsureVisible(selection, FALSE);
  }
  m_list.SetRedraw(TRUE);
  m_list.Invalidate(FALSE);
  UpdateSelection();
  UpdateStatus();
}

void CResultsPage::OnAddRoot() {
  auto& roots = m_mode == Mode::Configs ? m_configRoots : m_roots;
  const auto initial = roots.empty() ? fpe::userDataDir() : roots.back();
  const auto folder = BrowseFolder(this, browseStartFolder(initial, fpe::userDataDir()));
  if (folder.IsEmpty()) return;
  includeRoot(roots, toW(folder));
  Refresh();
}

void CResultsPage::OnBrowseConfig() {
  const auto* entry = SelectedEntry();
  const auto initial = entry ? fs::path(entry->path).parent_path().native() : fpe::userDataDir();
  const CString selected = BrowseFile(this, L"설정 (*.json)|*.json||", browseStartFolder(initial, fpe::userDataDir()));
  if (selected.IsEmpty()) return;
  const auto path = toW(selected);
  Entry config;
  if (!ReadConfig(path, config)) {
    AfxMessageBox(L"FPEval 설정 파일 읽기 실패", MB_ICONWARNING);
    return;
  }
  IncludeConfigRoot(fs::path(path).parent_path().native());
  Refresh();
  const auto found = std::find_if(m_entries.begin(), m_entries.end(), [&](const auto& item) { return samePath(item.path, path); });
  if (found == m_entries.end()) return;
  const int index = static_cast<int>(found - m_entries.begin());
  m_list.SetItemState(index, LVIS_SELECTED | LVIS_FOCUSED, LVIS_SELECTED | LVIS_FOCUSED);
  m_list.EnsureVisible(index, FALSE);
  m_list.SetFocus();
}

const CResultsPage::Entry* CResultsPage::SelectedEntry() const {
  const auto index = m_list.GetNextItem(-1, LVNI_SELECTED);
  return index >= 0 && static_cast<size_t>(index) < m_entries.size() ? &m_entries[index] : nullptr;
}

void CResultsPage::UpdateSelection() {
  const auto* entry = SelectedEntry();
  CString details;
  if (entry) details = m_mode == Mode::Resume ? entry->resumeDetails : entry->details;
  SetDlgItemText(IDC_RESULTS_DETAIL, details);
  GetDlgItem(IDC_RESULTS_HTML)->EnableWindow(entry && !entry->htmlPath.empty());
  GetDlgItem(IDC_RESULTS_DOCX)->EnableWindow(entry && !entry->docxPath.empty());
  GetDlgItem(IDC_RESULTS_SELECT)->EnableWindow(entry != nullptr);
}

void CResultsPage::UpdateStatus() {
  const auto* name = m_mode == Mode::Configs ? L"설정 파일" : m_mode == Mode::Resume ? L"저장된 실행" : L"리포트";
  CString status;
  if (m_entries.empty()) {
    status = m_mode == Mode::Resume && IsDlgButtonChecked(IDC_RESULTS_COMPLETED) != BST_CHECKED
                 ? CString(L"재개 후보 없음")
                 : CString(name) + L" 없음";
  } else {
    status.Format(L"%s %zu개", name, m_entries.size());
  }
  SetDlgItemText(IDC_RESULTS_STATUS, status);
}

void CResultsPage::OnSelectionChanged(NMHDR*, LRESULT* result) {
  UpdateSelection();
  *result = 0;
}

void CResultsPage::OnListDraw(NMHDR* header, LRESULT* result) {
  auto* draw = reinterpret_cast<NMLVCUSTOMDRAW*>(header);
  *result = CDRF_DODEFAULT;
  if (draw->nmcd.dwDrawStage == CDDS_PREPAINT) {
    HIGHCONTRASTW contrast{sizeof contrast};
    if (!SystemParametersInfoW(SPI_GETHIGHCONTRAST, sizeof contrast, &contrast, 0) ||
        !(contrast.dwFlags & HCF_HIGHCONTRASTON))
      *result = CDRF_NOTIFYITEMDRAW;
    return;
  }
  if (draw->nmcd.dwDrawStage != CDDS_ITEMPREPAINT) return;
  draw->clrText = theme::kText;
  const int index = static_cast<int>(draw->nmcd.dwItemSpec);
  if (!m_list.GetItemState(index, LVIS_SELECTED)) return;
  CRect row, viewport;
  if (!m_list.GetItemRect(index, &row, LVIR_BOUNDS)) return;
  m_list.GetClientRect(&viewport);
  CDC* dc = CDC::FromHandle(draw->nmcd.hdc);
  const int saved = dc->SaveDC();
  const int dpi = static_cast<int>(GetDpiForWindow(m_list.GetSafeHwnd()));
  const int padding = MulDiv(7, dpi, 96);
  CRect highlight(std::max(row.left, viewport.left) + 1, row.top, viewport.right - 1, row.bottom);
  const bool focused = ::GetFocus() == m_list.GetSafeHwnd() && m_list.GetItemState(index, LVIS_FOCUSED);
  theme::FillRoundRect(dc, highlight, MulDiv(8, dpi, 96), theme::kSelected,
                       focused ? theme::kAccent : theme::kSelected);
  if (CFont* font = m_list.GetFont()) dc->SelectObject(font);
  dc->SetBkMode(TRANSPARENT);
  dc->SetTextColor(theme::kAccentPressed);
  int x = row.left;
  const int columns = m_mode == Mode::Resume ? 4 : 3;
  for (int column = 0; column < columns; ++column) {
    const int width = m_list.GetColumnWidth(column);
    CRect cell(x + padding, row.top, x + width - padding, row.bottom);
    dc->DrawText(m_list.GetItemText(index, column), cell, DT_SINGLELINE | DT_VCENTER | DT_END_ELLIPSIS | DT_NOPREFIX);
    x += width;
  }
  dc->RestoreDC(saved);
  *result = CDRF_SKIPDEFAULT;
}

void CResultsPage::OnListActivate(NMHDR*, LRESULT* result) {
  if (m_mode == Mode::Reports) {
    const auto* entry = SelectedEntry();
    if (entry) OpenReport(entry->htmlPath.empty() ? entry->docxPath : entry->htmlPath);
  } else {
    OnSelect();
  }
  *result = 0;
}

void CResultsPage::OnSelect() {
  const auto* entry = SelectedEntry();
  if (!entry || !m_choose) return;
  const auto path = entry->path;
  const auto choose = m_choose;
  choose(path);
}

void CResultsPage::OnOpenHtml() {
  const auto* entry = SelectedEntry();
  if (entry) OpenReport(entry->htmlPath);
}

void CResultsPage::OnOpenDocx() {
  const auto* entry = SelectedEntry();
  if (entry) OpenReport(entry->docxPath);
}

void CResultsPage::OpenReport(const std::wstring& path) {
  if (path.empty()) return;
  if (!fpe::fileExists(path)) {
    Refresh();
    AfxMessageBox(L"리포트 파일 없음\n목록 새로 고침 완료", MB_ICONWARNING);
    return;
  }
  const auto result = ShellExecuteW(m_hWnd, L"open", path.c_str(), nullptr, nullptr, SW_SHOWNORMAL);
  if (reinterpret_cast<INT_PTR>(result) <= 32)
    AfxMessageBox(L"리포트 열기 실패\nWindows 기본 연결 앱 확인 필요", MB_ICONERROR);
}

// ------------------------------------------------------------ 홈 화면용 최근 실행

std::vector<RecentRun> ScanRecentRuns(const std::vector<std::wstring>& roots, size_t limit) {
  std::vector<RecentRun> runs;
  std::vector<std::wstring> seenRoots;
  for (const auto& root : roots) includeRoot(seenRoots, root);
  for (const auto& root : seenRoots) {
    std::error_code error;
    for (fs::directory_iterator it(root, error), end; !error && it != end; it.increment(error)) {
      if (!it->is_directory(error)) {
        error.clear();
        continue;
      }
      const fs::path folder = it->path();
      if (std::any_of(runs.begin(), runs.end(), [&](const auto& r) { return samePath(r.path, folder.native()); })) continue;
      std::string parseError;
      fpe::Json info;
      if (!fpe::Json::parse(readText(folder / L"run_info.json"), info, parseError)) continue;
      fpe::EvalConfig config;
      const bool hasConfig = fpe::EvalConfig::load((folder / L"config.json").native(), config, parseError);
      RecentRun run;
      run.path = folder.native();
      const auto name = folder.filename().native();
      CString stamp = toCS(info.get("startedAt").asString());
      if (stamp.IsEmpty() && name.size() >= 15 && isTimestamp(toCS(name.substr(name.size() - 15))))
        stamp = toCS(name.substr(name.size() - 15));
      run.date = formatTimestamp(stamp);
      run.dll = toCS(info.get("dllName").asString(fpe::toUtf8(fpe::fileNameOf(config.dllPath))));
      run.db = toCS(info.get("dbName").asString(fpe::toUtf8(fpe::fileNameOf(config.dbPath))));
      std::error_code pathError;
      const auto reportDir = hasConfig && !config.reportDir.empty() ? fs::absolute(fs::path(config.reportDir) / name, pathError)
                                                                    : folder;
      run.htmlPath = findReportFile(pathError ? folder : reportDir, folder, L"report.html");
      std::istringstream input(readText(folder / L"summary.csv"));
      std::string header, row;
      if (std::getline(input, header) && std::getline(input, row)) {
        const auto keys = fpe::parseCsvLine(header);
        const auto values = fpe::parseCsvLine(row);
        auto field = [&](const char* key) {
          const auto index = static_cast<size_t>(std::find(keys.begin(), keys.end(), key) - keys.begin());
          return index < keys.size() && index < values.size() ? toCS(values[index]) : CString();
        };
        run.hasSummary = true;
        run.complete = field("incomplete_jobs") == L"0";
        run.eer = field("eer_percent");
      }
      runs.push_back(std::move(run));
    }
  }
  std::sort(runs.begin(), runs.end(), [](const auto& a, const auto& b) {
    const int byDate = a.date.Compare(b.date);
    return byDate != 0 ? byDate > 0 : a.path < b.path;
  });
  if (runs.size() > limit) runs.resize(limit);
  return runs;
}
