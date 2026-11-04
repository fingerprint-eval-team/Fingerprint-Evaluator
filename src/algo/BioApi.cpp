// FPAlgo.dll — K-NBTC 지문인식 성능시험 DLL 규격 구현
// 처리: 정규화 → 방향장·전경 분할 → 융선 간격 추정 → Gabor 강조·이진화 → 세선화 → 미뉴셔 추출 → 템플릿
#include "BioApi.h"

#include <cstring>
#include <exception>

#include "SensorTable.h"
#include "enhance/Enhance.h"
#include "image/Image.h"
#include "match/Matcher.h"
#include "minutiae/Minutiae.h"
#include "templ/Template.h"

namespace {

constexpr int kCapacity = 999999;
constexpr size_t kMinMinutiae = 8;

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
    fpa::ImageF img = fpa::toFloat(pImage, w, h);
    fpa::normalize(img);
    fpa::OrientationField of = fpa::computeOrientation(img);
    size_t fg = 0;
    for (auto v : of.mask) fg += v;
    if (fg < of.mask.size() / 20) {
      setError(piErr, msg, FPA_ERR_NO_FOREGROUND, "no fingerprint area");
      return 0;
    }
    float period = fpa::estimateRidgePeriod(img, of);
    fpa::localNormalize(img);
    fpa::ImageU8 bin = fpa::enhanceAndBinarize(img, of, period);
    fpa::thin(bin);
    std::vector<fpa::Minutia> m = fpa::extractMinutiae(bin, of);
    if (m.size() < kMinMinutiae) {
      setError(piErr, msg, FPA_ERR_FEW_MINUTIAE, "too few minutiae");
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
    *pdScore = fpa::matchMinutiae(e, a);
    setError(piErrorCode, arErrorMessage, FPA_OK, "");
    return 1;
  } catch (...) {
    setError(piErrorCode, arErrorMessage, FPA_ERR_EXCEPTION, "exception in compare");
  }
  return 0;
}
