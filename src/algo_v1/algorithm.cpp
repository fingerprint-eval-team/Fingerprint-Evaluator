#include "api.h"
#include "errors.h"
#include "fingerjet_loader.h"
#include "photometric.h"
#include "template_codec.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <new>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
template <class Function> int boundary(int* error, char* message, Function function) noexcept {
    if (!error || !message)
        return 0;
    *error = Success;
    message[0] = 0;
    try {
        function();
        return 1;
    } catch (const AlgorithmError& e) {
        *error = e.code;
        strncpy_s(message, 256, e.what(), _TRUNCATE);
    } catch (const std::bad_alloc&) {
        *error = OutOfMemory;
        strcpy_s(message, 256, "Memory allocation failed");
    } catch (const std::exception& e) {
        *error = StandardException;
        strncpy_s(message, 256, e.what(), _TRUNCATE);
    } catch (...) {
        *error = UnknownException;
        strcpy_s(message, 256, "Unknown C++ exception");
    }
    return 0;
}

void extract(int type, unsigned char* image, unsigned char* output, int* size,
             std::unique_ptr<FingerJet>& owner) {
    if (!size || !image || !output)
        throw AlgorithmError(NullArgument, "Null extraction argument");
    unsigned width = 0, height = 0;
    if (!dimensions(type, width, height))
        throw AlgorithmError(UnsupportedType, "Supported project iTypes are 1, 2 and 3");
    if (!wire::accessible(image, size_t(width) * height))
        throw AlgorithmError(InvalidImage, "Unreadable input image");
    auto pixels = photometric::normalize(image, size_t(width) * height);
    // The public wrapper retains the module until boundary() has caught and
    // destroyed any vendor exception. Unloading during unwinding invalidates
    // the vendor's exception metadata/destructor addresses.
    owner = std::make_unique<FingerJet>();
    FingerJet& library = *owner;
    FeatureHandle context{library}, features{library};
    auto check = [](FRFXLL_RESULT status) {
        if (!FRFXLL_SUCCESS(status))
            throw AlgorithmError(ExtractorFailure, "FingerJet status " + std::to_string(status));
    };
    check(library.createContext(&context.value));
    // The three project input types are all 500 ppi; do not infer DPI from size.
    check(library.createFeatures(context.value, pixels.data(), width * height, width, height, 500,
                                 FRFXLL_FEX_ENABLE_ENHANCEMENT, &features.value));
    unsigned count = 0, resolution = 0;
    check(library.getInfo(features.value, &count, &resolution));
    if (count < 3)
        throw AlgorithmError(TooFewMinutiae, "Fewer than three extracted minutiae");
    if (count > 65536 || resolution < 100 || resolution > 2000)
        throw AlgorithmError(InvalidExtractorData, "Invalid extractor count or resolution");
    std::vector<FRFXLL_Basic_19794_2_Minutia> points(count);
    const unsigned available = count;
    check(library.getMinutiae(features.value, BASIC_19794_2_MINUTIA_STRUCT, &count, points.data()));
    if (count < 3 || count > available)
        throw AlgorithmError(InvalidExtractorData, "Extractor returned inconsistent count");
    points.resize(count);
    std::stable_sort(points.begin(), points.end(), [](auto a, auto b) { return a.q > b.q; });
    minutiae::Feature feature;
    feature.count = std::min(count, unsigned(minutiae::maxPoints));
    for (unsigned i = 0; i < feature.count; ++i) {
        auto p = points[i];
        feature.points[i] = {float(p.x * 500.0 / resolution), float(p.y * 500.0 / resolution),
                             float(-p.a * 2 * minutiae::pi / 256), float(p.q / 100.0)};
    }
    minutiae::describe(feature);
    minutiae::orientation::Template value;
    minutiae::orientation::encode(value, feature, pixels.data(), width, height, 500);
    *size = wire::encode(value, output);
}
} // namespace

extern "C" __declspec(dllexport) int __cdecl KISA_Enroll(int type, unsigned char* image,
                                                         unsigned char* output, int* size,
                                                         int* error, char* message) {
    if (size)
        *size = 0;
    std::unique_ptr<FingerJet> library;
    return boundary(error, message, [&] { extract(type, image, output, size, library); });
}

extern "C" __declspec(dllexport) int __cdecl KISA_Attempt(int type, unsigned char* image,
                                                          unsigned char* output, int* size,
                                                          int* error, char* message) {
    if (size)
        *size = 0;
    std::unique_ptr<FingerJet> library;
    return boundary(error, message, [&] { extract(type, image, output, size, library); });
}

extern "C"
    __declspec(dllexport) int __cdecl KISA_Compare(unsigned char* first, unsigned char* second,
                                                   double* score, int* error, char* message) {
    if (score)
        *score = 0;
    return boundary(error, message, [&] {
        if (!first || !second || !score)
            throw AlgorithmError(NullArgument, "Null comparison argument");
        auto a = wire::decode(first), b = wire::decode(second);
        double value = minutiae::orientation::compare(a, b).score;
#ifdef FP_THREE_DECIMAL_SCORE
        value = std::round(value * 1000.0) / 1000.0;
#endif
        *score = value;
    });
}
