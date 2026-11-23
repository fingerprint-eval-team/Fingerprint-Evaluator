#include "match/Matcher.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "match/Geometry.h"
#include "match/Mcc.h"

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;
// 점수 융합 가중치 (AlgoBench 점수 덤프로 FVC 3개 DB에서 탐색, 결과 문서 참고)
constexpr double kFuseMcc = 0.85, kFuseAlign = 0.10, kFuseHough = 0.05;

float wrapPi(float a) {  // -π..π
  while (a > kPi) a -= 2 * kPi;
  while (a < -kPi) a += 2 * kPi;
  return a;
}

struct Vote {
  int64_t key;
  float rot, dx, dy;
};

struct MatchPair {
  int i, j;
};

// 변환을 적용해 1:1 대응점을 찾는다 (거리 순 탐욕 매칭)
int countMatches(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const Rigid& t, const MatchParams& p,
                 std::vector<MatchPair>* out = nullptr) {
  const float c = std::cos(t.rot), s = std::sin(t.rot);
  const float distTol2 = p.distTol * p.distTol, angTol = p.angTolDeg * kPi / 180.f;
  struct Cand {
    float d2;
    int i, j;
  };
  std::vector<Cand> cands;
  for (size_t i = 0; i < e.size(); ++i) {
    float x = e[i].x * c - e[i].y * s + t.dx;
    float y = e[i].x * s + e[i].y * c + t.dy;
    float ang = e[i].angle + t.rot;
    for (size_t j = 0; j < a.size(); ++j) {
      float dx = x - a[j].x, dy = y - a[j].y;
      float d2 = dx * dx + dy * dy;
      if (d2 > distTol2) continue;
      if (std::fabs(wrapPi(ang - a[j].angle)) > angTol) continue;
      cands.push_back({d2, static_cast<int>(i), static_cast<int>(j)});
    }
  }
  std::sort(cands.begin(), cands.end(), [](const Cand& x, const Cand& y) { return x.d2 < y.d2; });
  std::vector<uint8_t> ue(e.size(), 0), ua(a.size(), 0);
  int n = 0;
  for (const auto& cd : cands) {
    if (ue[cd.i] || ua[cd.j]) continue;
    ue[cd.i] = ua[cd.j] = 1;
    if (out) out->push_back({cd.i, cd.j});
    ++n;
  }
  return n;
}

// 대응 쌍으로 최소제곱 강체 변환
Rigid fitRigid(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const std::vector<MatchPair>& pr) {
  double ex = 0, ey = 0, ax = 0, ay = 0;
  for (const auto& q : pr) {
    ex += e[q.i].x;
    ey += e[q.i].y;
    ax += a[q.j].x;
    ay += a[q.j].y;
  }
  const double n = static_cast<double>(pr.size());
  ex /= n;
  ey /= n;
  ax /= n;
  ay /= n;
  double sxx = 0, sxy = 0;
  for (const auto& q : pr) {
    double px = e[q.i].x - ex, py = e[q.i].y - ey, qx = a[q.j].x - ax, qy = a[q.j].y - ay;
    sxx += px * qx + py * qy;
    sxy += px * qy - py * qx;
  }
  Rigid t;
  t.rot = static_cast<float>(std::atan2(sxy, sxx));
  const double c = std::cos(t.rot), s = std::sin(t.rot);
  t.dx = static_cast<float>(ax - (ex * c - ey * s));
  t.dy = static_cast<float>(ay - (ex * s + ey * c));
  return t;
}

// 겹치는 영역에 있는 미뉴셔 수: e를 변환했을 때 a의 볼록 껍질(+여유) 안에 드는 e 수, 그 반대도
void overlapCounts(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const Rigid& t,
                   const std::vector<Pt>& hullA, int& ne, int& na) {
  const float c = std::cos(t.rot), s = std::sin(t.rot);
  std::vector<Minutia> te(e);
  for (auto& m : te) {
    float x = m.x * c - m.y * s + t.dx, y = m.x * s + m.y * c + t.dy;
    m.x = x;
    m.y = y;
  }
  std::vector<Pt> hullE = hullOf(te);
  const float margin = 15.f;
  ne = 0;
  na = 0;
  for (const auto& m : te) ne += nearHull(hullA, m.x, m.y, margin);
  for (const auto& m : a) na += nearHull(hullE, m.x, m.y, margin);
}

// 후보 변환들을 검증: 대응 수 → 최소제곱 재정렬 2회 → 겹침 영역 정규화 점수
double consolidate(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const std::vector<Rigid>& cands,
                   const MatchParams& p) {
  // 1) 대응 수가 가장 많은 변환 (재정렬 포함)
  int best = 0;
  Rigid bestT;
  for (const auto& t0 : cands) {
    Rigid t = t0;
    std::vector<MatchPair> pr;
    int m = countMatches(e, a, t, p, &pr);
    for (int it = 0; it < 2 && m >= 3; ++it) {
      Rigid t2 = fitRigid(e, a, pr);
      std::vector<MatchPair> pr2;
      int m2 = countMatches(e, a, t2, p, &pr2);
      if (m2 < m) break;
      t = t2;
      m = m2;
      pr.swap(pr2);
    }
    if (m > best) {
      best = m;
      bestT = t;
    }
  }
  if (best < 3) return 0.0;
  // 2) 겹치는 영역의 미뉴셔 수로 정규화 (부분 지문에서 겹치지 않는 미뉴셔 때문에 점수가 깎이지 않도록)
  int ne = 0, na = 0;
  overlapCounts(e, a, bestT, hullOf(a), ne, na);
  const double kMinOverlap = 12.0;  // 겹침이 작을 때 소수 대응으로 고득점 방지
  double de = std::max<double>({static_cast<double>(ne), static_cast<double>(best), kMinOverlap});
  double da = std::max<double>({static_cast<double>(na), static_cast<double>(best), kMinOverlap});
  double m = best - 2.0;  // 대응 2개 이하는 우연
  return std::clamp(m * m / (de * da), 0.0, 1.0);
}

