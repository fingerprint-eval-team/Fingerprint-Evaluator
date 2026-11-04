# FPEval.exe — MFC 대화상자 기반 GUI
# CMAKE_MFC_FLAG는 디렉터리 단위로 적용되므로 GUI만 별도 디렉터리로 분리한다.
# 1 = 정적 MFC (정적 CRT /MT와 짝을 맞춤 → 배포 시 MFC/VC++ 재배포 패키지 불필요)
set(CMAKE_MFC_FLAG 1)

add_executable(FPEval WIN32
  FPEvalApp.cpp
  MainDlg.cpp
  Pages.cpp
  ProgressBridge.cpp
  FPEval.rc)
target_link_libraries(FPEval PRIVATE FPEvalCore uxtheme)
# 유니코드 MFC 진입점
target_link_options(FPEval PRIVATE /ENTRY:wWinMainCRTStartup)
fp_output(FPEval ${FP_BIN})
