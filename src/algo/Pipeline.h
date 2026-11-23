#pragma once
#include <vector>

#include "minutiae/Minutiae.h"

namespace fpa {

// 영상(w*h 바이트) → 미뉴셔. 반환값은 FpaError 코드, msg는 정적 문자열.
int extractFeatures(const unsigned char* image, int w, int h, std::vector<Minutia>& out, const char** msg);

// 두 미뉴셔 집합의 최종 점수 (0..1, 소수 셋째 자리)
double compareFeatures(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

}  // namespace fpa
