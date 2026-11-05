#include "enhance/Enhance.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;
constexpr float kPeakRatio = 0.15f;   // 간격 채택: 최대 주기 파워 비율 하한
constexpr float kSigmaRatio = 0.40f;  // Gabor σ = 0.40·간격 (간격 9 → σ≈3.6)

// 블록 마스크 정리: 가장 큰 연결 영역만 남기고 구멍 메우기, 경계 1블록 깎기
void morph(std::vector<uint8_t>& m, int bw, int bh, bool dilate) {
  std::vector<uint8_t> t = m;
  for (int y = 0; y < bh; ++y)
    for (int x = 0; x < bw; ++x) {
      bool any = false, all = true;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          int xx = x + dx, yy = y + dy;
          bool v = xx >= 0 && yy >= 0 && xx < bw && yy < bh && m[(size_t)yy * bw + xx];
          any |= v;
          all &= v;
        }
      t[(size_t)y * bw + x] = dilate ? any : all;
    }
  m.swap(t);
}

void cleanMask(std::vector<uint8_t>& m, int bw, int bh) {
  auto idx = [bw](int x, int y) { return (size_t)y * bw + x; };
  // 0) 닫힘(closing): 지문 안쪽의 흐린 영역이 끊기지 않도록
  morph(m, bw, bh, true);
  morph(m, bw, bh, true);
  morph(m, bw, bh, false);
  morph(m, bw, bh, false);
  // 1) 열림(opening) 비슷한 처리: 이웃이 적은 고립 블록 제거
  std::vector<uint8_t> t = m;
  for (int y = 0; y < bh; ++y)
    for (int x = 0; x < bw; ++x) {
      int cnt = 0;
      for (int dy = -1; dy <= 1; ++dy)
        for (int dx = -1; dx <= 1; ++dx) {
          int xx = x + dx, yy = y + dy;
          if (xx >= 0 && yy >= 0 && xx < bw && yy < bh && m[idx(xx, yy)]) ++cnt;
        }
      t[idx(x, y)] = cnt >= 5 ? 1 : 0;
    }
  m.swap(t);
  // 2) 가장 큰 연결 영역
  std::vector<int> label(m.size(), 0);
  int best = 0, bestSize = 0, cur = 0;
  for (int y = 0; y < bh; ++y)
    for (int x = 0; x < bw; ++x) {
      if (!m[idx(x, y)] || label[idx(x, y)]) continue;
      ++cur;
      int size = 0;
      std::queue<std::pair<int, int>> q;
      q.push({x, y});
      label[idx(x, y)] = cur;
      while (!q.empty()) {
        auto [cx, cy] = q.front();
        q.pop();
        ++size;
        const int nx[4] = {1, -1, 0, 0}, ny[4] = {0, 0, 1, -1};
        for (int k = 0; k < 4; ++k) {
          int xx = cx + nx[k], yy = cy + ny[k];
          if (xx >= 0 && yy >= 0 && xx < bw && yy < bh && m[idx(xx, yy)] && !label[idx(xx, yy)]) {
            label[idx(xx, yy)] = cur;
            q.push({xx, yy});
          }
        }
      }
      if (size > bestSize) {
        bestSize = size;
        best = cur;
      }
    }
  for (size_t i = 0; i < m.size(); ++i) m[i] = (label[i] == best && best) ? 1 : 0;
  // 3) 구멍 메우기: 바깥에서 닿지 않는 배경을 전경으로
  std::vector<uint8_t> outside(m.size(), 0);
  std::queue<std::pair<int, int>> q;
  for (int x = 0; x < bw; ++x) {
    q.push({x, 0});
    q.push({x, bh - 1});
  }
  for (int y = 0; y < bh; ++y) {
    q.push({0, y});
    q.push({bw - 1, y});
  }
  while (!q.empty()) {
    auto [x, y] = q.front();
    q.pop();
    if (x < 0 || y < 0 || x >= bw || y >= bh || m[idx(x, y)] || outside[idx(x, y)]) continue;
    outside[idx(x, y)] = 1;
    q.push({x + 1, y});
    q.push({x - 1, y});
    q.push({x, y + 1});
    q.push({x, y - 1});
  }
  for (size_t i = 0; i < m.size(); ++i)
    if (!outside[i]) m[i] = 1;
}
}  // namespace

