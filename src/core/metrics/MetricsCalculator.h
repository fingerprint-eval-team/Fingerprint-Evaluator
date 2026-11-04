#pragma once
#include <cstddef>
#include <map>
#include <vector>

#include "result/ResultStore.h"

namespace fpe {

struct MetricOptions {
  bool fvcEer = true;                 // true: FVC(t1,t2) 추정, false: |FMR-FNMR| 최소 지점
  bool predefinedDenominator = true;  // true: N_G/N_I = 계획된 전체 시도 수, false: 등록 실패 제외
};

struct CurvePoint {
  double t, fmr, fnmr;
};

struct TimeStat {
  double meanMs = 0;
  double maxMs = 0;
  size_t n = 0;
};

struct MetricsResult {
  // 등록
  size_t nEnroll = 0, nEF = 0;
  std::map<Status, size_t> enrollFailBy;
  size_t nAttemptExtract = 0, nAF = 0;
  double fte = 0;
  // 비교
  size_t nG = 0, nI = 0;                 // 계획된 시도 수
  size_t nGEF = 0, nGMF = 0, nIEF = 0, nIMF = 0;
  size_t nGCalled = 0, nICalled = 0;     // 실제로 DLL을 호출한 수
  size_t nGCallFail = 0, nICallFail = 0;
  size_t gOk = 0, iOk = 0;
  double rejG = 0, rejI = 0;
  // 오류율
  double eer = 0, eerLow = 0, eerHigh = 0, eerT = 0;
  bool eerExact = false;
  double eerStar = 0, eerStarT = 0;
  double zeroFMR = 0, zeroFNMR = -1, fmr100 = 0, fmr1000 = 0, fmr10000 = 0;  // -1 = 정의 불가
  // 시간·크기
  TimeStat enrollTime, attemptTime, matchAll, matchG, matchI;
  double tplMean = 0, tplMax = 0;
  // 그래프 데이터
  std::vector<CurvePoint> det;    // 모든 후보 임계값
  std::vector<CurvePoint> rates;  // t = 0.000 ~ 1.000, 0.001 간격
  std::vector<double> histG, histI;  // 100구간 비율
};

class MetricsCalculator {
 public:
  static MetricsResult compute(const std::vector<ExtractRecord>& ex, const std::vector<MatchRecord>& mt,
                               size_t plannedGenuine, size_t plannedImpostor, const MetricOptions& opt);
};

}  // namespace fpe
