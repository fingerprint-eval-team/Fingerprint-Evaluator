#pragma once
// 메인 대화상자의 탭 페이지 4개. 각 페이지는 리소스 대화상자 템플릿을 쓰는 자식 CDialogEx이며,
// DDX로 컨트롤 값을 멤버 변수와 주고받고 LoadFrom/SaveTo로 EvalConfig와 변환한다.
#include "pch.h"

#include "config/EvalConfig.h"

// 모든 페이지 공통: Enter/Esc로 자식 대화상자가 닫히지 않게, 탭 배경과 같은 테마 배경 사용
class CPageBase : public CDialogEx {
 public:
  explicit CPageBase(UINT id) : CDialogEx(id) {}
  virtual void LoadFrom(const fpe::EvalConfig& c) = 0;
  virtual bool SaveTo(fpe::EvalConfig& c) = 0;

 protected:
  BOOL OnInitDialog() override;
  void OnOK() override {}
  void OnCancel() override {}
  void NotifyParent(UINT msg) { GetParent()->PostMessage(msg); }
};

// ---- 탭 1: 대상 ----
class CTargetPage : public CPageBase {
 public:
  CTargetPage() : CPageBase(IDD_PAGE_TARGET) {}
  void LoadFrom(const fpe::EvalConfig& c) override;
  bool SaveTo(fpe::EvalConfig& c) override;
  void SetDbInfo(const CString& s) { SetDlgItemText(IDC_DB_INFO, s); }
  CString DbPath() const;

 protected:
  void DoDataExchange(CDataExchange* pDX) override;
  afx_msg void OnBrowseDll();
  afx_msg void OnBrowseDb();
  afx_msg void OnBrowseTpl();
  afx_msg void OnBrowseOut();
  afx_msg void OnBrowseReportDir();
  afx_msg void OnBrowseLogDir();
  afx_msg void OnDllChange();
  afx_msg void OnDbKillFocus();
  DECLARE_MESSAGE_MAP()

 private:
  void UpdateDllInfo();
  CString m_dll, m_db, m_tpl, m_out, m_repDir, m_logDir;
  int m_iType = 1;
};

// ---- 탭 2: 시험 규칙 ----
class CRulePage : public CPageBase {
 public:
  CRulePage() : CPageBase(IDD_PAGE_RULES) {}
  void LoadFrom(const fpe::EvalConfig& c) override;
  bool SaveTo(fpe::EvalConfig& c) override;
  void SetEstimate(const CString& s) { SetDlgItemText(IDC_ESTIMATE, s); }

 protected:
  BOOL OnInitDialog() override;
  void DoDataExchange(CDataExchange* pDX) override;
  afx_msg void OnRuleChanged() { NotifyParent(WM_APP_RULES_CHANGED); }
  afx_msg void OnNewSeed();
  afx_msg void OnBrowsePairs();
  DECLARE_MESSAGE_MAP()

 private:
  int m_enroll = 0, m_gen = 0, m_imp = 0;
  int m_enrollIdx = 1, m_genN = 200, m_impN = 2000;
  CString m_impIdx, m_seed, m_pairs;
  BOOL m_sym = TRUE;
};

// ---- 탭 3: 실행 옵션 ----
class CRunPage : public CPageBase {
 public:
  CRunPage() : CPageBase(IDD_PAGE_RUN) {}
  void LoadFrom(const fpe::EvalConfig& c) override;
  bool SaveTo(fpe::EvalConfig& c) override;

 protected:
  BOOL OnInitDialog() override;
  void DoDataExchange(CDataExchange* pDX) override;

 private:
  int m_workers = 4, m_timeout = 10;
  BOOL m_anon = TRUE;
};

// ---- 탭 4: 리포트 ----
class CReportPage : public CPageBase {
 public:
  static constexpr int kItems = 15;  // allReportItems()와 같은 순서·개수
  CReportPage() : CPageBase(IDD_PAGE_REPORT) {}
  void LoadFrom(const fpe::EvalConfig& c) override;
  bool SaveTo(fpe::EvalConfig& c) override;

 protected:
  BOOL OnInitDialog() override;
  void DoDataExchange(CDataExchange* pDX) override;

 private:
  BOOL m_items[kItems] = {};
  BOOL m_html = TRUE, m_docx = TRUE;
  int m_eer = 0, m_denom = 0;
};
