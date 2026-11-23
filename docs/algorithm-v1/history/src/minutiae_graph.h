#pragma once
#include "minutiae.h"

namespace minutiae::graph {
// Fixed, sensor-independent physical tolerances. Candidate v1 is kept separate
// from the frozen v2 baseline. No image IDs, labels or database names enter here.
constexpr double distanceSigma = 4.0, relativeDistanceSigma = 0.05;
constexpr double relativeAngleSigma = 0.35, bearingSigma = 0.35;

inline double compatibility(Point a1, Point a2, Point b1, Point b2) {
    double ax = a2.x - a1.x, ay = a2.y - a1.y, bx = b2.x - b1.x, by = b2.y - b1.y;
    double da = std::hypot(ax, ay), db = std::hypot(bx, by);
    if (da < 8 || db < 8)
        return 0;
    double distance = (da - db) / (distanceSigma + relativeDistanceSigma * (da + db) / 2);
    double direction = wrap((a2.theta - a1.theta) - (b2.theta - b1.theta)) / relativeAngleSigma;
    double bearingA = std::atan2(ay, ax), bearingB = std::atan2(by, bx);
    double first = wrap((bearingA - a1.theta) - (bearingB - b1.theta)) / bearingSigma;
    double second = wrap((bearingA + pi - a2.theta) - (bearingB + pi - b2.theta)) / bearingSigma;
    return std::exp(
        -0.5 * (distance * distance + direction * direction + first * first + second * second));
}

inline double coherentScore(const Feature& a, const Feature& b, Diagnostics& trace) {
    if (!trace.hasAlignment || trace.matches.size() < 3)
        return 0;
    auto original = trace.matches;
    double support = 0;
    for (size_t i = 0; i < original.size(); ++i) {
        auto p = original[i];
        double sum = 0, weights = 0;
        for (size_t j = 0; j < original.size(); ++j)
            if (i != j) {
                auto q = original[j];
                double weight = q.similarity;
                sum += weight *
                       compatibility(a.points[p.a], a.points[q.a], b.points[p.b], b.points[q.b]);
                weights += weight;
            }
        double coherence = weights > 0 ? sum / weights : 0;
        trace.matches[i].similarity = p.similarity * std::sqrt(std::clamp(coherence, 0.0, 1.0));
        support += trace.matches[i].similarity;
    }
    double fraction = support / std::sqrt(double(a.count) * b.count);
    trace.score = std::clamp(fraction * support / (support + 8), 0.0, 1.0);
    return trace.score;
}

inline double compare(const Feature& a, const Feature& b, Diagnostics* diagnostics = nullptr) {
    if (!valid(a) || !valid(b))
        throw std::runtime_error("Invalid minutiae template");
    if (diagnostics)
        *diagnostics = Diagnostics{};
    std::vector<Pair> candidates;
    for (unsigned i = 0; i < a.count; ++i)
        for (unsigned j = 0; j < b.count; ++j) {
            double dot = 0;
            for (int k = 0; k < descriptorSize; ++k)
                dot += double(a.descriptors[i][k]) * b.descriptors[j][k];
            if (dot >= 0.2)
                candidates.push_back({int(i), int(j), std::min(1.0, dot)});
        }
    std::sort(candidates.begin(), candidates.end(), [](auto x, auto y) {
        if (x.similarity != y.similarity)
            return x.similarity > y.similarity;
        return std::tie(x.a, x.b) < std::tie(y.a, y.b);
    });
    double best = 0;
    size_t hypotheses = 0;
    auto evaluate = [&](Transform transform) {
        ++hypotheses;
        Diagnostics candidate;
        scoreTransform(a, b, candidates, transform, &candidate);
        double score = coherentScore(a, b, candidate);
        if (score > best) {
            best = score;
            if (diagnostics)
                *diagnostics = std::move(candidate);
        }
    };
    size_t seeds = std::min(size_t(64), candidates.size());
    for (size_t i = 0; i < seeds; ++i) {
        auto p = candidates[i];
        evaluate(rootTransform(a.points[p.a], b.points[p.b]));
    }
    for (size_t i = 0; i < std::min(size_t(24), seeds); ++i)
        for (size_t j = i + 1; j < std::min(size_t(24), seeds); ++j) {
            auto p = candidates[i], q = candidates[j];
            if (p.a == q.a || p.b == q.b)
                continue;
            auto a1 = a.points[p.a], a2 = a.points[q.a], b1 = b.points[p.b], b2 = b.points[q.b];
            double ax = a2.x - a1.x, ay = a2.y - a1.y, bx = b2.x - b1.x, by = b2.y - b1.y,
                   da = std::hypot(ax, ay), db = std::hypot(bx, by);
            if (da < 25 || std::abs(da - db) > 8 + 0.05 * da)
                continue;
            double angle = std::atan2(by, bx) - std::atan2(ay, ax);
            if (std::abs(wrap(b1.theta - a1.theta - angle)) > angleTolerance)
                continue;
            double cs = std::cos(angle), sn = std::sin(angle);
            evaluate({cs, sn, b1.x - cs * a1.x + sn * a1.y, b1.y - sn * a1.x - cs * a1.y});
        }
    if (diagnostics) {
        diagnostics->candidateCount = candidates.size();
        diagnostics->hypotheses = hypotheses;
    }
    return best;
}
} // namespace minutiae::graph
