#pragma once
#include "image/Image.h"

namespace fpa {

constexpr int kBlock = 8;  // 500dpi 기준 블록 크기(px)

// 블록 단위 방향장(융선 방향, 0..π), 일관성(coherence 0..1), 전경 마스크
struct OrientationField {
  int bw = 0, bh = 0;           // 블록 격자 크기
  std::vector<float> theta;     // 융선 방향(라디안)
  std::vector<float> coherence; // 방향 일관성
  std::vector<uint8_t> mask;    // 1 = 지문 영역
  float thetaAt(int x, int y) const { return theta[(size_t)(y / kBlock) * bw + (x / kBlock)]; }
  float cohAt(int x, int y) const { return coherence[(size_t)(y / kBlock) * bw + (x / kBlock)]; }
  bool maskAt(int x, int y) const {
    int bx = x / kBlock, by = y / kBlock;
    if (bx >= bw || by >= bh) return false;
    return mask[(size_t)by * bw + bx] != 0;
  }
};

OrientationField computeOrientation(const ImageF& norm);

// 블록별 x-signature로 융선 간격(px)을 추정, 전체 중앙값 반환 (실패 시 9)
float estimateRidgePeriod(const ImageF& norm, const OrientationField& of);

// Gabor 필터로 융선 강조 후 이진화 (1 = 융선)
ImageU8 enhanceAndBinarize(const ImageF& norm, const OrientationField& of, float period);

// Zhang-Suen 세선화 (in-place, 1 = 골격)
void thin(ImageU8& bin);

}  // namespace fpa
