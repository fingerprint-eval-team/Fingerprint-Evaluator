#pragma once
#include <cstdint>
#include <vector>

#include "minutiae/Minutiae.h"

namespace fpa {

constexpr uint32_t kTemplateMagic = 0x31545046;  // 'FPT1'
constexpr uint16_t kTemplateVersion = 1;

#pragma pack(push, 1)
struct TemplateHeader {
  uint32_t magic;
  uint16_t version;
  uint16_t width, height;
  uint16_t count;
  uint8_t quality;   // 평균 품질 0..255
  uint8_t reserved;
};
struct PackedMinutia {
  uint16_t x, y;
  uint16_t angle;    // 0..35999 (0.01도 단위)
  uint8_t type;      // 1 끝점, 2 분기점
  uint8_t quality;   // 0..255
};
#pragma pack(pop)

// 직렬화: 반환값은 바이트 수. KISA_Compare에는 크기가 전달되지 않으므로 헤더에 개수를 담는다.
int writeTemplate(const std::vector<Minutia>& m, int w, int h, unsigned char* out, int capacity);
bool readTemplate(const unsigned char* p, std::vector<Minutia>& m, int& w, int& h);

}  // namespace fpa
