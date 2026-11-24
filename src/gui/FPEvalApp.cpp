#include "pch.h"

#include "FPEvalApp.h"
#include "MainDlg.h"
#include "SplashWnd.h"

#pragma comment(linker, "\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

BEGIN_MESSAGE_MAP(CFPEvalApp, CWinApp)
END_MESSAGE_MAP()

CFPEvalApp theApp;

BOOL CFPEvalApp::InitInstance() {
  INITCOMMONCONTROLSEX icc{sizeof icc, ICC_WIN95_CLASSES | ICC_TAB_CLASSES | ICC_PROGRESS_CLASS};
  InitCommonControlsEx(&icc);
  CWinApp::InitInstance();
  AfxOleInit();  // CFolderPickerDialog(IFileDialog)에 필요한 COM 초기화

  {  // 시작 화면: 실행 환경을 점검하는 동안 띄우고, 끝나면 닫은 뒤 메인 창을 연다
    CSplashWnd splash;
    splash.Run();
  }

  CMainDlg dlg;
  m_pMainWnd = &dlg;
  dlg.DoModal();
  return FALSE;  // 대화상자가 닫히면 메시지 루프 없이 종료
}
