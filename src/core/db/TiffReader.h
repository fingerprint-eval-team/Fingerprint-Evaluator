#pragma once
#include <cstdint>
#include <string>
#include <vector>

namespace fpe {

struct GrayImage {
  int width = 0, height = 0;
  std::vector<uint8_t> pixels;  // 좌상단부터 1 byte/pixel
};

// 비압축 8-bit grayscale TIFF 로더 (FVC DB 형식). 압축 TIFF는 지원하지 않는다.
bool readTiff(const std::wstring& path, GrayImage& out, std::string& err);

}  // namespace fpe
