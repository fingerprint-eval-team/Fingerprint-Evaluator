#include "api.h"
#include "minutiae.h"
#ifdef FP_ORIENTATION_TEMPLATE
#include "minutiae_orientation.h"
#endif
#ifdef FP_CONTRAST_NORMALIZATION
#include "photometric.h"
#endif
#ifdef FP_GRAPH_MATCHER
#include "minutiae_graph.h"
#endif
#ifdef FP_OVERLAP_MATCHER
#include "minutiae_overlap.h"
#endif
#ifdef FP_SUPPORTED_OVERLAP_MATCHER
#include "minutiae_supported_overlap.h"
#endif
#ifdef FP_BOZORTH_MATCHER
#include "bozorth_adapter.h"
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
#ifdef FP_CONTRAST_NORMALIZATION
        auto normalized = photometric::normalize(image, size_t(w) * h);
        image = normalized.data();
#endif
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
#ifdef FP_ORIENTATION_TEMPLATE
        auto extended = std::make_unique<minutiae::orientation::Template>();
        minutiae::orientation::encode(*extended, result, image, w, h, dpi);
        static_assert(sizeof(*extended) < capacity);
        std::memcpy(output, extended.get(), sizeof(*extended));
        *size = sizeof(*extended);
#else
        static_assert(sizeof(result) < capacity);
        std::memcpy(output, &result, sizeof(result));
        *size = sizeof(result);
#endif
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
#ifdef FP_ORIENTATION_TEMPLATE
        if (!minutiae::orientation::header(a) || !minutiae::orientation::header(b))
            throw std::runtime_error("Orientation template version required; regenerate features");
        auto fa = std::make_unique<minutiae::orientation::Template>(),
             fb = std::make_unique<minutiae::orientation::Template>();
        std::memcpy(fa.get(), a, sizeof(*fa));
        std::memcpy(fb.get(), b, sizeof(*fb));
        *score = minutiae::orientation::compare(*fa, *fb).score;
#else
        auto fa = std::make_unique<minutiae::Feature>(), fb = std::make_unique<minutiae::Feature>();
        std::memcpy(fa.get(), a, sizeof(*fa));
        std::memcpy(fb.get(), b, sizeof(*fb));
#if defined(FP_BOZORTH_MATCHER)
        *score = minutiae::bozorth::compare(*fa, *fb);
#elif defined(FP_GRAPH_MATCHER)
#ifdef FP_SUPPORTED_OVERLAP_MATCHER
        *score = minutiae::graph::compare(*fa, *fb, nullptr,
                                         minutiae::supported_overlap::scale(*fa, *fb));
#elif defined(FP_OVERLAP_MATCHER)
        *score = minutiae::graph::compare(*fa, *fb, nullptr, minutiae::overlap::scale(*fa, *fb));
#else
        *score = minutiae::graph::compare(*fa, *fb);
#endif
#else
        *score = minutiae::compare(*fa, *fb);
#endif
#endif
#ifdef FP_THREE_DECIMAL_SCORE
        *score = std::round(*score * 1000.0) / 1000.0;
#endif
        return 1;
    } catch (const std::exception& e) {
        *error = 2;
        strncpy_s(message, 256, e.what(), _TRUNCATE);
        return 0;
    }
}
#ifdef FP_ORIENTATION_TEMPLATE
// Research diagnostic extension; the evaluator still uses KISA_Compare.
extern "C" __declspec(dllexport) int FP_OrientationEvidence(unsigned char* a, unsigned char* b,
                                                             double* values) {
    if (!values) return 0;
    std::fill(values, values + 5, 0.0);
    try {
        if (!a || !b || !minutiae::orientation::header(a) || !minutiae::orientation::header(b))
            return 0;
        auto first = std::make_unique<minutiae::orientation::Template>(),
             second = std::make_unique<minutiae::orientation::Template>();
        std::memcpy(first.get(), a, sizeof(*first)); std::memcpy(second.get(), b, sizeof(*second));
        auto result = minutiae::orientation::compare(*first, *second);
        values[0] = result.geometryScore; values[1] = result.field.factor;
        values[2] = result.field.meanReliability; values[3] = double(result.field.samples);
        values[4] = result.score; return 1;
    } catch (...) {return 0;}
}
#endif
