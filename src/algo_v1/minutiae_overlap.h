#pragma once
#include "minutiae_graph.h"

namespace minutiae::overlap {
struct XY {
    double x, y;
};
inline double cross(XY a, XY b, XY c) {
    return (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
}
inline std::vector<XY> hull(const Feature& f) {
    std::vector<XY> p;
    for (unsigned i = 0; i < f.count; ++i)
        p.push_back({f.points[i].x, f.points[i].y});
    std::sort(p.begin(), p.end(),
              [](XY a, XY b) { return std::tie(a.x, a.y) < std::tie(b.x, b.y); });
    p.erase(std::unique(p.begin(), p.end(), [](XY a, XY b) { return a.x == b.x && a.y == b.y; }),
            p.end());
    if (p.size() < 3)
        return p;
    std::vector<XY> result;
    for (auto point : p) {
        while (result.size() > 1 && cross(result[result.size() - 2], result.back(), point) <= 0)
            result.pop_back();
        result.push_back(point);
    }
    size_t lower = result.size();
    for (size_t i = p.size() - 1; i-- > 0;) {
        while (result.size() > lower && cross(result[result.size() - 2], result.back(), p[i]) <= 0)
            result.pop_back();
        result.push_back(p[i]);
    }
    result.pop_back();
    return result;
}
inline bool containsPadded(const std::vector<XY>& polygon, XY p,
                           double padding = positionTolerance) {
    if (polygon.empty())
        return false;
    bool inside = polygon.size() >= 3;
    double nearest = 1e100;
    for (size_t i = 0; i < polygon.size(); ++i) {
        auto a = polygon[i], b = polygon[(i + 1) % polygon.size()];
        if (cross(a, b, p) < -1e-9)
            inside = false;
        double dx = b.x - a.x, dy = b.y - a.y, length = dx * dx + dy * dy;
        double t =
            length > 0 ? std::clamp(((p.x - a.x) * dx + (p.y - a.y) * dy) / length, 0.0, 1.0) : 0;
        nearest = std::min(nearest, std::hypot(p.x - a.x - t * dx, p.y - a.y - t * dy));
    }
    return inside || nearest <= padding + 1e-9;
}
inline graph::ScoreScale scale(const Feature& a, const Feature& b) {
    auto ah = hull(a), bh = hull(b);
    return [ah, bh](const Feature& first, const Feature& second, const Diagnostics& trace) {
        auto t = trace.transform;
        unsigned na = 0, nb = 0;
        for (unsigned i = 0; i < first.count; ++i) {
            auto p = first.points[i];
            na += containsPadded(bh,
                                 {t.cs * p.x - t.sn * p.y + t.tx, t.sn * p.x + t.cs * p.y + t.ty});
        }
        for (unsigned i = 0; i < second.count; ++i) {
            auto p = second.points[i];
            double x = p.x - t.tx, y = p.y - t.ty;
            nb += containsPadded(ah, {t.cs * x + t.sn * y, -t.sn * x + t.cs * y});
        }
        double ea = std::max(double(na), 0.5 * first.count),
               eb = std::max(double(nb), 0.5 * second.count);
        return std::sqrt(double(first.count) * second.count / (ea * eb));
    };
}
} // namespace minutiae::overlap
