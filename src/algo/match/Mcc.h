#pragma once
#include <vector>

#include "minutiae/Minutiae.h"

namespace fpa {

// Minutia Cylinder-Code (Cappelli, Ferrara, Maltoni, IEEE TPAMI 2010)
// 각 미뉴셔 주변 이웃 배치를 회전 불변 원통(공간 16x16 x 방향 6)으로 부호화하고,
// 지역 유사도를 LSS-R(relaxation)로 통합한다. 0(타인) ~ 1(동일), 반올림하지 않은 값.
double matchMcc(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

struct MccPair {
  int i, j;    // e[i] ↔ a[j]
  float sim;   // relaxation 후 지역 유사도
};
struct MccResult {
  double score = 0;
  std::vector<MccPair> pairs;  // 1:1 대응 후보, 유사도 내림차순
};
MccResult matchMccEx(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

}  // namespace fpa
