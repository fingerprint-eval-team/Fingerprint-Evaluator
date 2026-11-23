#pragma once
#include <algorithm>
#include <cmath>
#include <vector>

#include "minutiae/Minutiae.h"

namespace fpa {

struct Pt {
  float x, y;
};

// 강체 변환: p' = R(rot)·p + (dx, dy)
struct Rigid {
  float rot = 0, dx = 0, dy = 0;
};

// Andrew monotone chain 볼록 껍질 (반시계)
inline std::vector<Pt> convexHull(std::vector<Pt> p) {
  std::sort(p.begin(), p.end(), [](const Pt& a, const Pt& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
  if (p.size() < 3) return p;
  auto cross = [](const Pt& o, const Pt& a, const Pt& b) {
    return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x);
  };
  std::vector<Pt> h(2 * p.size());
  size_t k = 0;
  for (size_t i = 0; i < p.size(); ++i) {
    while (k >= 2 && cross(h[k - 2], h[k - 1], p[i]) <= 0) --k;
    h[k++] = p[i];
  }
  for (size_t i = p.size() - 1, t = k + 1; i > 0; --i) {
    while (k >= t && cross(h[k - 2], h[k - 1], p[i - 1]) <= 0) --k;
    h[k++] = p[i - 1];
  }
  h.resize(k - 1);
  return h;
}

// 볼록 다각형(반시계) 안이거나 경계에서 margin 이내인가
inline bool nearHull(const std::vector<Pt>& h, float x, float y, float margin) {
  if (h.size() < 3) return false;
  bool inside = true;
  float best = 1e30f;
  for (size_t i = 0; i < h.size(); ++i) {
    const Pt& a = h[i];
    const Pt& b = h[(i + 1) % h.size()];
    float ex = b.x - a.x, ey = b.y - a.y;
    if (ex * (y - a.y) - ey * (x - a.x) < 0) inside = false;
    float len2 = ex * ex + ey * ey;
    float t = len2 > 0 ? std::clamp(((x - a.x) * ex + (y - a.y) * ey) / len2, 0.f, 1.f) : 0.f;
    float dx = a.x + t * ex - x, dy = a.y + t * ey - y;
    best = std::min(best, dx * dx + dy * dy);
  }
  return inside || best <= margin * margin;
}

inline std::vector<Pt> hullOf(const std::vector<Minutia>& m) {
  std::vector<Pt> p;
  p.reserve(m.size());
  for (const auto& q : m) p.push_back({q.x, q.y});
  return convexHull(p);
}

}  // namespace fpa
