#include "match/Mcc.h"

#include <algorithm>
#include <cmath>
#include <cstdint>

#include "match/Geometry.h"

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;

// 논문 기본 파라미터 (500dpi)
constexpr float kR = 70.f;             // 원통 반지름
constexpr int kNs = 16;                // 공간 셀 수 (한 변)
constexpr int kNd = 6;                 // 방향 셀 수
constexpr float kSigmaS = 28.f / 3.f;  // 공간 기여 표준편차
constexpr float kSigmaD = 2 * kPi / 9; // 방향 기여 표준편차
constexpr float kMuPsi = 0.01f, kTauPsi = 400.f;
constexpr float kOmega = 50.f;         // 볼록 껍질 확장 거리
constexpr float kMinVc = 0.75f;        // 유효 원통: 유효 셀 비율
constexpr int kMinM = 2;               // 유효 원통: 최소 이웃 수
constexpr float kMinMe = 0.60f;        // 비교 가능: 공통 유효 셀 비율
constexpr float kDeltaTheta = kPi / 2; // 비교 가능: 방향 차 상한
// LSS-R
constexpr float kWr = 0.5f;
constexpr int kNrel = 5;
constexpr float kMuP = 20.f, kTauP = 0.4f;
constexpr int kMinNp = 4, kMaxNp = 12;

constexpr float kDeltaS = 2 * kR / kNs;
constexpr float kDeltaD = 2 * kPi / kNd;
constexpr int kCells = kNs * kNs;

float dPi(float a, float b) {  // a - b 를 [-π, π)로
  float d = a - b;
  while (d >= kPi) d -= 2 * kPi;
  while (d < -kPi) d += 2 * kPi;
  return d;
}

float sigmoid(float v, float mu, float tau) { return 1.f / (1.f + std::exp(-tau * (v - mu))); }

struct Cylinder {
  bool valid = false;
  std::vector<uint8_t> cellValid;  // kCells
  std::vector<float> v;            // kCells * kNd
  int validCount = 0;              // 유효 공간 셀 수
};

std::vector<Cylinder> buildCylinders(const std::vector<Minutia>& m) {
  std::vector<Pt> hull = hullOf(m);
  const float gsNorm = 1.f / (kSigmaS * std::sqrt(2 * kPi));
  const float inv2s2 = 1.f / (2 * kSigmaS * kSigmaS);
  const float maxNb = kR + 3 * kSigmaS;
  const float cellTol2 = 9 * kSigmaS * kSigmaS;
  const float erfK = 1.f / (kSigmaD * std::sqrt(2.f));
  int cellsInR = 0;
  for (int i = 0; i < kNs; ++i)
    for (int j = 0; j < kNs; ++j) {
      float u = (i - (kNs - 1) / 2.f) * kDeltaS, w = (j - (kNs - 1) / 2.f) * kDeltaS;
      if (u * u + w * w <= kR * kR) ++cellsInR;
    }

  std::vector<Cylinder> cyl(m.size());
  for (size_t c = 0; c < m.size(); ++c) {
    const Minutia& mc = m[c];
    Cylinder& cy = cyl[c];
    // 이웃과 방향 기여(셀 위치와 무관하므로 미리 계산)
    struct Nb {
      float x, y;
      float cd[kNd];
    };
    std::vector<Nb> nbs;
    for (size_t t = 0; t < m.size(); ++t) {
      if (t == c) continue;
      float dx = m[t].x - mc.x, dy = m[t].y - mc.y;
      if (dx * dx + dy * dy > maxNb * maxNb) continue;
      Nb nb{m[t].x, m[t].y, {}};
      float dphi = dPi(mc.angle, m[t].angle);
      for (int k = 0; k < kNd; ++k) {
        float phiK = -kPi + (k + 0.5f) * kDeltaD;
        float alpha = dPi(phiK, dphi);
        nb.cd[k] = 0.5f * (std::erf((alpha + kDeltaD / 2) * erfK) - std::erf((alpha - kDeltaD / 2) * erfK));
      }
      nbs.push_back(nb);
    }
    cy.cellValid.assign(kCells, 0);
    cy.v.assign(static_cast<size_t>(kCells) * kNd, 0.f);
    const float cs = std::cos(mc.angle), sn = std::sin(mc.angle);
    for (int i = 0; i < kNs; ++i)
      for (int j = 0; j < kNs; ++j) {
        float u = (i - (kNs - 1) / 2.f) * kDeltaS, w = (j - (kNs - 1) / 2.f) * kDeltaS;
        if (u * u + w * w > kR * kR) continue;
        float px = mc.x + cs * u - sn * w, py = mc.y + sn * u + cs * w;
        if (!nearHull(hull, px, py, kOmega)) continue;
        int cell = i * kNs + j;
        cy.cellValid[cell] = 1;
        ++cy.validCount;
        float acc[kNd] = {};
        for (const auto& nb : nbs) {
          float dx = nb.x - px, dy = nb.y - py;
          float d2 = dx * dx + dy * dy;
          if (d2 > cellTol2) continue;
          float g = gsNorm * std::exp(-d2 * inv2s2);
          for (int k = 0; k < kNd; ++k) acc[k] += g * nb.cd[k];
        }
        for (int k = 0; k < kNd; ++k) cy.v[static_cast<size_t>(cell) * kNd + k] = sigmoid(acc[k], kMuPsi, kTauPsi);
      }
    int nearCount = 0;
    for (const auto& nb : nbs) {
      float dx = nb.x - mc.x, dy = nb.y - mc.y;
      if (dx * dx + dy * dy <= (kR + 3 * kSigmaS) * (kR + 3 * kSigmaS)) ++nearCount;
    }
    cy.valid = cy.validCount >= kMinVc * cellsInR && nearCount >= kMinM;
  }
  return cyl;
}

