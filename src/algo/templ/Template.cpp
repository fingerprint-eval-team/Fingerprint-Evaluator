#include "templ/Template.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;
}

int writeTemplate(const std::vector<Minutia>& m, int w, int h, unsigned char* out, int capacity) {
  size_t n = std::min<size_t>(m.size(), 65535);
  int size = (int)(sizeof(TemplateHeader) + n * sizeof(PackedMinutia));
  if (size > capacity) return -1;
  TemplateHeader hd{};
  hd.magic = kTemplateMagic;
  hd.version = kTemplateVersion;
  hd.width = (uint16_t)w;
  hd.height = (uint16_t)h;
  hd.count = (uint16_t)n;
  float q = 0;
  for (size_t i = 0; i < n; ++i) q += m[i].quality;
  hd.quality = (uint8_t)std::clamp(n ? q / n * 255.f : 0.f, 0.f, 255.f);
  memcpy(out, &hd, sizeof hd);
  auto* pm = reinterpret_cast<PackedMinutia*>(out + sizeof hd);
  for (size_t i = 0; i < n; ++i) {
    PackedMinutia p{};
    p.x = (uint16_t)std::lround(m[i].x);
    p.y = (uint16_t)std::lround(m[i].y);
    p.angle = (uint16_t)(std::lround(m[i].angle * 18000.f / kPi) % 36000);
    p.type = (uint8_t)m[i].type;
    p.quality = (uint8_t)std::clamp(m[i].quality * 255.f, 0.f, 255.f);
    memcpy(&pm[i], &p, sizeof p);
  }
  return size;
}

bool readTemplate(const unsigned char* p, std::vector<Minutia>& m, int& w, int& h) {
  TemplateHeader hd;
  memcpy(&hd, p, sizeof hd);
  if (hd.magic != kTemplateMagic || hd.version != kTemplateVersion) return false;
  w = hd.width;
  h = hd.height;
  m.resize(hd.count);
  const unsigned char* q = p + sizeof hd;
  for (size_t i = 0; i < hd.count; ++i) {
    PackedMinutia pm;
    memcpy(&pm, q + i * sizeof pm, sizeof pm);
    m[i].x = pm.x;
    m[i].y = pm.y;
    m[i].angle = pm.angle * kPi / 18000.f;
    m[i].type = (MinutiaType)pm.type;
    m[i].quality = pm.quality / 255.f;
  }
  return true;
}

}  // namespace fpa
