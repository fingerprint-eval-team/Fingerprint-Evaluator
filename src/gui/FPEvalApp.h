#pragma once
#include "pch.h"

// 애플리케이션 클래스: 공용 컨트롤·COM 초기화 후 메인 대화상자를 모달로 띄운다
class CFPEvalApp : public CWinApp {
 public:
  BOOL InitInstance() override;
  DECLARE_MESSAGE_MAP()
};

extern CFPEvalApp theApp;
