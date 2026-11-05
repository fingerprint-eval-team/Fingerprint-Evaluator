// AlgoDebug.exe <raw|tif> <iType> <outPrefix>
// FPAlgo 처리 단계별 중간 영상을 PGM으로 저장 (알고리즘 튜닝용, 제출물 아님)
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "PathUtil.h"
#include "SensorTable.h"
#include "db/TiffReader.h"
#include "enhance/Enhance.h"
#include "image/Image.h"
#include "minutiae/Minutiae.h"

using namespace fpa;

static void savePgm(const std::string& path, int w, int h, const std::vector<uint8_t>& px) {
  FILE* f = fopen(path.c_str(), "wb");
  fprintf(f, "P5\n%d %d\n255\n", w, h);
  fwrite(px.data(), 1, px.size(), f);
  fclose(f);
}

int wmain(int argc, wchar_t** argv) {
  if (argc < 4) return 2;
  fpe::GrayImage g;
  std::string err;
  if (!fpe::readTiff(argv[1], g, err)) {
    printf("%s\n", err.c_str());
    return 1;
  }
  std::string out = fpe::toUtf8(argv[3]);
  ImageF img = toFloat(g.pixels.data(), g.width, g.height);
  normalize(img);
  OrientationField of = computeOrientation(img);
  std::vector<float> periodMap = estimateRidgePeriodMap(img, of);
  float period = medianPeriod(periodMap, of);
  localNormalize(img);
  ImageU8 bin = enhanceAndBinarize(img, of, periodMap);
  std::vector<uint8_t> px(bin.d.size());
  for (size_t i = 0; i < px.size(); ++i) px[i] = bin.d[i] ? 0 : 255;
  savePgm(out + "_bin.pgm", bin.w, bin.h, px);

  std::vector<uint8_t> mk(img.d.size());
  for (int y = 0; y < img.h; ++y)
    for (int x = 0; x < img.w; ++x) mk[(size_t)y * img.w + x] = of.maskAt(x, y) ? g.pixels[(size_t)y * img.w + x] : 0;
  savePgm(out + "_mask.pgm", img.w, img.h, mk);

  thin(bin);
  auto m = extractMinutiae(bin, of);
  std::vector<uint8_t> sk(bin.d.size());
  for (size_t i = 0; i < sk.size(); ++i) sk[i] = bin.d[i] ? 0 : 255;
  for (auto& mm : m)
    for (int d = -3; d <= 3; ++d) {
      int x = (int)mm.x, y = (int)mm.y;
      auto put = [&](int xx, int yy) {
        if (xx >= 0 && yy >= 0 && xx < bin.w && yy < bin.h) sk[(size_t)yy * bin.w + xx] = 128;
      };
      put(x + d, y - 3);
      put(x + d, y + 3);
      put(x - 3, y + d);
      put(x + 3, y + d);
    }
  savePgm(out + "_skel.pgm", bin.w, bin.h, sk);
  size_t fg = 0;
  for (auto v : of.mask) fg += v;
  printf("size %dx%d period %.2f mask %zu/%zu minutiae %zu\n", g.width, g.height, period, fg, of.mask.size(), m.size());
  return 0;
}
