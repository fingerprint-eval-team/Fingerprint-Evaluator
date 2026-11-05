#include "metrics/MetricsCalculator.h"

#include <algorithm>
#include <cmath>

namespace fpe {

namespace {

struct Rates {
  const std::vector<double>& g;  // 정렬된 genuine 점수
  const std::vector<double>& i;  // 정렬된 impostor 점수
  double gFail;                  // FNMR 분자에 더할 실패 수 (N_G,EF + N_G,MF)
  double nG, nI;                 // 분모

  double fmr(double t) const {
    if (nI <= 0) return 0;
    size_t ge = i.end() - std::lower_bound(i.begin(), i.end(), t);
    return ge / nI;
  }
  double fnmr(double t) const {
    if (nG <= 0) return 0;
    size_t lt = std::lower_bound(g.begin(), g.end(), t) - g.begin();
    return (gFail + lt) / nG;
  }
};

std::vector<double> candidates(const std::vector<double>& g, const std::vector<double>& i) {
  std::vector<double> c;
  c.reserve(g.size() + i.size() + 2);
  c.push_back(0.0);
  c.insert(c.end(), g.begin(), g.end());
  c.insert(c.end(), i.begin(), i.end());
  c.push_back(1.0 + 1e-9);  // 모든 점수보다 큰 임계값
  std::sort(c.begin(), c.end());
  c.erase(std::unique(c.begin(), c.end()), c.end());
  return c;
}

struct EerResult {
  double eer, low, high, t;
  bool exact;
};

// FVC 정의: t1 = max{t | FNMR(t) <= FMR(t)}, t2 = min{t | FNMR(t) >= FMR(t)}
EerResult computeEer(const Rates& r, const std::vector<double>& cand, bool fvc) {
  EerResult e{1, 1, 1, 0, false};
  if (!fvc) {
    double best = 2;
    for (double t : cand) {
      double a = r.fmr(t), b = r.fnmr(t);
      if (std::fabs(a - b) < best) {
        best = std::fabs(a - b);
        e = {(a + b) / 2, std::min(a, b), std::max(a, b), t, a == b};
      }
    }
    return e;
  }
  bool has1 = false, has2 = false;
  double t1 = 0, t2 = 0;
  for (double t : cand) {
    double a = r.fmr(t), b = r.fnmr(t);
    if (a == b) return {a, a, a, t, true};
    if (b <= a) {
      t1 = t;
      has1 = true;
    }
    if (b >= a && !has2) {
      t2 = t;
      has2 = true;
    }
  }
  if (!has1 && !has2) return e;
  if (!has1) return {(r.fmr(t2) + r.fnmr(t2)) / 2, r.fmr(t2), r.fnmr(t2), t2, false};
  if (!has2) return {(r.fmr(t1) + r.fnmr(t1)) / 2, r.fnmr(t1), r.fmr(t1), t1, false};
  double s1 = r.fmr(t1) + r.fnmr(t1), s2 = r.fmr(t2) + r.fnmr(t2);
  if (s1 <= s2) return {s1 / 2, r.fnmr(t1), r.fmr(t1), t1, false};
  return {s2 / 2, r.fmr(t2), r.fnmr(t2), t2, false};
}

// FMR <= limit 인 임계값 중 최저 FNMR
double lowestFnmrAt(const Rates& r, const std::vector<double>& cand, double limit) {
  double best = 1.0;
  for (double t : cand)
    if (r.fmr(t) <= limit + 1e-12) best = std::min(best, r.fnmr(t));
  return best;
}

void addTime(TimeStat& s, int64_t us) {
  double ms = us / 1000.0;
  s.meanMs += ms;
  s.maxMs = std::max(s.maxMs, ms);
  ++s.n;
}
void finish(TimeStat& s) {
  if (s.n) s.meanMs /= (double)s.n;
}

std::vector<double> histogram(const std::vector<double>& v) {
  std::vector<double> h(100, 0.0);
  for (double s : v) h[std::min<size_t>(99, (size_t)(s * 100))] += 1;
  if (!v.empty())
    for (double& x : h) x /= (double)v.size();
  return h;
}

}  // namespace

MetricsResult MetricsCalculator::compute(const std::vector<ExtractRecord>& ex, const std::vector<MatchRecord>& mt,
                                         size_t plannedG, size_t plannedI, const MetricOptions& opt) {
  MetricsResult m;
  double tplSum = 0;
  size_t tplN = 0;
  for (const auto& r : ex) {
    if (r.status == Status::Aborted) continue;
    bool ok = r.status == Status::OK;
    if (r.kind == 'E') {
      ++m.nEnroll;
      if (!ok) {
        ++m.nEF;
        ++m.enrollFailBy[r.status];
      } else {
        addTime(m.enrollTime, r.elapsedUs);
        tplSum += r.size;
        m.tplMax = std::max(m.tplMax, (double)r.size);
        ++tplN;
      }
    } else {
      ++m.nAttemptExtract;
      if (!ok) {
        ++m.nAF;
      } else {
        addTime(m.attemptTime, r.elapsedUs);
      }
    }
  }
  m.fte = m.nEnroll ? (double)m.nEF / m.nEnroll : 0;
  m.tplMean = tplN ? tplSum / tplN : 0;

  std::vector<double> g, i;
  for (const auto& r : mt) {
    if (r.status == Status::Aborted) continue;
    bool gen = r.type == 'G';
    if (r.status == Status::SkipEF) {
      ++(gen ? m.nGEF : m.nIEF);
      continue;
    }
    if (r.status == Status::SkipAF) {  // 비교용 템플릿 실패 → matching 실패로 집계
      ++(gen ? m.nGMF : m.nIMF);
      continue;
    }
    ++(gen ? m.nGCalled : m.nICalled);
    if (r.status == Status::OK) {
      (gen ? g : i).push_back(r.score);
      addTime(m.matchAll, r.elapsedUs);
      addTime(gen ? m.matchG : m.matchI, r.elapsedUs);
    } else {
      ++(gen ? m.nGCallFail : m.nICallFail);
      ++(gen ? m.nGMF : m.nIMF);
    }
  }
  finish(m.enrollTime);
  finish(m.attemptTime);
  finish(m.matchAll);
  finish(m.matchG);
  finish(m.matchI);
  std::sort(g.begin(), g.end());
  std::sort(i.begin(), i.end());
  m.gOk = g.size();
  m.iOk = i.size();
  m.nG = plannedG ? plannedG : m.nGEF + m.nGMF + m.gOk;
  m.nI = plannedI ? plannedI : m.nIEF + m.nIMF + m.iOk;
  m.rejG = m.nGCalled ? (double)m.nGCallFail / m.nGCalled : 0;
  m.rejI = m.nICalled ? (double)m.nICallFail / m.nICalled : 0;

  double nG = (double)m.nG, nI = (double)m.nI, gFail = (double)(m.nGEF + m.nGMF);
  if (!opt.predefinedDenominator) {  // 등록 실패분을 분모·분자에서 제외
    nG -= (double)m.nGEF;
    nI -= (double)m.nIEF;
    gFail -= (double)m.nGEF;
  }
  Rates r{g, i, gFail, nG, nI};
  auto cand = candidates(g, i);
  EerResult e = computeEer(r, cand, opt.fvcEer);
  m.eer = e.eer;
  m.eerLow = e.low;
  m.eerHigh = e.high;
  m.eerT = e.t;
  m.eerExact = e.exact;

  // EER*: 알고리즘이 OK로 돌려준 점수만 사용 (분모·분자 모두 성공 건 기준)
  Rates rs{g, i, 0.0, (double)g.size(), (double)i.size()};
  EerResult es = computeEer(rs, cand, opt.fvcEer);
  m.eerStar = es.eer;
  m.eerStarT = es.t;

  m.zeroFMR = lowestFnmrAt(r, cand, 0.0);
  m.fmr100 = lowestFnmrAt(r, cand, 0.01);
  m.fmr1000 = lowestFnmrAt(r, cand, 0.001);
  m.fmr10000 = lowestFnmrAt(r, cand, 0.0001);
  m.zeroFNMR = -1;
  for (double t : cand)
    if (r.fnmr(t) == 0) m.zeroFNMR = m.zeroFNMR < 0 ? r.fmr(t) : std::min(m.zeroFNMR, r.fmr(t));

  for (double t : cand) m.det.push_back({t, r.fmr(t), r.fnmr(t)});
  for (int k = 0; k <= 1000; ++k) {
    double t = k / 1000.0;
    m.rates.push_back({t, r.fmr(t), r.fnmr(t)});
  }
  m.histG = histogram(g);
  m.histI = histogram(i);
  return m;
}

}  // namespace fpe
