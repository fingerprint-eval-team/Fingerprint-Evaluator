#pragma once
#include <vector>

#include "enhance/Enhance.h"
#include "image/Image.h"

namespace fpa {

enum class MinutiaType : uint8_t { Ending = 1, Bifurcation = 2 };

struct Minutia {
  float x = 0, y = 0;
  float angle = 0;  // 0..2π
  MinutiaType type = MinutiaType::Ending;
  float quality = 0;  // 0..1
};

// 세선화 영상에서 Crossing Number로 미뉴셔 추출 + 거짓 미뉴셔 제거
std::vector<Minutia> extractMinutiae(const ImageU8& skeleton, const OrientationField& of, size_t maxCount = 120);

}  // namespace fpa