float localSimilarity(const Cylinder& a, const Cylinder& b, float angA, float angB, int cellsInR) {
  if (std::fabs(dPi(angA, angB)) > kDeltaTheta) return 0.f;
  int common = 0;
  double na = 0, nb = 0, nd = 0;
  for (int c = 0; c < kCells; ++c) {
    if (!a.cellValid[c] || !b.cellValid[c]) continue;
    ++common;
    const float* va = &a.v[static_cast<size_t>(c) * kNd];
    const float* vb = &b.v[static_cast<size_t>(c) * kNd];
    for (int k = 0; k < kNd; ++k) {
      na += va[k] * va[k];
      nb += vb[k] * vb[k];
      float d = va[k] - vb[k];
      nd += d * d;
    }
  }
  if (common < kMinMe * cellsInR) return 0.f;
  double den = std::sqrt(na) + std::sqrt(nb);
  if (den <= 0) return 0.f;
  return static_cast<float>(1.0 - std::sqrt(nd) / den);
}

int countCellsInR() {
  int n = 0;
  for (int i = 0; i < kNs; ++i)
    for (int j = 0; j < kNs; ++j) {
      float u = (i - (kNs - 1) / 2.f) * kDeltaS, w = (j - (kNs - 1) / 2.f) * kDeltaS;
      if (u * u + w * w <= kR * kR) ++n;
    }
  return n;
}
}  // namespace

double matchMcc(const std::vector<Minutia>& e, const std::vector<Minutia>& a) { return matchMccEx(e, a).score; }

