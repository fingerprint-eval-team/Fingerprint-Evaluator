# FPEval.exe — MFC 대화상자 기반 GUI
# CMAKE_MFC_FLAG는 디렉터리 단위로 적용되므로 GUI만 별도 디렉터리로 분리한다.
# 1 = 정적 MFC (정적 CRT /MT와 짝을 맞춤 → 배포 시 MFC/VC++ 재배포 패키지 불필요)
set(CMAKE_MFC_FLAG 1)

add_executable(FPEval WIN32
  FPEvalApp.cpp
  MainDlg.cpp
  Pages.cpp
  ProgressBridge.cpp
  AccentButton.cpp
  FlatCombo.cpp
  FlatEdit.cpp
  NavBar.cpp
  DashPage.cpp
  HomeView.cpp
  HelpTip.cpp
  ResultsPage.cpp
  SplashWnd.cpp
  FPEval.rc
  FPEval.manifest)  # 고DPI(Per-Monitor V2) 선언, 링커가 기본 매니페스트와 합침
target_link_libraries(FPEval PRIVATE FPEvalCore uxtheme)
# DPI 선언은 FPEval.manifest가 맡는다. VS가 MFC 프로젝트에 자동으로 넣는 dpiAware(true)와 겹치면
# 같은 항목이 두 번 들어가 "side-by-side 구성 오류"로 실행되지 않는다.
set_target_properties(FPEval PROPERTIES VS_DPI_AWARE OFF)
# 유니코드 MFC 진입점
target_link_options(FPEval PRIVATE /ENTRY:wWinMainCRTStartup)
fp_output(FPEval ${FP_BIN})