OrientationField computeOrientation(const ImageF& img) {
  const int w = img.w, h = img.h;
  OrientationField of;
  of.bw = w / kBlock;
  of.bh = h / kBlock;
  const size_t nb = (size_t)of.bw * of.bh;
  std::vector<float> vx(nb, 0), vy(nb, 0), energy(nb, 0), stdv(nb, 0);

  for (int by = 0; by < of.bh; ++by)
    for (int bx = 0; bx < of.bw; ++bx) {
      double gxx = 0, gxy = 0, gyy = 0, s = 0, s2 = 0;
      int n = 0;
      for (int y = by * kBlock; y < (by + 1) * kBlock; ++y)
        for (int x = bx * kBlock; x < (bx + 1) * kBlock; ++x) {
          float v = img.at(x, y);
          s += v;
          s2 += v * v;
          ++n;
          if (x < 1 || y < 1 || x >= w - 1 || y >= h - 1) continue;
          // Sobel
          float gx = (img.at(x + 1, y - 1) + 2 * img.at(x + 1, y) + img.at(x + 1, y + 1)) -
                     (img.at(x - 1, y - 1) + 2 * img.at(x - 1, y) + img.at(x - 1, y + 1));
          float gy = (img.at(x - 1, y + 1) + 2 * img.at(x, y + 1) + img.at(x + 1, y + 1)) -
                     (img.at(x - 1, y - 1) + 2 * img.at(x, y - 1) + img.at(x + 1, y - 1));
          gxx += gx * gx;
          gyy += gy * gy;
          gxy += gx * gy;
        }
      size_t i = (size_t)by * of.bw + bx;
      vx[i] = (float)(gxx - gyy);
      vy[i] = (float)(2 * gxy);
      energy[i] = (float)(gxx + gyy);
      double mean = s / n;
      stdv[i] = (float)std::sqrt(std::max(0.0, s2 / n - mean * mean));
    }

  // 전경 판정 기준: 이 영상의 블록 표준편차 상위 10% 값에 대한 비율 (센서·압력별 대비 차이 흡수)
  std::vector<float> sorted = stdv;
  std::nth_element(sorted.begin(), sorted.begin() + sorted.size() * 9 / 10, sorted.end());
  const float p90 = std::max(0.2f, sorted[sorted.size() * 9 / 10]);

  // 이중각 벡터를 5x5 블록 범위에서 평활화
  of.theta.assign(nb, 0);
  of.coherence.assign(nb, 0);
  of.mask.assign(nb, 0);
  for (int by = 0; by < of.bh; ++by)
    for (int bx = 0; bx < of.bw; ++bx) {
      double sx = 0, sy = 0, se = 0;
      for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx) {
          int xx = bx + dx, yy = by + dy;
          if (xx < 0 || yy < 0 || xx >= of.bw || yy >= of.bh) continue;
          double wgt = std::exp(-(dx * dx + dy * dy) / 4.0);
          size_t j = (size_t)yy * of.bw + xx;
          sx += wgt * vx[j];
          sy += wgt * vy[j];
          se += wgt * energy[j];
        }
      size_t i = (size_t)by * of.bw + bx;
      // 기울기 방향의 수직 = 융선 방향
      float t = 0.5f * (float)std::atan2(sy, sx) + kPi / 2;
      if (t >= kPi) t -= kPi;
      of.theta[i] = t;
      of.coherence[i] = se > 1e-6 ? (float)(std::sqrt(sx * sx + sy * sy) / se) : 0.f;
      of.mask[i] = ((stdv[i] > 0.22f * p90 && of.coherence[i] > 0.25f) || stdv[i] > 0.5f * p90) ? 1 : 0;
    }
  cleanMask(of.mask, of.bw, of.bh);
  return of;
}

