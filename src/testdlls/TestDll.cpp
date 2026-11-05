// 시험용 DLL: 평가 프로그램의 오류 감지 기능을 검증·시연하기 위한 DLL.
// 하나의 소스를 컴파일 정의(TESTDLL_*)로 6종 DLL로 빌드한다.
//   DUMMY_OK    : K-NBTC 예제 BioApi.cpp와 같은 동작(고정 템플릿, 점수 0.8)
//   BAD_CRASH   : 일부 입력에서 NULL 접근
//   BAD_TIMEOUT : 일부 입력에서 무한 루프
//   BAD_SCORE   : 일부 비교에서 1.5 / -0.1 / NaN 반환
//   BAD_SIZE    : 일부 추출에서 템플릿 크기 0 / 2,000,000 보고, 또는 출력 버퍼 오버런
//   BAD_RANDOM  : 무작위 점수(EER ≈ 50% 확인용)
#include <windows.h>

#include <cmath>
#include <cstdint>
#include <cstring>
#include <limits>

#define KISADLL extern "C" __declspec(dllexport)

namespace {

// 이미지 앞부분으로 만든 간단한 해시: 같은 이미지에는 항상 같은 동작을 하도록(재현성)
uint32_t imageHash(const unsigned char* p) {
  uint32_t h = 2166136261u;
  for (int i = 0; i < 4096; i += 7) h = (h ^ p[i]) * 16777619u;
  return h;
}

void setMsg(char* dst, const char* src) {
  strncpy_s(dst, 256, src, _TRUNCATE);
}

int extract(int iType, unsigned char* pImage, unsigned char* pOut, int* piSize, int* piErr, char* msg) {
  (void)iType;
  uint32_t h = imageHash(pImage);
  *piErr = 0;
  setMsg(msg, "");
#if defined(TESTDLL_BAD_CRASH)
  if (h % 5 == 0) {
    volatile int* p = nullptr;
    *p = 1;  // Access Violation
  }
#elif defined(TESTDLL_BAD_TIMEOUT)
  if (h % 6 == 0) {
    volatile bool forever = true;
    while (forever) Sleep(10);
  }
#elif defined(TESTDLL_BAD_SIZE)
  if (h % 4 == 0) {
    *piSize = 0;
    memcpy(pOut, &h, 4);
    return 1;  // 성공이라면서 크기 0
  }
  if (h % 4 == 1) {
    *piSize = 2000000;
    return 1;  // 규격 초과 크기
  }
  if (h % 4 == 2) {
    memset(pOut, 0x11, 1000000 + 64);  // 출력 버퍼 오버런
    *piSize = 1000;
    return 1;
  }
#endif
  // 4바이트 해시 + 패딩 → 1000바이트 템플릿 (예제 DLL과 같은 크기)
  memset(pOut, 0, 1000);
  memcpy(pOut, &h, 4);
  *piSize = 1000;
  return 1;
}

}  // namespace

KISADLL int KISA_Enroll(int iType, unsigned char* pImage, unsigned char* pEnroll, int* piEnrollSize,
                        int* piErrorCode, char arErrorMessage[256]) {
  return extract(iType, pImage, pEnroll, piEnrollSize, piErrorCode, arErrorMessage);
}

KISADLL int KISA_Attempt(int iType, unsigned char* pImage, unsigned char* pAttempt, int* piAttemptSize,
                         int* piErrorCode, char arErrorMessage[256]) {
  return extract(iType, pImage, pAttempt, piAttemptSize, piErrorCode, arErrorMessage);
}

KISADLL int KISA_Compare(unsigned char* pEnroll, unsigned char* pAttempt, double* pdScore, int* piErrorCode,
                         char arErrorMessage[256]) {
  uint32_t a, b;
  memcpy(&a, pEnroll, 4);
  memcpy(&b, pAttempt, 4);
  uint32_t h = (a * 31u) ^ b;
  (void)h;
  *piErrorCode = 0;
  setMsg(arErrorMessage, "");
#if defined(TESTDLL_BAD_SCORE)
  if (h % 7 == 0) {
    *pdScore = 1.5;
    return 1;
  }
  if (h % 7 == 1) {
    *pdScore = -0.1;
    return 1;
  }
  if (h % 7 == 2) {
    *pdScore = std::numeric_limits<double>::quiet_NaN();
    return 1;
  }
  if (h % 7 == 3) {
    *piErrorCode = 42;
    setMsg(arErrorMessage, "compare refused");
    return 0;
  }
#endif
#if defined(TESTDLL_BAD_RANDOM) || defined(TESTDLL_BAD_SCORE)
  *pdScore = std::floor(((h * 2654435761u) % 1001u)) / 1000.0;
#else
  *pdScore = 0.8;  // 예제 DLL과 동일
#endif
  return 1;
}

BOOL APIENTRY DllMain(HMODULE, DWORD, LPVOID) { return TRUE; }
