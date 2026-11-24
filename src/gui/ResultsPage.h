#pragma once

#include "pch.h"

#include <functional>
#include <vector>

#include "Pages.h"

// 홈 화면의 "최근 실행" 한 줄. 결과 폴더의 run_info.json·summary.csv에서 읽는다.
struct RecentRun {
  std::wstring path, htmlPath;
  CString date, dll, db, eer;  // date는 "YYYY-MM-DD hh:mm:ss", eer는 "2.41" (없으면 빈 문자열)
  bool complete = false;       // summary.csv의 incomplete_jobs가 0
  bool hasSummary = false;     // summary.csv가 있다 (없으면 중단되었거나 아직 진행 중)
};
// roots 바로 아래 실행 폴더를 읽어 시작 시각이 최근인 순서로 최대 limit개 돌려준다.
std::vector<RecentRun> ScanRecentRuns(const std::vector<std::wstring>& roots, size_t limit);

class CResultsPage : public CPageBase {
 public:
  enum class Mode { Reports, Resume, Configs };
  CResultsPage() : CPageBase(IDD_PAGE_RESULTS) {}
  bool SaveTo(fpe::EvalConfig&) override { return true; }
  void LoadFrom(const fpe::EvalConfig&) override {}
  void IncludeRoot(const std::wstring& path);
  void IncludeConfigRoot(const std::wstring& path);
  void ShowMode(Mode mode, std::function<void(const std::wstring&)> choose = {});
  void Refresh();

 protected:
  BOOL OnInitDialog() override;
  afx_msg void OnSize(UINT type, int width, int height);
  afx_msg void OnPaint();
  afx_msg HBRUSH OnCtlColor(CDC* dc, CWnd* wnd, UINT type);
  afx_msg void OnAddRoot();
  afx_msg void OnOpenHtml();
  afx_msg void OnOpenDocx();
  afx_msg void OnSelect();
  afx_msg void OnBrowseConfig();
  afx_msg void OnSelectionChanged(NMHDR*, LRESULT* result);
  afx_msg void OnListActivate(NMHDR* header, LRESULT* result);
  afx_msg void OnListDraw(NMHDR* header, LRESULT* result);
  DECLARE_MESSAGE_MAP()

 private:
  struct Entry {
    std::wstring path, htmlPath, docxPath;
    CString date, dll, db, name, details, status, resumeDetails;
    bool complete = false;
  };
  bool ReadRun(const std::wstring& path, Entry& entry) const;
  bool ReadConfig(const std::wstring& path, Entry& entry) const;
  void UpdateSelection();
  void UpdateStatus();
  const Entry* SelectedEntry() const;
  void OpenReport(const std::wstring& path);
  void LayoutContent(int width, int height);
  void SizeColumns();

  CListCtrl m_list;
  CBrush m_detailBg;
  CRect m_listPanel{0, 0, 0, 0}, m_detailPanel{0, 0, 0, 0};
  Mode m_mode = Mode::Reports;
  std::function<void(const std::wstring&)> m_choose;
  std::vector<std::wstring> m_roots, m_configRoots;
  std::vector<Entry> m_entries;
};
