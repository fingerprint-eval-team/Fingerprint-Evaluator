// 처리: 정규화 → 방향장·전경 분할 → 융선 간격 추정 → Gabor 강조·이진화 → 세선화 → 미뉴셔 추출
#include "Pipeline.h"

#include "BioApi.h"
#include "enhance/Enhance.h"
#include "image/Image.h"
#include "match/Matcher.h"

namespace fpa {

namespace {
constexpr size_t kMinMinutiae = 8;
}

int extractFeatures(const unsigned char* image, int w, int h, std::vector<Minutia>& out, const char** msg) {
  *msg = "";
  ImageF img = toFloat(image, w, h);
  normalize(img);
  OrientationField of = computeOrientation(img);
  size_t fg = 0;
  for (auto v : of.mask) fg += v;
  if (fg < of.mask.size() / 20) {
    *msg = "no fingerprint area";
    return FPA_ERR_NO_FOREGROUND;
  }
  std::vector<float> period = estimateRidgePeriodMap(img, of);
  localNormalize(img);
  ImageU8 bin = enhanceAndBinarize(img, of, period);
  thin(bin);
  out = extractMinutiae(bin, of);
  if (out.size() < kMinMinutiae) {
    *msg = "too few minutiae";
    return FPA_ERR_FEW_MINUTIAE;
  }
  return FPA_OK;
}

double compareFeatures(const std::vector<Minutia>& e, const std::vector<Minutia>& a) {
  return matchFinal(e, a);
}

}  // namespace fpa
