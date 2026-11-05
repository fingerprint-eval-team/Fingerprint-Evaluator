#include "pch.h"

#include "Pages.h"

#include <algorithm>
#include <vector>

namespace {

// 콤보 상자 항목 ↔ 설정 값
const std::vector<std::string> kEnrollRules{"All", "First", "Index", "RandomPerFinger"};
const std::vector<std::string> kGenRules{"Symmetric", "All", "RandomN"};
const std::vector<std::string> kImpRules{"FirstOne", "All", "SelectedIndex", "RandomN"};
const std::vector<std::string> kEerMethods{"FVC", "Nearest"};
const std::vector<std::string> kDenoms{"predefined", "attempted"};

int indexOf(const std::vector<std::string>& v, const std::string& s) {
  auto it = std::find(v.begin(), v.end(), s);
  return it == v.end() ? 0 : (int)(it - v.begin());
}

const std::string& valueAt(const std::vector<std::string>& v, int i) { return v[(size_t)std::clamp(i, 0, (int)v.size() - 1)]; }

void fillCombo(CWnd* page, int id, const std::vector<std::string>& v) {
  auto* cb = static_cast<CComboBox*>(page->GetDlgItem(id));
  cb->ResetContent();
  for (auto& s : v) cb->AddString(toCS(s));
}

}  // namespace

// ============================================================ 공통

BOOL CPageBase::OnInitDialog() {
  CDialogEx::OnInitDialog();
  EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);  // 탭 컨트롤 배경과 같은 테마 배경
  return TRUE;
}

// ============================================================ 탭 1: 대상

BEGIN_MESSAGE_MAP(CTargetPage, CPageBase)
  ON_BN_CLICKED(IDC_DLL_BROWSE, &CTargetPage::OnBrowseDll)
  ON_BN_CLICKED(IDC_DB_BROWSE, &CTargetPage::OnBrowseDb)
  ON_BN_CLICKED(IDC_TPL_BROWSE, &CTargetPage::OnBrowseTpl)
  ON_BN_CLICKED(IDC_OUT_BROWSE, &CTargetPage::OnBrowseOut)
  ON_EN_CHANGE(IDC_DLL, &CTargetPage::OnDllChange)
  ON_EN_KILLFOCUS(IDC_DB, &CTargetPage::OnDbKillFocus)
END_MESSAGE_MAP()

void CTargetPage::DoDataExchange(CDataExchange* pDX) {
  CPageBase::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_DLL, m_dll);
  DDX_Text(pDX, IDC_DB, m_db);
  DDX_Text(pDX, IDC_ITYPE, m_iType);
  DDV_MinMaxInt(pDX, m_iType, 0, 1000);
  DDX_Text(pDX, IDC_TPL, m_tpl);
  DDX_Text(pDX, IDC_OUT, m_out);
}

void CTargetPage::LoadFrom(const fpe::EvalConfig& c) {
  m_dll = toCS(c.dllPath);
  m_db = toCS(c.dbPath);
  m_iType = c.iType;
  m_tpl = toCS(c.templateDir);
  m_out = toCS(c.outputDir);
  UpdateData(FALSE);
  UpdateDllInfo();
}

bool CTargetPage::SaveTo(fpe::EvalConfig& c) {
  if (!UpdateData(TRUE)) return false;
  c.dllPath = toW(m_dll);
  c.dbPath = toW(m_db);
  c.iType = m_iType;
  c.templateDir = toW(m_tpl);
  c.outputDir = toW(m_out);
  return true;
}

CString CTargetPage::DbPath() const {
  CString s;
  GetDlgItemText(IDC_DB, s);
  return s;
}

void CTargetPage::UpdateDllInfo() {
  CString p;
  GetDlgItemText(IDC_DLL, p);
  CString info;
  if (!p.IsEmpty()) {
    if (!fpe::fileExists(toW(p))) {
      info = L"파일이 없습니다";
    } else {
      int bits = fpe::peBitness(toW(p));
      if (bits) {
        info.Format(L"%dbit DLL → FPWorker%d.exe에서 실행", bits, bits);
      } else {
        info = L"Windows DLL이 아닙니다";
      }
    }
  }
  SetDlgItemText(IDC_DLL_INFO, info);
}

void CTargetPage::OnDllChange() { UpdateDllInfo(); }
void CTargetPage::OnDbKillFocus() { NotifyParent(WM_APP_DB_CHANGED); }

void CTargetPage::OnBrowseDll() {
  CString p = BrowseFile(this, L"DLL (*.dll)|*.dll||");
  if (!p.IsEmpty()) SetDlgItemText(IDC_DLL, p);
}