MccResult matchMccEx(const std::vector<Minutia>& e, const std::vector<Minutia>& a) {
  MccResult res;
  if (e.size() < 2 || a.size() < 2) return res;
  static const int cellsInR = countCellsInR();
  std::vector<Cylinder> ce = buildCylinders(e), ca = buildCylinders(a);

  // 1) 지역 유사도 행렬
  struct Cand {
    float s;
    int i, j;
  };
  std::vector<Cand> cands;
  for (size_t i = 0; i < e.size(); ++i) {
    if (!ce[i].valid) continue;
    for (size_t j = 0; j < a.size(); ++j) {
      if (!ca[j].valid) continue;
      float s = localSimilarity(ce[i], ca[j], e[i].angle, a[j].angle, cellsInR);
      if (s > 0) cands.push_back({s, static_cast<int>(i), static_cast<int>(j)});
    }
  }
  if (cands.empty()) return res;
  std::sort(cands.begin(), cands.end(), [](const Cand& x, const Cand& y) { return x.s > y.s; });

  // 2) 1:1 대응으로 상위 nR 쌍 선택
  const size_t nR = std::min(e.size(), a.size());
  std::vector<uint8_t> ue(e.size(), 0), ua(a.size(), 0);
  std::vector<Cand> sel;
  for (const auto& c : cands) {
    if (ue[c.i] || ua[c.j]) continue;
    ue[c.i] = ua[c.j] = 1;
    sel.push_back(c);
    if (sel.size() >= nR) break;
  }
  const size_t n = sel.size();

  // 3) relaxation: 다른 쌍과 기하적으로 맞는 쌍의 유사도를 강화
  std::vector<float> rho(n * n, 0.f);
  for (size_t t = 0; t < n; ++t)
    for (size_t k = 0; k < n; ++k) {
      if (t == k) continue;
      const Minutia &at = e[sel[t].i], &ak = e[sel[k].i], &bt = a[sel[t].j], &bk = a[sel[k].j];
      float da = std::hypot(at.x - ak.x, at.y - ak.y), db = std::hypot(bt.x - bk.x, bt.y - bk.y);
      float d1 = std::fabs(da - db);
      float d2 = std::fabs(dPi(dPi(at.angle, ak.angle), dPi(bt.angle, bk.angle)));
      float ra = dPi(at.angle, std::atan2(ak.y - at.y, ak.x - at.x));
      float rb = dPi(bt.angle, std::atan2(bk.y - bt.y, bk.x - bt.x));
      float d3 = std::fabs(dPi(ra, rb));
      rho[t * n + k] = sigmoid(d1, 5.f, -8.f / 5.f) * sigmoid(d2, kPi / 12, -30.f) * sigmoid(d3, kPi / 12, -30.f);
    }
  std::vector<float> lam(n), lam0(n);
  for (size_t t = 0; t < n; ++t) lam[t] = lam0[t] = sel[t].s;
  if (n > 1) {
    std::vector<float> nxt(n);
    for (int it = 0; it < kNrel; ++it) {
      for (size_t t = 0; t < n; ++t) {
        double acc = 0;
        for (size_t k = 0; k < n; ++k)
          if (k != t) acc += rho[t * n + k] * lam[k];
        nxt[t] = kWr * lam[t] + (1 - kWr) * static_cast<float>(acc / (n - 1));
      }
      lam.swap(nxt);
    }
  }

  // 4) 효율(λ_end/λ_0)이 높은 nP 쌍의 평균
  const size_t nP = static_cast<size_t>(
      kMinNp + std::lround(sigmoid(static_cast<float>(std::min(e.size(), a.size())), kMuP, kTauP) * (kMaxNp - kMinNp)));
  std::vector<size_t> idx(n);
  for (size_t t = 0; t < n; ++t) idx[t] = t;
  std::sort(idx.begin(), idx.end(), [&](size_t x, size_t y) { return lam[x] / lam0[x] > lam[y] / lam0[y]; });
  const size_t take = std::min(nP, n);
  double sum = 0;
  for (size_t t = 0; t < take; ++t) sum += lam[idx[t]];
  // 대응 후보가 nP보다 적으면 부족분은 0으로 본다 (적은 대응으로 높은 점수 방지)
  res.score = sum / nP;
  // 정렬 후보: relaxation 후 유사도 순
  std::vector<size_t> byLam(n);
  for (size_t t = 0; t < n; ++t) byLam[t] = t;
  std::sort(byLam.begin(), byLam.end(), [&](size_t x, size_t y) { return lam[x] > lam[y]; });
  for (size_t t : byLam) res.pairs.push_back({sel[t].i, sel[t].j, lam[t]});
  return res;
}

}  // namespace fpa
