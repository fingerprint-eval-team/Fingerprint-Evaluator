#include "api.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <vector>
// Spatial/rotation-aligned orientation field plus local-contrast texture.
namespace {
constexpr int grid = 24, textureGrid = 96;
constexpr unsigned magic = 0x46503032;
struct Cell {
    float x, y, weight;
};
struct Feature {
    unsigned tag;
    Cell cells[grid * grid];
    float texture[textureGrid * textureGrid];
};
int fail(int* size, int* error, char* message, int code, const char* text) {
    *size = 0;
    *error = code;
    strcpy_s(message, 256, text);
    return 0;
}
int extract(int type, unsigned char* image, unsigned char* output, int* size, int* error,
            char* message) {
    if (!image || !output || !size || !error || !message)
        return 0;
    *size = 0;
    *error = 0;
    message[0] = 0;
    UINT w = 0, h = 0;
    if (!dimensions(type, w, h))
        return fail(size, error, message, 1, "Unsupported iType");
    std::vector<float> horizontal(w * h), smoothed(w * h);
    constexpr int kernel[5] = {1, 4, 6, 4, 1};
    for (int y = 0; y < int(h); ++y)
        for (int x = 0; x < int(w); ++x) {
            float sum = 0;
            for (int d = -2; d <= 2; ++d)
                sum += kernel[d + 2] * image[y * w + std::clamp(x + d, 0, int(w) - 1)];
            horizontal[y * w + x] = sum / 16;
        }
    for (int y = 0; y < int(h); ++y)
        for (int x = 0; x < int(w); ++x) {
            float sum = 0;
            for (int d = -2; d <= 2; ++d)
                sum += kernel[d + 2] * horizontal[std::clamp(y + d, 0, int(h) - 1) * w + x];
            smoothed[y * w + x] = sum / 16;
        }
    struct Acc {
        double xx = 0, xy = 0, energy = 0, sum = 0, squared = 0;
        int n = 0;
    };
    std::array<Acc, grid * grid> acc{};
    for (int y = 1; y < int(h) - 1; ++y)
        for (int x = 1; x < int(w) - 1; ++x) {
            double gx = smoothed[y * w + x + 1] - smoothed[y * w + x - 1],
                   gy = smoothed[(y + 1) * w + x] - smoothed[(y - 1) * w + x];
            auto& a = acc[(y * grid / h) * grid + x * grid / w];
            a.xx += gx * gx - gy * gy;
            a.xy += 2 * gx * gy;
            a.energy += gx * gx + gy * gy;
            double v = smoothed[y * w + x];
            a.sum += v;
            a.squared += v * v;
            ++a.n;
        }
    Feature f{};
    f.tag = magic;
    int valid = 0;
    for (int c = 0; c < grid * grid; ++c) {
        auto& a = acc[c];
        if (!a.n)
            continue;
        double length = std::hypot(a.xx, a.xy), coherence = length / (a.energy + 1e-9),
               variance = a.squared / a.n - std::pow(a.sum / a.n, 2);
        if (variance < 40 || coherence < 0.25 || a.energy / a.n < 8)
            continue;
        f.cells[c] = {float(a.xx / (length + 1e-9)), float(a.xy / (length + 1e-9)),
                      float(coherence)};
        ++valid;
    }
    if (valid < 30)
        return fail(size, error, message, 3, "Insufficient fingerprint structure");
    for (int y = 0; y < textureGrid; ++y)
        for (int x = 0; x < textureGrid; ++x) {
            int px = std::min(int(w) - 1, (2 * x + 1) * int(w) / (2 * textureGrid)),
                py = std::min(int(h) - 1, (2 * y + 1) * int(h) / (2 * textureGrid));
            double sum = 0, sq = 0;
            int n = 0;
            for (int dy = -6; dy <= 6; dy += 2)
                for (int dx = -6; dx <= 6; dx += 2) {
                    double v = smoothed[std::clamp(py + dy, 0, int(h) - 1) * w +
                                        std::clamp(px + dx, 0, int(w) - 1)];
                    sum += v;
                    sq += v * v;
                    ++n;
                }
            double mean = sum / n, sd = std::sqrt(std::max(0.0, sq / n - mean * mean));
            f.texture[y * textureGrid + x] =
                sd < 5 ? 0
                       : float(std::clamp((smoothed[py * w + px] - mean) / (sd + 1), -2.0, 2.0));
        }
    static_assert(sizeof(f) < capacity);
    std::memcpy(output, &f, sizeof(f));
    *size = sizeof(f);
    return 1;
}
struct Candidate {
    double value;
    int dx, dy;
    double angle;
};
double match(const Feature& a, const Feature& b) {
    double totalA = 0, totalB = 0;
    for (auto c : a.cells)
        totalA += c.weight;
    for (auto c : b.cells)
        totalB += c.weight;
    if (totalA < 1 || totalB < 1)
        return 0;
    std::vector<Candidate> candidates;
    for (int degrees = -18; degrees <= 18; degrees += 3) {
        double angle = degrees * 3.141592653589793 / 180, cs = std::cos(angle),
               sn = std::sin(angle), c2 = std::cos(2 * angle), s2 = std::sin(2 * angle);
        for (int dy = -4; dy <= 4; ++dy)
            for (int dx = -4; dx <= 4; ++dx) {
                double sum = 0, weights = 0;
                for (int y = 1; y < grid - 1; ++y)
                    for (int x = 1; x < grid - 1; ++x) {
                        auto ca = a.cells[y * grid + x];
                        if (ca.weight == 0)
                            continue;
                        double ax = x - (grid - 1) / 2.0, ay = y - (grid - 1) / 2.0;
                        int bx = int(std::round(cs * ax - sn * ay + (grid - 1) / 2.0 + dx)),
                            by = int(std::round(sn * ax + cs * ay + (grid - 1) / 2.0 + dy));
                        if (bx < 0 || by < 0 || bx >= grid || by >= grid)
                            continue;
                        auto cb = b.cells[by * grid + bx];
                        if (cb.weight == 0)
                            continue;
                        double weight = std::min(ca.weight, cb.weight);
                        sum += weight *
                               ((ca.x * c2 - ca.y * s2) * cb.x + (ca.x * s2 + ca.y * c2) * cb.y);
                        weights += weight;
                    }
                double overlap = weights / std::min(totalA, totalB);
                if (overlap < 0.45)
                    continue;
                double value = (sum / (weights + 1e-9) + 1) / 2 * std::sqrt(std::min(1.0, overlap));
                candidates.push_back({value, dx, dy, angle});
            }
    }
    if (candidates.empty())
        return 0;
    std::sort(candidates.begin(), candidates.end(),
              [](auto x, auto y) { return x.value > y.value; });
    double best = 0;
    for (size_t k = 0; k < std::min(size_t(4), candidates.size()); ++k) {
        auto c = candidates[k];
        double cs = std::cos(c.angle), sn = std::sin(c.angle), bestTexture = -1;
        for (int oy = -2; oy <= 2; oy += 2)
            for (int ox = -2; ox <= 2; ox += 2) {
                double ab = 0, aa = 0, bb = 0;
                int n = 0;
                for (int y = 4; y < textureGrid - 4; y += 2)
                    for (int x = 4; x < textureGrid - 4; x += 2) {
                        if (a.cells[(y / 4) * grid + x / 4].weight == 0)
                            continue;
                        double ax = x - (textureGrid - 1) / 2.0, ay = y - (textureGrid - 1) / 2.0;
                        int bx = int(std::round(cs * ax - sn * ay + (textureGrid - 1) / 2.0 +
                                                c.dx * 4 + ox)),
                            by = int(std::round(sn * ax + cs * ay + (textureGrid - 1) / 2.0 +
                                                c.dy * 4 + oy));
                        if (bx < 0 || by < 0 || bx >= textureGrid || by >= textureGrid ||
                            b.cells[(by / 4) * grid + bx / 4].weight == 0)
                            continue;
                        double av = a.texture[y * textureGrid + x],
                               bv = b.texture[by * textureGrid + bx];
                        ab += av * bv;
                        aa += av * av;
                        bb += bv * bv;
                        ++n;
                    }
                if (n > 200 && aa > 0 && bb > 0)
                    bestTexture = std::max(bestTexture, ab / std::sqrt(aa * bb));
            }
        best = std::max(best, 0.65 * c.value + 0.35 * (bestTexture + 1) / 2);
    }
    return std::clamp(best, 0.0, 1.0);
}
} // namespace
extern "C" __declspec(dllexport) int KISA_Enroll(int t, unsigned char* i, unsigned char* o, int* s,
                                                 int* e, char* m) {
    return extract(t, i, o, s, e, m);
}
extern "C" __declspec(dllexport) int KISA_Attempt(int t, unsigned char* i, unsigned char* o, int* s,
                                                  int* e, char* m) {
    return extract(t, i, o, s, e, m);
}
extern "C" __declspec(dllexport) int KISA_Compare(unsigned char* a, unsigned char* b, double* score,
                                                  int* error, char* message) {
    if (!a || !b || !score || !error || !message)
        return 0;
    *score = 0;
    *error = 0;
    message[0] = 0;
    Feature fa{}, fb{};
    std::memcpy(&fa, a, sizeof(fa));
    std::memcpy(&fb, b, sizeof(fb));
    if (fa.tag != magic || fb.tag != magic) {
        *error = 2;
        strcpy_s(message, 256, "Invalid template version");
        return 0;
    }
    *score = match(fa, fb);
    return 1;
}
