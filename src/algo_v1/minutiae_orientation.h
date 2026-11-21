#pragma once
#include "minutiae_overlap.h"
#include <cstddef>
#include <memory>

namespace minutiae::orientation {
// A 16-pixel cell at 500 ppi spans roughly two fingerprint ridges. Physical
// sampling is shared across sensors and resolutions, with no fitted threshold.
constexpr int cellSize = 16, maxSide = 210;
struct Cell {
    float c = 0, s = 0; // Coherence-weighted doubled gradient direction.
};
struct Template {
    uint32_t magic = 0x46504f31, version = 1, bytes = 0, checksum = 0;
    uint32_t cols = 0, rows = 0, width = 0, height = 0, dpi = 0, reserved = 0;
    minutiae::Feature base;
    Cell cells[maxSide * maxSide]{};
};
static_assert(sizeof(Template) == 551512 && offsetof(Template, base) == 40);

inline uint32_t checksum(const Template& value) {
    auto p = reinterpret_cast<const unsigned char*>(&value);
    uint32_t result = 2166136261u;
    for (size_t i = 0; i < sizeof(value); ++i)
        if (i < 12 || i >= 16)
            result = (result ^ p[i]) * 16777619u;
    return result;
}
inline bool header(const unsigned char* bytes) {
    uint32_t words[3]{};
    std::memcpy(words, bytes, sizeof(words));
    return words[0] == 0x46504f31 && words[1] == 1 && words[2] == sizeof(Template);
}
inline bool valid(const Template& t) {
    if (!header(reinterpret_cast<const unsigned char*>(&t)) || t.reserved || t.width < 150 ||
        t.height < 150 || t.width > 2000 || t.height > 2000 || t.dpi < 300 || t.dpi > 1024 ||
        t.cols != (t.width * 500 + t.dpi * cellSize - 1) / (t.dpi * cellSize) ||
        t.rows != (t.height * 500 + t.dpi * cellSize - 1) / (t.dpi * cellSize) ||
        t.cols > maxSide || t.rows > maxSide || t.checksum != checksum(t) ||
        !minutiae::valid(t.base))
        return false;
    for (size_t i = 0; i < size_t(t.cols) * t.rows; ++i) {
        auto p = t.cells[i];
        if (!std::isfinite(p.c) || !std::isfinite(p.s) ||
            double(p.c) * p.c + double(p.s) * p.s > 1.000001)
            return false;
    }
    return true;
}
inline void encode(Template& out, const minutiae::Feature& base, const unsigned char* image,
                   unsigned width, unsigned height, unsigned dpi) {
    if (!image || width < 150 || height < 150 || width > 2000 || height > 2000 || dpi < 300 ||
        dpi > 1024)
        throw std::runtime_error("Unsupported orientation image");
    out.width = width;
    out.height = height;
    out.dpi = dpi;
    out.cols = (width * 500 + dpi * cellSize - 1) / (dpi * cellSize);
    out.rows = (height * 500 + dpi * cellSize - 1) / (dpi * cellSize);
    if (out.cols > maxSide || out.rows > maxSide)
        throw std::runtime_error("Orientation grid too large");
    out.base = base;
    std::fill(std::begin(out.cells), std::end(out.cells), Cell{});
    struct Tensor {
        double xx = 0, yy = 0, xy = 0;
    };
    std::vector<Tensor> tensors(size_t(out.cols) * out.rows);
    for (unsigned y = 1; y + 1 < height; ++y)
        for (unsigned x = 1; x + 1 < width; ++x) {
            double gx = int(image[y * width + x + 1]) - int(image[y * width + x - 1]),
                   gy = int(image[(y + 1) * width + x]) - int(image[(y - 1) * width + x]);
            auto& t = tensors[(y * 500 / (dpi * cellSize)) * out.cols + x * 500 / (dpi * cellSize)];
            t.xx += gx * gx;
            t.yy += gy * gy;
            t.xy += gx * gy;
        }
    for (size_t i = 0; i < tensors.size(); ++i) {
        auto t = tensors[i];
        double energy = t.xx + t.yy;
        out.cells[i] =
            energy > 0 ? Cell{float((t.xx - t.yy) / energy), float(2 * t.xy / energy)} : Cell{};
    }
    out.bytes = sizeof(out);
    out.checksum = checksum(out);
}
inline bool sample(const Template& field, double x, double y, Cell& result) {
    x = x / cellSize - .5;
    y = y / cellSize - .5;
    if (x < -1e-8 || y < -1e-8 || x > field.cols - 1 + 1e-8 || y > field.rows - 1 + 1e-8)
        return false;
    x = std::clamp(x, 0.0, double(field.cols - 1));
    y = std::clamp(y, 0.0, double(field.rows - 1));
    unsigned x0 = unsigned(x), y0 = unsigned(y), x1 = std::min(x0 + 1, field.cols - 1),
             y1 = std::min(y0 + 1, field.rows - 1);
    double dx = x - x0, dy = y - y0, c = 0, s = 0;
    for (unsigned iy = 0; iy < 2; ++iy)
        for (unsigned ix = 0; ix < 2; ++ix) {
            double w = (ix ? dx : 1 - dx) * (iy ? dy : 1 - dy);
            auto p = field.cells[(iy ? y1 : y0) * field.cols + (ix ? x1 : x0)];
            c += w * p.c;
            s += w * p.s;
        }
    result = {float(c), float(s)};
    return true;
}
struct Evidence {
    double factor = 1, meanReliability = 0;
    size_t samples = 0;
};
inline Evidence evidence(const Template& a, const Template& b, minutiae::Transform t) {
    double disagreement = 0, reliability = 0;
    Evidence result;
    auto direction = [&](const Template& first, const Template& second, minutiae::Transform u) {
        double cs2 = u.cs * u.cs - u.sn * u.sn, sn2 = 2 * u.cs * u.sn;
        for (unsigned y = 0; y < first.rows; ++y)
            for (unsigned x = 0; x < first.cols; ++x) {
                double px = (x + .5) * cellSize, py = (y + .5) * cellSize;
                Cell q;
                if (!sample(second, u.cs * px - u.sn * py + u.tx, u.sn * px + u.cs * py + u.ty, q))
                    continue;
                auto p = first.cells[y * first.cols + x];
                double c = cs2 * p.c - sn2 * p.s, s = sn2 * p.c + cs2 * p.s;
                double weight = std::clamp(std::hypot(c, s) * std::hypot(q.c, q.s), 0.0, 1.0);
                double dot = std::clamp(c * q.c + s * q.s, -weight, weight);
                disagreement += (weight - dot) / 2;
                reliability += weight;
                ++result.samples;
            }
    };
    direction(a, b, t);
    direction(b, a, {t.cs, -t.sn, -t.cs * t.tx - t.sn * t.ty, t.sn * t.tx - t.cs * t.ty});
    if (result.samples) {
        result.factor = std::clamp(1 - disagreement / result.samples, 0.0, 1.0);
        result.meanReliability = reliability / result.samples;
    }
    return result;
}
struct Comparison {
    double score = 0, geometryScore = 0;
    Evidence field;
};
inline Comparison compare(const Template& a, const Template& b) {
    if (!valid(a) || !valid(b))
        throw std::runtime_error("Invalid orientation template");
    minutiae::Diagnostics trace;
    Comparison result;
    result.geometryScore = graph::compare(a.base, b.base, &trace, overlap::scale(a.base, b.base));
    if (trace.hasAlignment)
        result.field = evidence(a, b, trace.transform);
    result.score = result.geometryScore * result.field.factor;
    return result;
}
} // namespace minutiae::orientation
