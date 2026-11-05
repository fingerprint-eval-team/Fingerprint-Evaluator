// FPAlgo.dll — K-NBTC 지문인식 성능시험 DLL 규격 구현
// 처리 단계는 Pipeline.cpp 참고
#include "BioApi.h"

#include <cstring>
#include <exception>
#include <vector>

#include "Pipeline.h"
#include "SensorTable.h"
#include "templ/Template.h"

namespace {

constexpr int kCapacity = 999999;

void setError(int* piErr, char* msg, int code, const char* text) {
  if (piErr) *piErr = code;
  if (msg) strncpy_s(msg, 256, text, _TRUNCATE);
}

int extractTemplate(int iType, const unsigned char* pImage, unsigned char* pOut, int* piSize, int* piErr,
                    char* msg) {
  if (!pImage || !pOut || !piSize) {
    setError(piErr, msg, FPA_ERR_NULL_ARG, "null argument");
    return 0;
  }
  int w = 0, h = 0;
  if (!fpa::lookupSensor(iType, w, h)) {
    setError(piErr, msg, FPA_ERR_UNKNOWN_ITYPE, "unknown iType");
    return 0;
  }
  try {
    std::vector<fpa::Minutia> m;
    const char* text = "";
    int code = fpa::extractFeatures(pImage, w, h, m, &text);
    if (code != FPA_OK) {
      setError(piErr, msg, code, text);
      return 0;
    }
    int size = fpa::writeTemplate(m, w, h, pOut, kCapacity);
    if (size <= 0) {
      setError(piErr, msg, FPA_ERR_TEMPLATE, "template too large");
      return 0;
    }
    *piSize = size;
    setError(piErr, msg, FPA_OK, "");
    return 1;
  } catch (const std::exception& e) {
    setError(piErr, msg, FPA_ERR_EXCEPTION, e.what());
  } catch (...) {
    setError(piErr, msg, FPA_ERR_EXCEPTION, "unknown exception");
  }
  return 0;
}

}  // namespace

KISADLL int KISA_Enroll(int iType, unsigned char* pImage, unsigned char* pEnroll, int* piEnrollSize,
                        int* piErrorCode, char arErrorMessage[256]) {
  return extractTemplate(iType, pImage, pEnroll, piEnrollSize, piErrorCode, arErrorMessage);
}

KISADLL int KISA_Attempt(int iType, unsigned char* pImage, unsigned char* pAttempt, int* piAttemptSize,
                         int* piErrorCode, char arErrorMessage[256]) {
  return extractTemplate(iType, pImage, pAttempt, piAttemptSize, piErrorCode, arErrorMessage);
}

KISADLL int KISA_Compare(unsigned char* pEnroll, unsigned char* pAttempt, double* pdScore, int* piErrorCode,
                         char arErrorMessage[256]) {
  if (!pEnroll || !pAttempt || !pdScore) {
    setError(piErrorCode, arErrorMessage, FPA_ERR_NULL_ARG, "null argument");
    return 0;
  }
  try {
    std::vector<fpa::Minutia> e, a;
    int w1, h1, w2, h2;
    if (!fpa::readTemplate(pEnroll, e, w1, h1) || !fpa::readTemplate(pAttempt, a, w2, h2)) {
      setError(piErrorCode, arErrorMessage, FPA_ERR_TEMPLATE, "invalid template");
      return 0;
    }
    *pdScore = fpa::compareFeatures(e, a);
    setError(piErrorCode, arErrorMessage, FPA_OK, "");
    return 1;
  } catch (...) {
    setError(piErrorCode, arErrorMessage, FPA_ERR_EXCEPTION, "exception in compare");
  }
  return 0;
}