void CTargetPage::OnBrowseDb() {
  CString p = BrowseFolder(this);
  if (!p.IsEmpty()) {
    SetDlgItemText(IDC_DB, p);
    NotifyParent(WM_APP_DB_CHANGED);
  }
}

void CTargetPage::OnBrowseTpl() {
  CString p = BrowseFolder(this);
  if (!p.IsEmpty()) SetDlgItemText(IDC_TPL, p);
}

void CTargetPage::OnBrowseOut() {
  CString p = BrowseFolder(this);
  if (!p.IsEmpty()) SetDlgItemText(IDC_OUT, p);
}

// ============================================================ 탭 2: 시험 규칙

BEGIN_MESSAGE_MAP(CRulePage, CPageBase)
  ON_CBN_SELCHANGE(IDC_ENROLL, &CRulePage::OnRuleChanged)
  ON_CBN_SELCHANGE(IDC_GEN, &CRulePage::OnRuleChanged)
  ON_CBN_SELCHANGE(IDC_IMP, &CRulePage::OnRuleChanged)
  ON_EN_KILLFOCUS(IDC_ENROLL_IDX, &CRulePage::OnRuleChanged)
  ON_EN_KILLFOCUS(IDC_GEN_N, &CRulePage::OnRuleChanged)
  ON_EN_KILLFOCUS(IDC_IMP_N, &CRulePage::OnRuleChanged)
  ON_EN_KILLFOCUS(IDC_IMP_IDX, &CRulePage::OnRuleChanged)
  ON_EN_KILLFOCUS(IDC_SEED, &CRulePage::OnRuleChanged)
  ON_EN_KILLFOCUS(IDC_PAIRS, &CRulePage::OnRuleChanged)
  ON_BN_CLICKED(IDC_IMP_SYM, &CRulePage::OnRuleChanged)
  ON_BN_CLICKED(IDC_SEED_NEW, &CRulePage::OnNewSeed)
  ON_BN_CLICKED(IDC_PAIRS_BROWSE, &CRulePage::OnBrowsePairs)
END_MESSAGE_MAP()

BOOL CRulePage::OnInitDialog() {
  CPageBase::OnInitDialog();
  fillCombo(this, IDC_ENROLL, kEnrollRules);
  fillCombo(this, IDC_GEN, kGenRules);
  fillCombo(this, IDC_IMP, kImpRules);
  return TRUE;
}

void CRulePage::DoDataExchange(CDataExchange* pDX) {
  CPageBase::DoDataExchange(pDX);
  DDX_CBIndex(pDX, IDC_ENROLL, m_enroll);
  DDX_Text(pDX, IDC_ENROLL_IDX, m_enrollIdx);
  DDX_CBIndex(pDX, IDC_GEN, m_gen);
  DDX_Text(pDX, IDC_GEN_N, m_genN);
  DDX_CBIndex(pDX, IDC_IMP, m_imp);
  DDX_Text(pDX, IDC_IMP_N, m_impN);
  DDX_Text(pDX, IDC_IMP_IDX, m_impIdx);
  DDX_Check(pDX, IDC_IMP_SYM, m_sym);
  DDX_Text(pDX, IDC_SEED, m_seed);
  DDX_Text(pDX, IDC_PAIRS, m_pairs);
}

void CRulePage::LoadFrom(const fpe::EvalConfig& c) {
  m_enroll = indexOf(kEnrollRules, c.enrollRule);
  m_enrollIdx = c.enrollIndex;
  m_gen = indexOf(kGenRules, c.genuineRule);
  m_genN = c.genuineN;
  m_imp = indexOf(kImpRules, c.impostorRule);
  m_impN = c.impostorN;
  m_impIdx.Empty();
  for (size_t i = 0; i < c.impostorIndices.size(); ++i) {
    CString t;
    t.Format(i ? L",%d" : L"%d", c.impostorIndices[i]);
    m_impIdx += t;
  }
  m_sym = c.impostorSymmetric;
  m_seed.Format(L"%llu", (unsigned long long)c.seed);
  m_pairs = toCS(c.pairsFile);
  UpdateData(FALSE);
}

