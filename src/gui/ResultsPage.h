#pragma once

#include "pch.h"

#include <functional>
#include <vector>

#include "Pages.h"

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
