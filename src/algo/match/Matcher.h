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

// 개선 Hough: 최소제곱 재정렬 + 겹침 영역 정규화 (반올림 없음)
double matchHough2(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

// MCC 상위 지역 대응으로 정렬 후보를 만들고 Hough2와 같은 방식으로 검증 (반올림 없음)
double matchMccAlign(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

// 융합: MCC + MCC 정렬 검증 + Hough v1 가중합 (반올림 없음)
double matchFused(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

// 최종 점수: 융합 점수의 제곱근, 0..1, 소수 셋째 자리 (KISA_Compare 출력)
double matchFinal(const std::vector<Minutia>& e, const std::vector<Minutia>& a);

// 벤치 비교용 매처 목록 (AlgoBench에서 같은 쌍에 모두 적용해 비교)
using MatchFn = double (*)(const std::vector<Minutia>&, const std::vector<Minutia>&);
struct MatcherVariant {
  const char* name;
  MatchFn fn;
};
std::vector<MatcherVariant> matcherVariants();

}  // namespace fpa
