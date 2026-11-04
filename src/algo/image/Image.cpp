#include "image/Image.h"

#include <algorithm>
#include <cmath>

namespace fpa {

ImageF toFloat(const unsigned char* p, int w, int h) {
  ImageF img(w, h);
  const size_t n = (size_t)w * h;
  for (size_t i = 0; i < n; ++i) img.d[i] = (float)p[i];
  return img;
}

void normalize(ImageF& img) {
  double sum = 0, sq = 0;
  for (float v : img.d) {
    sum += v;
    sq += (double)v * v;
  }
  const double n = (double)img.d.size();
  const double mean = sum / n;
  double var = sq / n - mean * mean;
  const double sd = var > 1e-6 ? std::sqrt(var) : 1.0;
  for (float& v : img.d) v = (float)((v - mean) / sd);
}

void localNormalize(ImageF& img, int r) {
  const int w = img.w, h = img.h;
  // 적분 영상으로 (2r+1)^2 창의 평균·분산 계산
  std::vector<double> s((size_t)(w + 1) * (h + 1), 0), s2((size_t)(w + 1) * (h + 1), 0);
  for (int y = 0; y < h; ++y) {
    double rs = 0, rs2 = 0;
    for (int x = 0; x < w; ++x) {
      double v = img.at(x, y);
      rs += v;
      rs2 += v * v;
      s[(size_t)(y + 1) * (w + 1) + x + 1] = s[(size_t)y * (w + 1) + x + 1] + rs;
      s2[(size_t)(y + 1) * (w + 1) + x + 1] = s2[(size_t)y * (w + 1) + x + 1] + rs2;
    }
  }
  ImageF out(w, h);
  for (int y = 0; y < h; ++y)
    for (int x = 0; x < w; ++x) {
      int x0 = std::max(0, x - r), x1 = std::min(w, x + r + 1), y0 = std::max(0, y - r), y1 = std::min(h, y + r + 1);
      auto box = [&](const std::vector<double>& t) {
        return t[(size_t)y1 * (w + 1) + x1] - t[(size_t)y0 * (w + 1) + x1] - t[(size_t)y1 * (w + 1) + x0] +
               t[(size_t)y0 * (w + 1) + x0];
      };
      double n = (double)(x1 - x0) * (y1 - y0);
      double mean = box(s) / n, var = box(s2) / n - mean * mean;
      out.at(x, y) = (float)((img.at(x, y) - mean) / std::sqrt(std::max(var, 0.01)));
    }
  img = std::move(out);
}

}  // namespace fpa