std::vector<float> estimateRidgePeriodMap(const ImageF& img, const OrientationField& of) {
  const int L = 32, Wd = 16;  // 융선 수직 방향 길이 L, 융선 방향 폭 Wd
  const float kMinP = 4.f, kMaxP = 16.f, kStep = 0.25f;
  const size_t nb = static_cast<size_t>(of.bw) * of.bh;
  std::vector<float> per(nb, 0.f);  // 0 = 미정
  float hann[L];
  for (int k = 0; k < L; ++k) hann[k] = 0.5f - 0.5f * std::cos(2 * kPi * k / (L - 1));

  for (int by = 0; by < of.bh; ++by)
    for (int bx = 0; bx < of.bw; ++bx) {
      size_t i = static_cast<size_t>(by) * of.bw + bx;
      if (!of.mask[i] || of.coherence[i] < 0.3f) continue;
      float t = of.theta[i];
      float cx = (bx + 0.5f) * kBlock, cy = (by + 0.5f) * kBlock;
      float ux = std::cos(t), uy = std::sin(t);  // 융선 방향
      float nx = -uy, ny = ux;                   // 수직 방향
      float sig[L];
      bool ok = true;
      for (int k = 0; k < L && ok; ++k) {
        float acc = 0;
        for (int d = 0; d < Wd; ++d) {
          int ix = static_cast<int>(std::lround(cx + (k - L / 2) * nx + (d - Wd / 2) * ux));
          int iy = static_cast<int>(std::lround(cy + (k - L / 2) * ny + (d - Wd / 2) * uy));
          if (ix < 0 || iy < 0 || ix >= img.w || iy >= img.h) {
            ok = false;
            break;
          }
          acc += img.at(ix, iy);
        }
        sig[k] = acc / Wd;
      }
      if (!ok) continue;
      float mean = 0;
      for (float v : sig) mean += v;
      mean /= L;
      double total = 0;
      for (int k = 0; k < L; ++k) {
        sig[k] = (sig[k] - mean) * hann[k];
        total += sig[k] * sig[k];
      }
      if (total <= 1e-6) continue;
      // 후보 주기별 DFT 파워의 최댓값
      double bestPow = 0;
      float bestP = 0;
      for (float p = kMinP; p <= kMaxP + 1e-3f; p += kStep) {
        double re = 0, im = 0;
        for (int k = 0; k < L; ++k) {
          float ph = 2 * kPi * k / p;
          re += sig[k] * std::cos(ph);
          im += sig[k] * std::sin(ph);
        }
        double pw = re * re + im * im;
        if (pw > bestPow) {
          bestPow = pw;
          bestP = p;
        }
      }
      // 파워가 한 주기에 충분히 모인 경우만 채택 (Parseval: 순수 정현파면 약 (L/2)·total)
      if (bestPow > kPeakRatio * L * total) per[i] = bestP;
    }

  // 미정 블록 채우기: 이웃 평균을 반복 전파
  size_t known = 0;
  for (float v : per) known += v > 0;
  if (known < 5) return std::vector<float>(nb, 9.f);
  for (int it = 0; it < of.bw + of.bh && known < nb; ++it) {
    std::vector<float> nxt = per;
    for (int by = 0; by < of.bh; ++by)
      for (int bx = 0; bx < of.bw; ++bx) {
        size_t i = static_cast<size_t>(by) * of.bw + bx;
        if (per[i] > 0) continue;
        float s = 0;
        int c = 0;
        for (int dy = -1; dy <= 1; ++dy)
          for (int dx = -1; dx <= 1; ++dx) {
            int xx = bx + dx, yy = by + dy;
            if (xx < 0 || yy < 0 || xx >= of.bw || yy >= of.bh) continue;
            float v = per[static_cast<size_t>(yy) * of.bw + xx];
            if (v > 0) {
              s += v;
              ++c;
            }
          }
        if (c) {
          nxt[i] = s / c;
          ++known;
        }
      }
    per.swap(nxt);
  }
  // 5x5 가우시안 평활화
  std::vector<float> out(nb);
  for (int by = 0; by < of.bh; ++by)
    for (int bx = 0; bx < of.bw; ++bx) {
      double s = 0, ws = 0;
      for (int dy = -2; dy <= 2; ++dy)
        for (int dx = -2; dx <= 2; ++dx) {
          int xx = bx + dx, yy = by + dy;
          if (xx < 0 || yy < 0 || xx >= of.bw || yy >= of.bh) continue;
          float v = per[static_cast<size_t>(yy) * of.bw + xx];
          if (v <= 0) continue;
          double w = std::exp(-(dx * dx + dy * dy) / 4.0);
          s += w * v;
          ws += w;
        }
      out[static_cast<size_t>(by) * of.bw + bx] = ws > 0 ? std::clamp(static_cast<float>(s / ws), 5.f, 15.f) : 9.f;
    }
  return out;
}

float medianPeriod(const std::vector<float>& per, const OrientationField& of) {
  std::vector<float> v;
  for (size_t i = 0; i < per.size(); ++i)
    if (of.mask[i]) v.push_back(per[i]);
  if (v.empty()) return 9.f;
  std::nth_element(v.begin(), v.begin() + v.size() / 2, v.end());
  return v[v.size() / 2];
}

