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

// 계산할 수 없는 지표 값 (분모가 0이거나 조건을 만족하는 임계값이 없음)
constexpr double kNA = -1.0;
inline bool isNA(double v) { return v < 0; }

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
  size_t nInternal = 0;                  // 평가 프로그램 쪽 오류(INTERNAL): FTE 분모·분자에서 제외
  double fte = 0;                        // nEnroll == 0이면 kNA
  // 비교
  size_t nG = 0, nI = 0;                 // 계획된 시도 수
  size_t nGEF = 0, nGMF = 0, nIEF = 0, nIMF = 0;
  size_t nGCalled = 0, nICalled = 0;     // 실제로 DLL을 호출한 수
  size_t nGCallFail = 0, nICallFail = 0;
  size_t gOk = 0, iOk = 0;
  double rejG = 0, rejI = 0;             // 호출 건수가 0이면 kNA
  // 오류율. 계산할 수 없으면 kNA (예: 부분 평가에서 genuine 또는 impostor 쌍이 없음)
  bool ratesDefined = false;             // N_G > 0 && N_I > 0
  bool starDefined = false;              // 성공 점수가 genuine·impostor 모두 있음
  double eer = kNA, eerLow = kNA, eerHigh = kNA, eerT = 0;
  bool eerExact = false;
  double eerStar = kNA, eerStarT = 0;
  double zeroFMR = kNA, zeroFNMR = kNA, fmr100 = kNA, fmr1000 = kNA, fmr10000 = kNA;
  // 시간·크기
  TimeStat enrollTime, attemptTime, matchAll, matchG, matchI;
  double tplMean = 0, tplMax = 0;
  // 그래프 데이터 (ratesDefined가 false면 det·rates는 비어 있음)
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