std::vector<Rigid> houghCandidates(const std::vector<Minutia>& e, const std::vector<Minutia>& a,
                                   const MatchParams& p) {
  const float rotBin = p.rotBinDeg * kPi / 180.f, maxRot = p.maxRotDeg * kPi / 180.f;
  std::vector<Vote> votes;
  votes.reserve(e.size() * a.size());
  for (const auto& me : e)
    for (const auto& ma : a) {
      float rot = wrapPi(ma.angle - me.angle);
      if (std::fabs(rot) > maxRot) continue;
      float c = std::cos(rot), s = std::sin(rot);
      float dx = ma.x - (me.x * c - me.y * s);
      float dy = ma.y - (me.x * s + me.y * c);
      int64_t rb = static_cast<int64_t>(std::floor(rot / rotBin)) + 64;
      int64_t xb = static_cast<int64_t>(std::floor(dx / p.transBin)) + 4096;
      int64_t yb = static_cast<int64_t>(std::floor(dy / p.transBin)) + 4096;
      votes.push_back({(rb << 32) | (xb << 16) | yb, rot, dx, dy});
    }
  std::sort(votes.begin(), votes.end(), [](const Vote& x, const Vote& y) { return x.key < y.key; });
  struct Bin {
    int count;
    Rigid t;
  };
  std::vector<Bin> bins;
  for (size_t i = 0; i < votes.size();) {
    size_t j = i;
    double sr = 0, sx = 0, sy = 0;
    while (j < votes.size() && votes[j].key == votes[i].key) {
      sr += votes[j].rot;
      sx += votes[j].dx;
      sy += votes[j].dy;
      ++j;
    }
    int n = static_cast<int>(j - i);
    bins.push_back({n, {static_cast<float>(sr / n), static_cast<float>(sx / n), static_cast<float>(sy / n)}});
    i = j;
  }
  size_t k = std::min<size_t>(bins.size(), static_cast<size_t>(p.candidates));
  std::partial_sort(bins.begin(), bins.begin() + k, bins.end(),
                    [](const Bin& x, const Bin& y) { return x.count > y.count; });
  std::vector<Rigid> out;
  for (size_t i = 0; i < k; ++i) out.push_back(bins[i].t);
  return out;
}

// MCC 상위 지역 대응 쌍 각각이 가리키는 강체 변환
std::vector<Rigid> mccCandidates(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const MccResult& r,
                                 size_t k) {
  std::vector<Rigid> out;
  for (size_t t = 0; t < r.pairs.size() && t < k; ++t) {
    const Minutia& me = e[r.pairs[t].i];
    const Minutia& ma = a[r.pairs[t].j];
    Rigid g;
    g.rot = wrapPi(ma.angle - me.angle);
    float c = std::cos(g.rot), s = std::sin(g.rot);
    g.dx = ma.x - (me.x * c - me.y * s);
    g.dy = ma.y - (me.x * s + me.y * c);
    out.push_back(g);
  }
  return out;
}

double round3(double v) { return std::round(v * 1000.0) / 1000.0; }
}  // namespace

// 기존(v1) Hough 매처: 전체 미뉴셔 수로 정규화 (비교 기준용)
double matchMinutiae(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const MatchParams& p) {
  if (e.size() < 4 || a.size() < 4) return 0.0;
  std::vector<Rigid> cands = houghCandidates(e, a, p);
  int best = 0;
  for (const auto& t : cands) best = std::max(best, countMatches(e, a, t, p));
  double m = std::max(0, best - 2);
  double score = (m * m) / (static_cast<double>(e.size()) * static_cast<double>(a.size()));
  score = std::min(1.0, std::sqrt(score) * 1.25);
  return round3(score);
}

double matchHough2(const std::vector<Minutia>& e, const std::vector<Minutia>& a) {
  if (e.size() < 4 || a.size() < 4) return 0.0;
  MatchParams p;
  return consolidate(e, a, houghCandidates(e, a, p), p);
}

double matchMccAlign(const std::vector<Minutia>& e, const std::vector<Minutia>& a) {
  if (e.size() < 4 || a.size() < 4) return 0.0;
  MatchParams p;
  MccResult r = matchMccEx(e, a);
  return consolidate(e, a, mccCandidates(e, a, r, 10), p);
}

double matchFused(const std::vector<Minutia>& e, const std::vector<Minutia>& a) {
  if (e.size() < 4 || a.size() < 4) return 0.0;
  MatchParams p;
  MccResult r = matchMccEx(e, a);
  double align = consolidate(e, a, mccCandidates(e, a, r, 10), p);
  double hough = matchMinutiae(e, a, p);
  return kFuseMcc * r.score + kFuseAlign * align + kFuseHough * hough;
}

double matchFinal(const std::vector<Minutia>& e, const std::vector<Minutia>& a) {
  // 융합 점수는 대부분 0~0.6에 모인다. 제곱근으로 낮은 구간을 넓혀 소수 셋째 자리 반올림 시 동점을 줄인다 (순위 불변)
  return round3(std::min(1.0, std::sqrt(std::max(0.0, matchFused(e, a)))));
}

std::vector<MatcherVariant> matcherVariants() {
  return {
      {"fused", matchFused},
      {"hough", [](const std::vector<Minutia>& e, const std::vector<Minutia>& a) { return matchMinutiae(e, a); }},
      {"hough2", matchHough2},
      {"mcc", matchMcc},
      {"mccAlign", matchMccAlign},
  };
}

}  // namespace fpa