ImageU8 enhanceAndBinarize(const ImageF& img, const OrientationField& of, const std::vector<float>& per) {
  const int NA = 24;                       // 방향 단계
  const float kPMin = 5.f, kPStep = 0.5f;  // 간격 단계 5, 5.5, ... 15
  const int NP = 21;
  // (간격, 방향)별 Gabor 커널: 사용하는 간격 단계만 계산, 평균 0으로 밝기 편향 제거
  std::vector<int> radius(NP, 0);
  std::vector<std::vector<float>> kernels(static_cast<size_t>(NP) * NA);
  auto binOf = [&](float p) { return std::clamp(static_cast<int>(std::lround((p - kPMin) / kPStep)), 0, NP - 1); };
  std::vector<uint8_t> used(NP, 0);
  for (size_t i = 0; i < per.size(); ++i)
    if (of.mask[i]) used[binOf(per[i])] = 1;
  for (int pb = 0; pb < NP; ++pb) {
    if (!used[pb]) continue;
    float period = kPMin + pb * kPStep, sigma = kSigmaRatio * period, f = 1.f / period;
    int R = static_cast<int>(std::ceil(2.5f * sigma)), K = 2 * R + 1;
    radius[pb] = R;
    for (int a = 0; a < NA; ++a) {
      float t = a * kPi / NA;
      float c = std::cos(t), s = std::sin(t);
      auto& k = kernels[static_cast<size_t>(pb) * NA + a];
      k.resize(static_cast<size_t>(K) * K);
      double sum = 0;
      for (int y = -R; y <= R; ++y)
        for (int x = -R; x <= R; ++x) {
          float xr = x * c + y * s;   // 융선 방향
          float yr = -x * s + y * c;  // 수직 방향
          float g = std::exp(-(xr * xr + yr * yr) / (2 * sigma * sigma)) * std::cos(2 * kPi * f * yr);
          k[static_cast<size_t>(y + R) * K + (x + R)] = g;
          sum += g;
        }
      float mean = static_cast<float>(sum / (static_cast<double>(K) * K));
      for (float& v : k) v -= mean;
    }
  }

  ImageU8 out(img.w, img.h, 0);
  for (int y = 0; y < img.h; ++y)
    for (int x = 0; x < img.w; ++x) {
      if (!of.maskAt(x, y)) continue;
      size_t bi = static_cast<size_t>(y / kBlock) * of.bw + (x / kBlock);
      int pb = binOf(per[bi]);
      int R = radius[pb], K = 2 * R + 1;
      if (x < R || y < R || x >= img.w - R || y >= img.h - R) continue;
      int a = static_cast<int>(std::lround(of.thetaAt(x, y) / kPi * NA)) % NA;
      const float* k = kernels[static_cast<size_t>(pb) * NA + a].data();
      float acc = 0;
      for (int dy = -R; dy <= R; ++dy) {
        const float* row = &img.d[static_cast<size_t>(y + dy) * img.w + (x - R)];
        const float* kr = k + static_cast<size_t>(dy + R) * K;
        for (int dx = 0; dx < K; ++dx) acc += row[dx] * kr[dx];
      }
      out.at(x, y) = acc < 0 ? 1 : 0;  // 융선은 어두움(음수) → 1
    }
  return out;
}

void thin(ImageU8& b) {
  const int w = b.w, h = b.h;
  std::vector<size_t> del;
  bool changed = true;
  while (changed) {
    changed = false;
    for (int pass = 0; pass < 2; ++pass) {
      del.clear();
      for (int y = 1; y < h - 1; ++y)
        for (int x = 1; x < w - 1; ++x) {
          if (!b.at(x, y)) continue;
          int p2 = b.at(x, y - 1), p3 = b.at(x + 1, y - 1), p4 = b.at(x + 1, y), p5 = b.at(x + 1, y + 1);
          int p6 = b.at(x, y + 1), p7 = b.at(x - 1, y + 1), p8 = b.at(x - 1, y), p9 = b.at(x - 1, y - 1);
          int B = p2 + p3 + p4 + p5 + p6 + p7 + p8 + p9;
          if (B < 2 || B > 6) continue;
          int A = (!p2 && p3) + (!p3 && p4) + (!p4 && p5) + (!p5 && p6) + (!p6 && p7) + (!p7 && p8) +
                  (!p8 && p9) + (!p9 && p2);
          if (A != 1) continue;
          if (pass == 0) {
            if (p2 * p4 * p6 || p4 * p6 * p8) continue;
          } else {
            if (p2 * p4 * p8 || p2 * p6 * p8) continue;
          }
          del.push_back((size_t)y * w + x);
        }
      for (size_t i : del) b.d[i] = 0;
      if (!del.empty()) changed = true;
    }
  }
}

}  // namespace fpa
