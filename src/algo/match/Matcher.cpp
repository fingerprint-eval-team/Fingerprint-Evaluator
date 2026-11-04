#include "match/Matcher.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;

float wrapPi(float a) {  // -π..π
  while (a > kPi) a -= 2 * kPi;
  while (a < -kPi) a += 2 * kPi;
  return a;
}

struct Vote {
  int64_t key;
  float rot, dx, dy;
};

struct Transform {
  float rot, dx, dy;
};

// 변환을 적용해 1:1 대응점 수를 센다 (거리 순 탐욕 매칭)
int countMatches(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const Transform& t,
                 const MatchParams& p) {
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
      cands.push_back({d2, (int)i, (int)j});
    }
  }
  std::sort(cands.begin(), cands.end(), [](const Cand& x, const Cand& y) { return x.d2 < y.d2; });
  std::vector<uint8_t> ue(e.size(), 0), ua(a.size(), 0);
  int n = 0;
  for (const auto& cd : cands) {
    if (ue[cd.i] || ua[cd.j]) continue;
    ue[cd.i] = ua[cd.j] = 1;
    ++n;
  }
  return n;
}
}  // namespace

double matchMinutiae(const std::vector<Minutia>& e, const std::vector<Minutia>& a, const MatchParams& p) {
  if (e.size() < 4 || a.size() < 4) return 0.0;
  const float rotBin = p.rotBinDeg * kPi / 180.f, maxRot = p.maxRotDeg * kPi / 180.f;

  // 1) Hough 투표: 모든 미뉴셔 쌍이 가리키는 (회전, 이동)
  std::vector<Vote> votes;
  votes.reserve(e.size() * a.size());
  for (const auto& me : e)
    for (const auto& ma : a) {
      // 끝점/분기점 종류는 노이즈가 많아 투표에는 사용하지 않는다
      float rot = wrapPi(ma.angle - me.angle);
      if (std::fabs(rot) > maxRot) continue;
      float c = std::cos(rot), s = std::sin(rot);
      float dx = ma.x - (me.x * c - me.y * s);
      float dy = ma.y - (me.x * s + me.y * c);
      int64_t rb = (int64_t)std::floor(rot / rotBin) + 64;
      int64_t xb = (int64_t)std::floor(dx / p.transBin) + 4096;
      int64_t yb = (int64_t)std::floor(dy / p.transBin) + 4096;
      votes.push_back({(rb << 32) | (xb << 16) | yb, rot, dx, dy});
    }
  if (votes.empty()) return 0.0;
  std::sort(votes.begin(), votes.end(), [](const Vote& x, const Vote& y) { return x.key < y.key; });

  // 2) bin별 득표 수와 평균 변환
  struct Bin {
    int count;
    Transform t;
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
    int n = (int)(j - i);
    bins.push_back({n, {(float)(sr / n), (float)(sx / n), (float)(sy / n)}});
    i = j;
  }
  size_t k = std::min<size_t>(bins.size(), (size_t)p.candidates);
  std::partial_sort(bins.begin(), bins.begin() + k, bins.end(),
                    [](const Bin& x, const Bin& y) { return x.count > y.count; });

  // 3) 상위 후보 변환마다 실제 대응점 수 계산
  int best = 0;
  for (size_t i = 0; i < k; ++i) best = std::max(best, countMatches(e, a, bins[i].t, p));

  // 4) 정규화: m² / (n_e · n_a), 대응 2개 이하는 우연으로 보고 감점
  double m = std::max(0, best - 2);
  double score = (m * m) / ((double)e.size() * (double)a.size());
  score = std::min(1.0, std::sqrt(score) * 1.25);
  return std::round(score * 1000.0) / 1000.0;
}

}  // namespace fpa
