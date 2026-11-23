#include "api.h"
#include "minutiae.h"
#ifdef FP_GRAPH_MATCHER
#include "minutiae_graph.h"
#endif
#define FRFXLL_EXPORT __declspec(dllimport)
#include <FRFXLL.h>
#include <memory>
#include <string>
#include <vector>
namespace {
struct Handle {
    FRFXLL_HANDLE value = nullptr;
    ~Handle() {
        if (value)
            FRFXLLCloseHandle(&value);
    }
};
int extractPixels(int width, int height, int dpi, unsigned char* image, unsigned char* output,
                  int* size, int* error, char* message) {
    if (!size || !error || !message)
        return 0;
    *size = 0;
    *error = 0;
    message[0] = 0;
    try {
        if (!image || !output)
            throw std::runtime_error("Null image or output");
        if (width < 150 || height < 150 || width > 2000 || height > 2000 || dpi < 300 || dpi > 1024)
            throw std::runtime_error("Unsupported dimensions or resolution");
        unsigned w = width, h = height;
        Handle context, features;
        auto check = [](FRFXLL_RESULT result) {
            if (!FRFXLL_SUCCESS(result))
                throw std::runtime_error("Feature extraction failed: " + std::to_string(result));
        };
        check(FRFXLLCreateLibraryContext(&context.value));
        check(FRFXLLCreateFeatureSetFromRaw(context.value, image, w * h, w, h, dpi,
                                            FRFXLL_FEX_ENABLE_ENHANCEMENT, &features.value));
        unsigned count = 0, resolution = 0;
        check(FRFXLLGetMinutiaInfo(features.value, &count, &resolution));
        if (count < 3 || count > 256 || resolution < 100 || resolution > 2000)
            throw std::runtime_error("Insufficient or invalid minutiae");
        std::vector<FRFXLL_Basic_19794_2_Minutia> points(count);
        check(
            FRFXLLGetMinutiae(features.value, BASIC_19794_2_MINUTIA_STRUCT, &count, points.data()));
        std::stable_sort(points.begin(), points.end(), [](auto a, auto b) { return a.q > b.q; });
        minutiae::Feature result;
        result.count = std::min(count, unsigned(minutiae::maxPoints));
        for (unsigned i = 0; i < result.count; ++i) {
            auto p = points[i];
            result.points[i] = {float(p.x * 500.0 / resolution), float(p.y * 500.0 / resolution),
                                float(-p.a * 2 * minutiae::pi / 256), float(p.q / 100.0)};
        }
        minutiae::describe(result);
        static_assert(sizeof(result) < capacity);
        std::memcpy(output, &result, sizeof(result));
        *size = sizeof(result);
        return 1;
    } catch (const std::exception& e) {
        *error = 1;
        strncpy_s(message, 256, e.what(), _TRUNCATE);
        return 0;
    }
}
int extract(int type, unsigned char* image, unsigned char* output, int* size, int* error,
            char* message) {
    UINT w = 0, h = 0;
    if (!dimensions(type, w, h)) {
        if (size)
            *size = 0;
        if (error)
            *error = 3;
        if (message)
            strcpy_s(message, 256, "Unsupported iType");
        return 0;
    }
    return extractPixels(w, h, 500, image, output, size, error, message);
}
} // namespace
extern "C" __declspec(dllexport) int FP_ExtractImage(int w, int h, int dpi, unsigned char* i,
                                                     unsigned char* o, int* s, int* e, char* m) {
    return extractPixels(w, h, dpi, i, o, s, e, m);
}
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
    if (!score || !error || !message)
        return 0;
    *score = 0;
    *error = 0;
    message[0] = 0;
    try {
        if (!a || !b)
            throw std::runtime_error("Null template");
        auto fa = std::make_unique<minutiae::Feature>(), fb = std::make_unique<minutiae::Feature>();
        std::memcpy(fa.get(), a, sizeof(*fa));
        std::memcpy(fb.get(), b, sizeof(*fb));
#ifdef FP_GRAPH_MATCHER
        *score = minutiae::graph::compare(*fa, *fb);
#else
        *score = minutiae::compare(*fa, *fb);
#endif
        return 1;
    } catch (const std::exception& e) {
        *error = 2;
        strncpy_s(message, 256, e.what(), _TRUNCATE);
        return 0;
    }
}
