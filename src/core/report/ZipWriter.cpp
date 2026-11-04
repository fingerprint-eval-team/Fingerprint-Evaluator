#include "report/ZipWriter.h"

namespace fpe {

namespace {
void put16(std::vector<uint8_t>& o, uint16_t v) {
  o.push_back((uint8_t)v);
  o.push_back((uint8_t)(v >> 8));
}
void put32(std::vector<uint8_t>& o, uint32_t v) {
  for (int i = 0; i < 4; ++i) o.push_back((uint8_t)(v >> (8 * i)));
}
constexpr uint16_t kDosTime = 0;
constexpr uint16_t kDosDate = (uint16_t)(((2026 - 1980) << 9) | (11 << 5) | 4);
}  // namespace

uint32_t ZipWriter::crc32(const uint8_t* p, size_t n) {
  static uint32_t table[256];
  static bool init = false;
  if (!init) {
    for (uint32_t i = 0; i < 256; ++i) {
      uint32_t c = i;
      for (int k = 0; k < 8; ++k) c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
      table[i] = c;
    }
    init = true;
  }
  uint32_t c = 0xFFFFFFFFu;
  for (size_t i = 0; i < n; ++i) c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
  return c ^ 0xFFFFFFFFu;
}

void ZipWriter::add(const std::string& name, std::vector<uint8_t> data) { entries_.push_back({name, std::move(data)}); }

std::vector<uint8_t> ZipWriter::finish() const {
  std::vector<uint8_t> out, central;
  for (const auto& e : entries_) {
    uint32_t crc = crc32(e.data.data(), e.data.size());
    uint32_t offset = (uint32_t)out.size();
    // local file header
    put32(out, 0x04034b50);
    put16(out, 20);
    put16(out, 0x0800);  // UTF-8 파일명
    put16(out, 0);       // stored
    put16(out, kDosTime);
    put16(out, kDosDate);
    put32(out, crc);
    put32(out, (uint32_t)e.data.size());
    put32(out, (uint32_t)e.data.size());
    put16(out, (uint16_t)e.name.size());
    put16(out, 0);
    out.insert(out.end(), e.name.begin(), e.name.end());
    out.insert(out.end(), e.data.begin(), e.data.end());
    // central directory
    put32(central, 0x02014b50);
    put16(central, 20);
    put16(central, 20);
    put16(central, 0x0800);
    put16(central, 0);
    put16(central, kDosTime);
    put16(central, kDosDate);
    put32(central, crc);
    put32(central, (uint32_t)e.data.size());
    put32(central, (uint32_t)e.data.size());
    put16(central, (uint16_t)e.name.size());
    put16(central, 0);
    put16(central, 0);
    put16(central, 0);
    put16(central, 0);
    put32(central, 0);
    put32(central, offset);
    central.insert(central.end(), e.name.begin(), e.name.end());
  }
  uint32_t cdOff = (uint32_t)out.size();
  out.insert(out.end(), central.begin(), central.end());
  put32(out, 0x06054b50);
  put16(out, 0);
  put16(out, 0);
  put16(out, (uint16_t)entries_.size());
  put16(out, (uint16_t)entries_.size());
  put32(out, (uint32_t)central.size());
  put32(out, cdOff);
  put16(out, 0);
  return out;
}

}  // namespace fpe
