#include "minutiae/Minutiae.h"

#include <algorithm>
#include <cmath>

namespace fpa {

namespace {
constexpr float kPi = 3.14159265358979f;
const int kNx[8] = {1, 1, 0, -1, -1, -1, 0, 1};  // 시계 반대 방향 8이웃
const int kNy[8] = {0, -1, -1, -1, 0, 1, 1, 1};

int crossingNumber(const ImageU8& s, int x, int y) {
  int cn = 0;
  for (int k = 0; k < 8; ++k) {
    int a = s.at(x + kNx[k], y + kNy[k]);
    int b = s.at(x + kNx[(k + 1) % 8], y + kNy[(k + 1) % 8]);
    cn += std::abs(a - b);
  }
  return cn / 2;
}

struct TraceResult {
  int ex, ey, len;
  bool hitJunction;
};

// (sx,sy)에서 이웃 (nx,ny) 방향으로 골격을 maxLen 픽셀까지 따라간다.
// 방문 표시: 시작점과 시작점의 골격 이웃 전부(다른 가지로 새지 않도록) + 지나온 점의 이웃.
TraceResult trace(const ImageU8& s, int sx, int sy, int nx, int ny, int maxLen) {
  std::vector<std::pair<int, int>> visited;
  auto isVisited = [&](int x, int y) {
    for (auto& v : visited)
      if (v.first == x && v.second == y) return true;
    return false;
  };
  visited.push_back({sx, sy});
  for (int k = 0; k < 8; ++k) {
    int xx = sx + kNx[k], yy = sy + kNy[k];
    if (s.inside(xx, yy) && s.at(xx, yy)) visited.push_back({xx, yy});
  }
  int cx = nx, cy = ny, len = 1;
  while (len < maxLen) {
    std::pair<int, int> cand[8];
    int nc = 0;
    for (int k = 0; k < 8; ++k) {
      int xx = cx + kNx[k], yy = cy + kNy[k];
      if (s.inside(xx, yy) && s.at(xx, yy) && !isVisited(xx, yy)) cand[nc++] = {xx, yy};
    }
    if (nc == 0) return {cx, cy, len, false};  // 융선 끝
    // 후보들이 서로 붙어 있으면 한 경로(4-이웃 우선), 떨어져 있으면 분기점
    int pick = 0;
    for (int i = 0; i < nc; ++i) {
      for (int j = i + 1; j < nc; ++j)
        if (std::abs(cand[i].first - cand[j].first) > 1 || std::abs(cand[i].second - cand[j].second) > 1)
          return {cx, cy, len, true};
      if (cand[i].first == cx || cand[i].second == cy) pick = i;
    }
    for (int i = 0; i < nc; ++i) visited.push_back(cand[i]);
    cx = cand[pick].first;
    cy = cand[pick].second;
    ++len;
  }
  return {cx, cy, len, false};
}

float angDiff(float a, float b) {
  float d = std::fmod(std::fabs(a - b), 2 * kPi);
  return d > kPi ? 2 * kPi - d : d;
}
}  // namespace

std::vector<Minutia> extractMinutiae(const ImageU8& s, const OrientationField& of, size_t maxCount) {
  const int margin = 12, L = 12;
  std::vector<Minutia> cand;
  std::vector<int> branchLen;

  for (int y = margin; y < s.h - margin; ++y)
    for (int x = margin; x < s.w - margin; ++x) {
      if (!s.at(x, y)) continue;
      // 경계 근처(마스크 바깥으로 1블록 이내)는 제외
      bool nearEdge = false;
      for (int dy = -kBlock; dy <= kBlock && !nearEdge; dy += kBlock)
        for (int dx = -kBlock; dx <= kBlock; dx += kBlock)
          if (!of.maskAt(std::clamp(x + dx, 0, s.w - 1), std::clamp(y + dy, 0, s.h - 1))) {
            nearEdge = true;
            break;
          }
      if (nearEdge) continue;

      int cn = crossingNumber(s, x, y);
      if (cn != 1 && cn != 3) continue;

      std::vector<TraceResult> br;
      // 8이웃 고리에서 0→1로 바뀌는 지점마다 가지 하나 (가지 수 = CN)
      for (int k = 0; k < 8; ++k) {
        int prev = (k + 7) % 8;
        int xx = x + kNx[k], yy = y + kNy[k];
        if (s.at(xx, yy) && !s.at(x + kNx[prev], y + kNy[prev])) br.push_back(trace(s, x, y, xx, yy, L));
      }

      Minutia m;
      m.x = (float)x;
      m.y = (float)y;
      m.quality = of.cohAt(x, y);
      if (cn == 1 && br.size() == 1) {
        m.type = MinutiaType::Ending;
        // 융선 끝에서 융선 안쪽을 향하는 방향: 끝점↔분기점 뒤바뀜(압력 차) 시 방향이 일치
        m.angle = std::atan2((float)(br[0].ey - y), (float)(br[0].ex - x));
        branchLen.push_back(br[0].len);  // 짧으면 잔가지(spur) 또는 짧은 조각
      } else if (cn == 3 && br.size() == 3) {
        m.type = MinutiaType::Bifurcation;
        float a[3];
        for (int i = 0; i < 3; ++i) a[i] = std::atan2((float)(br[i].ey - y), (float)(br[i].ex - x));
        // 가장 가까운 두 가지(포크)의 평균 방향
        float d01 = angDiff(a[0], a[1]), d02 = angDiff(a[0], a[2]), d12 = angDiff(a[1], a[2]);
        int i1 = 0, i2 = 1;
        if (d02 < d01 && d02 <= d12) {
          i1 = 0;
          i2 = 2;
        } else if (d12 < d01 && d12 < d02) {
          i1 = 1;
          i2 = 2;
        }
        m.angle = std::atan2(std::sin(a[i1]) + std::sin(a[i2]), std::cos(a[i1]) + std::cos(a[i2]));
        int minLen = std::min({br[0].len, br[1].len, br[2].len});
        branchLen.push_back(minLen);
      } else {
        continue;
      }
      if (m.angle < 0) m.angle += 2 * kPi;
      cand.push_back(m);
    }

  // 거짓 미뉴셔 제거
  std::vector<uint8_t> drop(cand.size(), 0);
  for (size_t i = 0; i < cand.size(); ++i)
    if (branchLen[i] < (cand[i].type == MinutiaType::Ending ? 6 : 3)) drop[i] = 1;  // 짧은 가지(spur)
  for (size_t i = 0; i < cand.size(); ++i)
    for (size_t j = i + 1; j < cand.size(); ++j) {
      float dx = cand[i].x - cand[j].x, dy = cand[i].y - cand[j].y;
      float d2 = dx * dx + dy * dy;
      if (d2 < 4 * 4) {
        drop[i] = drop[j] = 1;  // 너무 가까운 쌍
      } else if (d2 < 10 * 10 && cand[i].type == MinutiaType::Ending && cand[j].type == MinutiaType::Ending &&
                 angDiff(cand[i].angle, cand[j].angle) > kPi * 0.75f) {
        drop[i] = drop[j] = 1;  // 끊어진 융선의 양 끝
      }
    }
  std::vector<Minutia> out;
  for (size_t i = 0; i < cand.size(); ++i)
    if (!drop[i]) out.push_back(cand[i]);

  if (out.size() > maxCount) {
    std::sort(out.begin(), out.end(), [](const Minutia& a, const Minutia& b) { return a.quality > b.quality; });
    out.resize(maxCount);
  }
  return out;
}

}  // namespace fpa
