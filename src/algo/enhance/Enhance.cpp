#include "enhance/Enhance.h"

#include <algorithm>
#include <cmath>
#include <queue>

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;

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

float estimateRidgePeriod(const ImageF& img, const OrientationField& of) {
  std::vector<float> periods;
  const int L = 32, Wd = 16;  // 융선 수직 방향 길이 L, 융선 방향 폭 Wd
  for (int by = 2; by < of.bh - 2; by += 2)
    for (int bx = 2; bx < of.bw - 2; bx += 2) {
      size_t i = (size_t)by * of.bw + bx;
      if (!of.mask[i] || of.coherence[i] < 0.4f) continue;
      float t = of.theta[i];
      float cx = (bx + 0.5f) * kBlock, cy = (by + 0.5f) * kBlock;
      float ux = std::cos(t), uy = std::sin(t);   // 융선 방향
      float nx = -uy, ny = ux;                    // 수직 방향
      float sig[L];
      bool ok = true;
      for (int k = 0; k < L && ok; ++k) {
        float acc = 0;
        for (int d = 0; d < Wd; ++d) {
          float px = cx + (k - L / 2) * nx + (d - Wd / 2) * ux;
          float py = cy + (k - L / 2) * ny + (d - Wd / 2) * uy;
          int ix = (int)std::lround(px), iy = (int)std::lround(py);
          if (ix < 0 || iy < 0 || ix >= img.w || iy >= img.h) {
            ok = false;
            break;
          }
          acc += img.at(ix, iy);
        }
        sig[k] = acc / Wd;
      }
      if (!ok) continue;
      // 국소 최대값 사이 평균 간격
      int first = -1, last = -1, peaks = 0;
      for (int k = 1; k < L - 1; ++k)
        if (sig[k] > sig[k - 1] && sig[k] >= sig[k + 1]) {
          if (first < 0) first = k;
          last = k;
          ++peaks;
        }
      if (peaks >= 2) {
        float p = (float)(last - first) / (peaks - 1);
        if (p >= 4.f && p <= 16.f) periods.push_back(p);
      }
    }
  if (periods.size() < 5) return 9.f;
  std::nth_element(periods.begin(), periods.begin() + periods.size() / 2, periods.end());
  return std::clamp(periods[periods.size() / 2], 5.f, 14.f);
}

ImageU8 enhanceAndBinarize(const ImageF& img, const OrientationField& of, float period) {
  const int R = 6, K = 2 * R + 1, NA = 24;
  const float sigma = 4.0f, f = 1.0f / period;
  // 방향별 Gabor 커널 미리 계산 (평균 0으로 맞춰 밝기 편향 제거)
  std::vector<std::vector<float>> kernels(NA, std::vector<float>(K * K));
  for (int a = 0; a < NA; ++a) {
    float t = a * kPi / NA;
    float c = std::cos(t), s = std::sin(t);
    double sum = 0;
    for (int y = -R; y <= R; ++y)
      for (int x = -R; x <= R; ++x) {
        float xr = x * c + y * s;    // 융선 방향
        float yr = -x * s + y * c;   // 수직 방향
        float g = std::exp(-(xr * xr + yr * yr) / (2 * sigma * sigma)) * std::cos(2 * kPi * f * yr);
        kernels[a][(y + R) * K + (x + R)] = g;
        sum += g;
      }
    float mean = (float)(sum / (K * K));
    for (float& v : kernels[a]) v -= mean;
  }

  ImageU8 bin(img.w, img.h, 0);
  for (int y = R; y < img.h - R; ++y)
    for (int x = R; x < img.w - R; ++x) {
      if (!of.maskAt(x, y)) continue;
      int a = (int)std::lround(of.thetaAt(x, y) / kPi * NA) % NA;
      const float* k = kernels[a].data();
      float acc = 0;
      for (int dy = -R; dy <= R; ++dy) {
        const float* row = &img.d[(size_t)(y + dy) * img.w + (x - R)];
        const float* kr = k + (dy + R) * K;
        for (int dx = 0; dx < K; ++dx) acc += row[dx] * kr[dx];
      }
      bin.at(x, y) = acc < 0 ? 1 : 0;  // 융선은 어두움(음수) → 1
    }
  return bin;
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
