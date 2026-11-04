#pragma once
#include <cstdint>
#include <vector>

namespace fpa {

struct ImageF {
  int w = 0, h = 0;
  std::vector<float> d;
  ImageF() = default;
  ImageF(int w_, int h_, float v = 0.f) : w(w_), h(h_), d((size_t)w_ * h_, v) {}
  float& at(int x, int y) { return d[(size_t)y * w + x]; }
  float at(int x, int y) const { return d[(size_t)y * w + x]; }
};

struct ImageU8 {
  int w = 0, h = 0;
  std::vector<uint8_t> d;
  ImageU8() = default;
  ImageU8(int w_, int h_, uint8_t v = 0) : w(w_), h(h_), d((size_t)w_ * h_, v) {}
  uint8_t& at(int x, int y) { return d[(size_t)y * w + x]; }
  uint8_t at(int x, int y) const { return d[(size_t)y * w + x]; }
  bool inside(int x, int y) const { return x >= 0 && y >= 0 && x < w && y < h; }
};

// 입력 raw 버퍼(w*h 바이트)를 float로 복사. 버퍼 범위 밖은 절대 읽지 않는다.
ImageF toFloat(const unsigned char* p, int w, int h);

// 평균 0, 표준편차 1로 정규화
void normalize(ImageF& img);

// 국소 정규화: 반경 r 창의 평균·표준편차로 정규화 (흐린 영역의 융선 대비 회복)
void localNormalize(ImageF& img, int r = 8);

}  // namespace fpa
