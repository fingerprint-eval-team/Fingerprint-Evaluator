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

#include <string>

#include "PathUtil.h"
#include "resource.h"

// std::string(UTF-8) / std::wstring ↔ CString
inline CString toCS(const std::string& utf8) { return CString(fpe::fromUtf8(utf8).c_str()); }
inline CString toCS(const std::wstring& w) { return CString(w.c_str()); }
inline std::wstring toW(const CString& s) { return std::wstring(s.GetString()); }

// 파일·폴더 선택 대화상자
inline CString BrowseFile(CWnd* parent, LPCTSTR filter, BOOL save = FALSE, LPCTSTR defExt = nullptr) {
  CFileDialog dlg(!save, defExt, nullptr,
                  save ? (OFN_OVERWRITEPROMPT | OFN_HIDEREADONLY) : (OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST), filter,
                  parent);
  return dlg.DoModal() == IDOK ? dlg.GetPathName() : CString();
}

inline CString BrowseFolder(CWnd* parent) {
  CFolderPickerDialog dlg(nullptr, 0, parent);
  return dlg.DoModal() == IDOK ? dlg.GetPathName() : CString();
}
