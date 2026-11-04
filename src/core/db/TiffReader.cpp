#include "db/TiffReader.h"

#include "PathUtil.h"

namespace fpe {

namespace {
struct Reader {
  const std::vector<uint8_t>& b;
  bool le = true;
  bool ok(size_t off, size_t n) const { return off + n <= b.size(); }
  uint16_t u16(size_t o) const {
    return le ? (uint16_t)(b[o] | (b[o + 1] << 8)) : (uint16_t)((b[o] << 8) | b[o + 1]);
  }
  uint32_t u32(size_t o) const {
    return le ? (uint32_t)(b[o] | (b[o + 1] << 8) | (b[o + 2] << 16) | ((uint32_t)b[o + 3] << 24))
              : (uint32_t)(((uint32_t)b[o] << 24) | (b[o + 1] << 16) | (b[o + 2] << 8) | b[o + 3]);
  }
};

struct Entry {
  uint16_t tag = 0, type = 0;
  uint32_t count = 0;
  size_t valueOff = 0;  // 값이 위치한 파일 오프셋
};

// SHORT(3) 또는 LONG(4) 배열 읽기
std::vector<uint32_t> values(const Reader& r, const Entry& e) {
  std::vector<uint32_t> v;
  size_t sz = e.type == 3 ? 2 : 4;
  size_t off = (sz * e.count <= 4) ? e.valueOff : r.u32(e.valueOff);
  if (!r.ok(off, sz * e.count)) return v;
  for (uint32_t i = 0; i < e.count; ++i) v.push_back(e.type == 3 ? r.u16(off + i * 2) : r.u32(off + i * 4));
  return v;
}
}  // namespace

bool readTiff(const std::wstring& path, GrayImage& out, std::string& err) {
  std::vector<uint8_t> buf;
  if (!readFile(path, buf) || buf.size() < 8) {
    err = "cannot read file";
    return false;
  }
  Reader r{buf};
  if (buf[0] == 'I' && buf[1] == 'I') r.le = true;
  else if (buf[0] == 'M' && buf[1] == 'M') r.le = false;
  else { err = "not a TIFF"; return false; }
  if (r.u16(2) != 42) { err = "not a TIFF"; return false; }

  size_t ifd = r.u32(4);
  if (!r.ok(ifd, 2)) { err = "bad IFD"; return false; }
  uint16_t n = r.u16(ifd);
  uint32_t width = 0, height = 0, bps = 8, compression = 1, photometric = 1, spp = 1;
  std::vector<uint32_t> stripOff, stripCnt, colormap;
  for (uint16_t i = 0; i < n; ++i) {
    size_t eo = ifd + 2 + i * 12;
    if (!r.ok(eo, 12)) break;
    Entry e{r.u16(eo), r.u16(eo + 2), r.u32(eo + 4), eo + 8};
    auto v = values(r, e);
    uint32_t first = v.empty() ? 0 : v[0];
    switch (e.tag) {
      case 256: width = first; break;
      case 257: height = first; break;
      case 258: bps = first; break;
      case 259: compression = first; break;
      case 262: photometric = first; break;
      case 273: stripOff = v; break;
      case 277: spp = first; break;
      case 279: stripCnt = v; break;
      case 320: colormap = v; break;
      default: break;
    }
  }
  if (compression != 1) { err = "compressed TIFF not supported"; return false; }
  if (bps != 8 || spp != 1) { err = "only 8-bit grayscale supported"; return false; }
  if (!width || !height || stripOff.empty()) { err = "missing image tags"; return false; }

  out.width = (int)width;
  out.height = (int)height;
  out.pixels.assign((size_t)width * height, 0);
  size_t pos = 0;
  for (size_t s = 0; s < stripOff.size() && pos < out.pixels.size(); ++s) {
    size_t cnt = s < stripCnt.size() ? stripCnt[s] : out.pixels.size() - pos;
    cnt = std::min(cnt, out.pixels.size() - pos);
    if (!r.ok(stripOff[s], cnt)) { err = "strip out of range"; return false; }
    memcpy(out.pixels.data() + pos, buf.data() + stripOff[s], cnt);
    pos += cnt;
  }
  if (pos != out.pixels.size()) { err = "truncated pixel data"; return false; }

  if (photometric == 0) {  // WhiteIsZero
    for (auto& p : out.pixels) p = (uint8_t)(255 - p);
  } else if (photometric == 3 && colormap.size() >= 256) {  // 팔레트 → 빨강 채널을 밝기로
    for (auto& p : out.pixels) p = (uint8_t)(colormap[p] >> 8);
  }
  return true;
}

}  // namespace fpe