bool CRulePage::SaveTo(fpe::EvalConfig& c) {
  if (!UpdateData(TRUE)) return false;
  c.enrollRule = valueAt(kEnrollRules, m_enroll);
  c.enrollIndex = m_enrollIdx;
  c.genuineRule = valueAt(kGenRules, m_gen);
  c.genuineN = m_genN;
  c.impostorRule = valueAt(kImpRules, m_imp);
  c.impostorN = m_impN;
  c.impostorIndices.clear();
  int pos = 0;
  for (CString tok = m_impIdx.Tokenize(L",", pos); !tok.IsEmpty(); tok = m_impIdx.Tokenize(L",", pos))
    c.impostorIndices.push_back(_wtoi(tok));
  c.impostorSymmetric = m_sym != FALSE;
  c.seed = _wcstoui64(m_seed, nullptr, 10);
  c.pairsFile = toW(m_pairs);
  return true;
}

void CRulePage::OnNewSeed() {
  LARGE_INTEGER t;
  QueryPerformanceCounter(&t);
  CString s;
  s.Format(L"%llu", (unsigned long long)(t.QuadPart % 1000000000ULL));
  SetDlgItemText(IDC_SEED, s);
  OnRuleChanged();
}

void CRulePage::OnBrowsePairs() {
  CString p = BrowseFile(this, L"CSV (*.csv;*.txt)|*.csv;*.txt||");
  if (!p.IsEmpty()) {
    SetDlgItemText(IDC_PAIRS, p);
    OnRuleChanged();
  }
}

// ============================================================ 탭 3: 실행 옵션

BOOL CRunPage::OnInitDialog() {
  CPageBase::OnInitDialog();
  SYSTEM_INFO si;
  GetSystemInfo(&si);
  CString s;
  s.Format(L"Worker 프로세스 개수 (이 PC의 논리 CPU %lu개)", si.dwNumberOfProcessors);
  SetDlgItemText(IDC_CPU_INFO, s);
  return TRUE;
}

void CRunPage::DoDataExchange(CDataExchange* pDX) {
  CPageBase::DoDataExchange(pDX);
  DDX_Text(pDX, IDC_WORKERS, m_workers);
  DDV_MinMaxInt(pDX, m_workers, 1, 64);
  DDX_Text(pDX, IDC_TIMEOUT, m_timeout);
  DDV_MinMaxInt(pDX, m_timeout, 1, 3600);
  DDX_Check(pDX, IDC_ANON, m_anon);
}

void CRunPage::LoadFrom(const fpe::EvalConfig& c) {
  m_workers = std::clamp(c.workers, 1, 64);
  m_timeout = std::clamp(c.timeoutSec, 1, 3600);
  m_anon = c.anonymize;
  UpdateData(FALSE);
}

bool CRunPage::SaveTo(fpe::EvalConfig& c) {
  if (!UpdateData(TRUE)) return false;
  c.workers = m_workers;
  c.timeoutSec = m_timeout;
  c.anonymize = m_anon != FALSE;
  return true;
}

// ============================================================ 탭 4: 리포트

BOOL CReportPage::OnInitDialog() {
  CPageBase::OnInitDialog();
  fillCombo(this, IDC_EER_METHOD, kEerMethods);
  fillCombo(this, IDC_DENOM, kDenoms);
  return TRUE;
}

void CReportPage::DoDataExchange(CDataExchange* pDX) {
  CPageBase::DoDataExchange(pDX);
  for (int i = 0; i < kItems; ++i) DDX_Check(pDX, IDC_ITEM0 + i, m_items[i]);
  DDX_Check(pDX, IDC_HTML, m_html);
  DDX_Check(pDX, IDC_DOCX, m_docx);
  DDX_CBIndex(pDX, IDC_EER_METHOD, m_eer);
  DDX_CBIndex(pDX, IDC_DENOM, m_denom);
}

void CReportPage::LoadFrom(const fpe::EvalConfig& c) {
  auto all = fpe::allReportItems();
  for (int i = 0; i < kItems; ++i) m_items[i] = c.items.count(all[i]) > 0;
  m_html = c.html;
  m_docx = c.docx;
  m_eer = indexOf(kEerMethods, c.eerMethod);
  m_denom = indexOf(kDenoms, c.denominator);
  UpdateData(FALSE);
}

bool CReportPage::SaveTo(fpe::EvalConfig& c) {
  if (!UpdateData(TRUE)) return false;
  auto all = fpe::allReportItems();
  c.items.clear();
  for (int i = 0; i < kItems; ++i)
    if (m_items[i]) c.items.insert(all[i]);
  c.html = m_html != FALSE;
  c.docx = m_docx != FALSE;
  c.eerMethod = valueAt(kEerMethods, m_eer);
  c.denominator = valueAt(kDenoms, m_denom);
  return true;
}
