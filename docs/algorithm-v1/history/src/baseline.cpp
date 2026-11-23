#include "api.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
// Baseline: locally normalized, pooled gradient orientation descriptors.
// This is a measurable starting point, not a minutiae matcher.
namespace {
constexpr int cells = 16, bins = 8, count = cells * cells * bins;
struct Feature {
    unsigned int magic;
    float data[count];
};
int extract(int type, unsigned char* image, unsigned char* output, int* size, int* error,
            char* message) {
    if (!image || !output || !size || !error || !message)
        return 0;
    *size = 0;
    *error = 0;
    message[0] = 0;
    UINT w = 0, h = 0;
    if (!dimensions(type, w, h)) {
        *error = 1;
        strcpy_s(message, 256, "Unsupported iType");
        return 0;
    }
    Feature f{};
    f.magic = 0x46503031;
    for (UINT y = 1; y < h - 1; ++y)
        for (UINT x = 1; x < w - 1; ++x) {
            const double gx = double(image[y * w + x + 1]) - image[y * w + x - 1];
            const double gy = double(image[(y + 1) * w + x]) - image[(y - 1) * w + x];
            double angle = std::atan2(gy, gx);
            if (angle < 0)
                angle += 3.141592653589793;
            if (angle >= 3.141592653589793)
                angle -= 3.141592653589793;
            int b = std::min(bins - 1, int(angle * bins / 3.141592653589793));
            int c = int(y * cells / h) * cells + int(x * cells / w);
            f.data[c * bins + b] += float(std::sqrt(gx * gx + gy * gy));
        }
    for (int c = 0; c < cells * cells; ++c) {
        double norm = 0;
        for (int b = 0; b < bins; ++b)
            norm += double(f.data[c * bins + b]) * f.data[c * bins + b];
        norm = std::sqrt(norm) + 1e-9;
        for (int b = 0; b < bins; ++b)
            f.data[c * bins + b] = float(f.data[c * bins + b] / norm);
    }
    std::memcpy(output, &f, sizeof(f));
    *size = sizeof(f);
    return 1;
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
    *error = 0;
    message[0] = 0;
    *score = 0;
    Feature fa{}, fb{};
    std::memcpy(&fa, a, sizeof(fa));
    std::memcpy(&fb, b, sizeof(fb));
    if (fa.magic != 0x46503031 || fb.magic != 0x46503031) {
        *error = 2;
        strcpy_s(message, 256, "Invalid template");
        return 0;
    }
    double dot = 0, na = 0, nb = 0;
    for (int k = 0; k < count; ++k) {
        dot += double(fa.data[k]) * fb.data[k];
        na += double(fa.data[k]) * fa.data[k];
        nb += double(fb.data[k]) * fb.data[k];
    }
    if (na == 0 || nb == 0) {
        *error = 3;
        strcpy_s(message, 256, "No image structure");
        return 0;
    }
    *score = std::clamp(dot / std::sqrt(na * nb), 0.0, 1.0);
    return 1;
}
