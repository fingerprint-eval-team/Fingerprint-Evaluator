#pragma once
#include <vector>

#include "minutiae/Minutiae.h"

namespace fpa {

struct MatchParams {
  float rotBinDeg = 6.f;     // Hough 회전 bin
  float transBin = 8.f;      // Hough 이동 bin (px)
  float maxRotDeg = 60.f;    // 허용 회전 범위
  float distTol = 14.f;      // 대응점 거리 허용 오차 (px)
  float angTolDeg = 25.f;    // 대응점 방향 허용 오차
  int candidates = 12;       // 정밀 검사할 Hough 후보 수
};

// 0(타인) ~ 1(동일) 점수, 소수 셋째 자리 반올림
double matchMinutiae(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const MatchParams& p = {});

}  // namespace fpa
