#pragma once
// K-NBTC 지문인식 성능시험 DLL 함수 규격 (안내서 p26-29)
namespace fpe {

using KisaExtractFn = int(__cdecl*)(int iType, unsigned char* pImage, unsigned char* pOut, int* piOutSize,
                                    int* piErrorCode, char arErrorMessage[256]);
using KisaCompareFn = int(__cdecl*)(unsigned char* pEnroll, unsigned char* pAttempt, double* pdScore,
                                    int* piErrorCode, char arErrorMessage[256]);

constexpr const char* kEnrollName = "KISA_Enroll";
constexpr const char* kAttemptName = "KISA_Attempt";
constexpr const char* kCompareName = "KISA_Compare";

}  // namespace fpe
