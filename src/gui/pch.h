// MFC 공통 헤더. MFC 헤더는 windows.h보다 먼저 포함되어야 하므로 모든 GUI 소스의 첫 줄에서 포함한다.
#pragma once

#ifndef VC_EXTRALEAN
#define VC_EXTRALEAN
#endif

#include <sdkddkver.h>

#include <afxwin.h>       // MFC 핵심
#include <afxext.h>       // MFC 확장
#include <afxdialogex.h>  // CDialogEx
#include <afxcmn.h>       // 공용 컨트롤 (CTabCtrl, CProgressCtrl)
#include <afxdlgs.h>      // CFileDialog, CFolderPickerDialog
#include <afxdisp.h>      // AfxOleInit (COM 초기화)

#include <uxtheme.h>

#include <filesystem>
#include <string>
#include <system_error>

#include "PathUtil.h"
#include "resource.h"

// std::string(UTF-8) / std::wstring ↔ CString
inline CString toCS(const std::string& utf8) { return CString(fpe::fromUtf8(utf8).c_str()); }
inline CString toCS(const std::wstring& w) { return CString(w.c_str()); }
inline std::wstring toW(const CString& s) { return std::wstring(s.GetString()); }

// 파일·폴더 선택 대화상자
inline std::wstring existingBrowseFolder(const std::wstring& path) {
  if (path.empty()) return {};
  std::error_code error;
  auto folder = std::filesystem::absolute(path, error).lexically_normal();
  if (error) return {};
  while (!folder.empty()) {
    if (fpe::dirExists(folder.native())) return folder.native();
    const auto parent = folder.parent_path();
    if (parent == folder) break;
    folder = parent;
  }
  return {};
}

inline std::wstring browseStartFolder(const std::wstring& current, const std::wstring& fallback) {
  const auto folder = existingBrowseFolder(current);
  return folder.empty() ? existingBrowseFolder(fallback) : folder;
}

inline CString BrowseFile(CWnd* parent, LPCTSTR filter, const std::wstring& folder, BOOL save = FALSE,
                          LPCTSTR defExt = nullptr) {
  CFileDialog dlg(!save, defExt, nullptr,
                  OFN_NOCHANGEDIR | (save ? (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY)
                                          : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST)),
                  filter, parent);
  if (!folder.empty()) dlg.GetOFN().lpstrInitialDir = folder.c_str();
  return dlg.DoModal() == IDOK ? dlg.GetPathName() : CString();
}

inline CString BrowseFolder(CWnd* parent, const std::wstring& folder) {
  CFolderPickerDialog dlg(nullptr, OFN_NOCHANGEDIR, parent);
  if (!folder.empty()) dlg.GetOFN().lpstrInitialDir = folder.c_str();
  return dlg.DoModal() == IDOK ? dlg.GetPathName() : CString();
}
