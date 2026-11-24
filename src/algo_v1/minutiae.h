#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <tuple>
#include <vector>
namespace minutiae {
constexpr double pi = 3.14159265358979323846;
constexpr int maxPoints = 128, side = 8, angles = 6, descriptorSize = side * side * angles;
constexpr double radius = 80, step = 20, positionTolerance = 14, angleTolerance = 0.55;
// Parameters are fixed in physical (500 dpi) units, identical for all sensors.
// This is an original local-neighborhood + geometric-consensus matcher, not
// an implementation or performance claim for the published MCC algorithm.
struct Point {
    float x = 0, y = 0, theta = 0, quality = 0;
};
struct Feature {
    uint32_t count = 0;
    Point points[maxPoints]{};
    std::vector<std::array<float, descriptorSize>> descriptors;
};
inline double wrap(double x) {
    // Most angles already lie in this range. For a quotient of 0 or +/-1,
    // remainder is exactly x or one rounded subtraction of the same period.
    // Keep all larger values (including tie cases) on the standard path.
    if (x >= -pi && x <= pi)
        return x;
    if (x > pi && x < 2 * pi)
        return x - 2 * pi;
    if (x < -pi && x > -2 * pi)
        return x + 2 * pi;
    return std::remainder(x, 2 * pi);
}
inline void describe(Feature& f) {
    f.descriptors.assign(f.count, {});
    for (unsigned i = 0; i < f.count; ++i) {
        auto p = f.points[i];
        auto& out = f.descriptors[i];
        double cs = std::cos(p.theta), sn = std::sin(p.theta);
        for (unsigned j = 0; j < f.count; ++j)
            if (j != i) {
                auto q = f.points[j];
                double dx = q.x - p.x, dy = q.y - p.y;
                if (dx * dx + dy * dy > (radius + step) * (radius + step))
                    continue;
                double xx = (cs * dx + sn * dy) / step + (side - 1) / 2.0,
                       yy = (-sn * dx + cs * dy) / step + (side - 1) / 2.0,
                       aa = (wrap(q.theta - p.theta) + pi) * angles / (2 * pi);
                std::array<double, angles> angularWeights{};
                for (int b = 0; b < angles; ++b) {
                    double da = std::remainder(b + 0.5 - aa, double(angles));
                    angularWeights[b] = std::exp(-da * da / 0.72);
                }
                for (int y = std::max(0, int(std::floor(yy)) - 1);
                     y <= std::min(side - 1, int(std::floor(yy)) + 2); ++y)
                    for (int x = std::max(0, int(std::floor(xx)) - 1);
                         x <= std::min(side - 1, int(std::floor(xx)) + 2); ++x) {
                        double px = (x - (side - 1) / 2.0) * step,
                               py = (y - (side - 1) / 2.0) * step;
                        if (px * px + py * py > radius * radius)
                            continue;
                        double spatial =
                            std::exp(-((x - xx) * (x - xx) + (y - yy) * (y - yy)) / 0.72);
                        for (int b = 0; b < angles; ++b) {
                            out[(y * side + x) * angles + b] += float(spatial * angularWeights[b]);
                        }
                    }
            }
        double norm = 0;
        for (float& v : out) {
            v = float(1 - std::exp(-v));
            norm += v * v;
        }
        norm = std::sqrt(norm);
        if (norm > 0)
            for (float& v : out)
                v = float(v / norm);
    }
}
inline bool valid(const Feature& f) {
    if (f.count < 3 || f.count > maxPoints || f.descriptors.size() != f.count)
        return false;
    for (unsigned i = 0; i < f.count; ++i) {
        auto p = f.points[i];
        if (!std::isfinite(p.x) || !std::isfinite(p.y) || !std::isfinite(p.theta) ||
            !std::isfinite(p.quality) || p.x < 0 || p.y < 0 || p.x > 4000 || p.y > 4000 ||
            std::abs(p.theta) > 2 * pi)
            return false;
        for (float v : f.descriptors[i])
            if (!std::isfinite(v) || v < 0 || v > 1.001)
                return false;
    }
    return true;
}
struct Pair {
    int a, b;
    double similarity;
};
struct Transform {
    double cs = 1, sn = 0, tx = 0, ty = 0;
};
struct Diagnostics {
    double score = 0;
    bool hasAlignment = false;
    size_t candidateCount = 0, hypotheses = 0;
    Transform transform;
    // similarity contains the final geometric/descriptor weight, as used in scoring.
    std::vector<Pair> matches;
};
inline Transform rootTransform(Point a, Point b) {
    double angle = wrap(b.theta - a.theta), cs = std::cos(angle), sn = std::sin(angle);
    return {cs, sn, b.x - cs * a.x + sn * a.y, b.y - sn * a.x - cs * a.y};
}
inline double scoreTransform(const Feature& a, const Feature& b,
                             const std::vector<Pair>& candidates, Transform transform,
                             Diagnostics* diagnostics = nullptr) {
    std::vector<Pair> matched;
    matched.reserve(maxPoints);
    struct Weighted {
        Pair pair;
        double value;
    };
    std::vector<Weighted> eligible;
    eligible.reserve(candidates.size());
    for (int iteration = 0; iteration < 3; ++iteration) {
        matched.clear();
        std::array<bool, maxPoints> usedA{}, usedB{};
        double rotation = std::atan2(transform.sn, transform.cs);
        eligible.clear();
        for (auto pair : candidates) {
            auto p = a.points[pair.a], q = b.points[pair.b];
            double dx = transform.cs * p.x - transform.sn * p.y + transform.tx - q.x,
                   dy = transform.sn * p.x + transform.cs * p.y + transform.ty - q.y;
            double distance = dx * dx + dy * dy;
            if (distance > positionTolerance * positionTolerance)
                continue;
            double da = std::abs(wrap(p.theta + rotation - q.theta));
            if (da > angleTolerance)
                continue;
            double weight =
                pair.similarity * (0.5 + 0.5 * std::exp(-distance / 128 - da * da / 0.32));
            eligible.push_back({pair, weight});
        }
        std::sort(eligible.begin(), eligible.end(),
                  [](auto x, auto y) { return x.value > y.value; });
        for (auto e : eligible)
            if (!usedA[e.pair.a] && !usedB[e.pair.b]) {
                usedA[e.pair.a] = true;
                usedB[e.pair.b] = true;
                matched.push_back({e.pair.a, e.pair.b, e.value});
            }
        if (matched.size() < 3)
            return 0;
        if (iteration == 2)
            break;
        double ax = 0, ay = 0, bx = 0, by = 0, weights = 0;
        for (auto p : matched) {
            double w = p.similarity;
            ax += w * a.points[p.a].x;
            ay += w * a.points[p.a].y;
            bx += w * b.points[p.b].x;
            by += w * b.points[p.b].y;
            weights += w;
        }
        ax /= weights;
        ay /= weights;
        bx /= weights;
        by /= weights;
        double cross = 0, dot = 0;
        for (auto p : matched) {
            double x = a.points[p.a].x - ax, y = a.points[p.a].y - ay, u = b.points[p.b].x - bx,
                   v = b.points[p.b].y - by;
            dot += p.similarity * (x * u + y * v);
            cross += p.similarity * (x * v - y * u);
        }
        double angle = std::atan2(cross, dot);
        transform = {std::cos(angle), std::sin(angle), 0, 0};
        transform.tx = bx - transform.cs * ax + transform.sn * ay;
        transform.ty = by - transform.sn * ax - transform.cs * ay;
    }
    double support = 0;
    for (auto pair : matched)
        support += pair.similarity;
    double fraction = support / std::sqrt(double(a.count) * b.count);
    double score = std::clamp(fraction * support / (support + 8), 0.0, 1.0);
    if (diagnostics && score > diagnostics->score) {
        diagnostics->score = score;
        diagnostics->hasAlignment = true;
        diagnostics->transform = transform;
        diagnostics->matches = matched;
    }
    return score;
}
} // namespace minutiae
