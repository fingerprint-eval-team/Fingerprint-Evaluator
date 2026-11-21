#pragma once
#include <algorithm>
#include <array>
#include <cstdint>
#include <stdexcept>
#include <vector>

namespace photometric {
// Fixed image-wide 1%/99% order statistics. No DB/type/noise labels are used.
inline std::vector<unsigned char> normalize(const unsigned char* image, size_t size) {
    if (!image || !size)
        throw std::runtime_error("Empty contrast-normalization input");
    std::array<size_t, 256> histogram{};
    for (size_t i = 0; i < size; ++i)
        ++histogram[image[i]];
    auto quantile = [&](size_t rank) {
        size_t sum = 0;
        for (int i = 0; i < 256; ++i) {
            sum += histogram[i];
            if (sum > rank)
                return i;
        }
        return 255;
    };
    int low = quantile((size - 1) / 100), high = quantile((size - 1) * 99 / 100), span = high - low;
    std::vector<unsigned char> result(image, image + size);
    // Do not magnify nearly flat input into apparent fingerprint structure.
    if (span < 16)
        return result;
    for (auto& value : result) {
        if (value <= low)
            value = 0;
        else if (value >= high)
            value = 255;
        else
            value = static_cast<unsigned char>(((int(value) - low) * 255 + span / 2) / span);
    }
    return result;
}
} // namespace photometric
