#pragma once
#include <windows.h>

#ifndef _KISA_IMPORT
#define KISADLL extern "C" __declspec(dllexport)
#else
#define KISADLL extern "C" __declspec(dllimport)
#endif

// 오류 코드 (piErrorCode)
enum FpaError {
  FPA_OK = 0,
  FPA_ERR_UNKNOWN_ITYPE = 1,
  FPA_ERR_NULL_ARG = 2,
  FPA_ERR_NO_FOREGROUND = 10,
  FPA_ERR_FEW_MINUTIAE = 11,
  FPA_ERR_TEMPLATE = 20,
  FPA_ERR_EXCEPTION = 99
};

KISADLL int KISA_Enroll(IN int iType, IN unsigned char* pImage, OUT unsigned char* pEnroll, OUT int* piEnrollSize,
                        OUT int* piErrorCode, OUT char arErrorMessage[256]);

KISADLL int KISA_Attempt(IN int iType, IN unsigned char* pImage, OUT unsigned char* pAttempt,
                         OUT int* piAttemptSize, OUT int* piErrorCode, OUT char arErrorMessage[256]);

KISADLL int KISA_Compare(IN unsigned char* pEnroll, IN unsigned char* pAttempt, OUT double* pdScore,
                         OUT int* piErrorCode, OUT char arErrorMessage[256]);
