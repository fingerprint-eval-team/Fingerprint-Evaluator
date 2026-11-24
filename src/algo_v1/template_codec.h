#pragma once
#include "errors.h"
#include "minutiae_orientation.h"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <limits>

namespace wire {
struct Header {
    uint32_t magic = 0x46505632, version = 2, bytes = 0, checksum = 0;
    uint32_t width = 0, height = 0, dpi = 0, cols = 0, rows = 0, count = 0;
};
static_assert(sizeof(Header) == 40 && sizeof(minutiae::Point) == 16);
static_assert(sizeof(std::array<float, minutiae::descriptorSize>) == 1536);

inline bool accessible(const void* pointer, size_t bytes, bool write = false) {
    uintptr_t current = reinterpret_cast<uintptr_t>(pointer);
    if (!pointer || bytes > std::numeric_limits<uintptr_t>::max() - current)
        return false;
    const uintptr_t end = current + bytes;
    while (current < end) {
        MEMORY_BASIC_INFORMATION info{};
        if (!VirtualQuery(reinterpret_cast<void*>(current), &info, sizeof(info)) ||
            info.State != MEM_COMMIT || (info.Protect & (PAGE_GUARD | PAGE_NOACCESS)))
            return false;
        DWORD protection = info.Protect & 0xff;
        bool writable = protection == PAGE_READWRITE || protection == PAGE_WRITECOPY ||
                        protection == PAGE_EXECUTE_READWRITE ||
                        protection == PAGE_EXECUTE_WRITECOPY;
        bool readable = writable || protection == PAGE_READONLY || protection == PAGE_EXECUTE_READ;
        if (write ? !writable : !readable)
            return false;
        uintptr_t next = reinterpret_cast<uintptr_t>(info.BaseAddress) + info.RegionSize;
        if (next <= current)
            return false;
        current = std::min(next, end);
    }
    return true;
}

inline uint32_t checksum(const unsigned char* bytes, size_t count) {
    uint32_t value = 2166136261u;
    for (size_t i = 0; i < count; ++i)
        if (i < 12 || i >= 16)
            value = (value ^ bytes[i]) * 16777619u;
    return value;
}

inline size_t payloadBytes(const Header& h) {
    return sizeof(Header) +
           size_t(h.count) *
               (sizeof(minutiae::Point) + sizeof(std::array<float, minutiae::descriptorSize>)) +
           size_t(h.cols) * h.rows * sizeof(minutiae::orientation::Cell);
}

inline Header inspect(const unsigned char* data) {
    if (!accessible(data, sizeof(Header)))
        throw AlgorithmError(InvalidTemplate, "Unreadable template header");
    Header h;
    std::memcpy(&h, data, sizeof(h));
    if (h.magic != 0x46505632 || h.version != 2)
        throw AlgorithmError(UnsupportedTemplate,
                             "Template v2 required; regenerate both templates");
    if (h.width < 150 || h.height < 150 || h.width > 2000 || h.height > 2000 || h.dpi < 300 ||
        h.dpi > 1024 || h.count < 3 || h.count > minutiae::maxPoints ||
        h.cols != (h.width * 500 + h.dpi * 16 - 1) / (h.dpi * 16) ||
        h.rows != (h.height * 500 + h.dpi * 16 - 1) / (h.dpi * 16) ||
        h.cols > minutiae::orientation::maxSide || h.rows > minutiae::orientation::maxSide)
        throw AlgorithmError(InvalidTemplate, "Invalid template dimensions or point count");
    if (h.bytes != payloadBytes(h) || h.bytes >= capacity || !accessible(data, h.bytes))
        throw AlgorithmError(InvalidTemplate, "Invalid or unreadable template payload");
    if (h.checksum != checksum(data, h.bytes))
        throw AlgorithmError(TemplateChecksum, "Template checksum mismatch");
    return h;
}

inline int encode(const minutiae::orientation::Template& value, unsigned char* data) {
    Header h;
    h.width = value.width;
    h.height = value.height;
    h.dpi = value.dpi;
    h.cols = value.cols;
    h.rows = value.rows;
    h.count = value.base.count;
    h.bytes = uint32_t(payloadBytes(h));
    if (h.bytes >= capacity || !accessible(data, h.bytes, true))
        throw AlgorithmError(InvalidOutput, "Output buffer is not writable");
    std::memcpy(data, &h, sizeof(h));
    size_t offset = sizeof(h), count = h.count * sizeof(minutiae::Point);
    std::memcpy(data + offset, value.base.points, count);
    offset += count;
    count = size_t(h.count) * sizeof(std::array<float, minutiae::descriptorSize>);
    std::memcpy(data + offset, value.base.descriptors.data(), count);
    offset += count;
    std::memcpy(data + offset, value.cells.data(), value.cells.size() * sizeof(value.cells[0]));
    h.checksum = checksum(data, h.bytes);
    std::memcpy(data, &h, sizeof(h));
    return int(h.bytes);
}

inline minutiae::orientation::Template decode(const unsigned char* data) {
    Header h = inspect(data);
    minutiae::orientation::Template value;
    value.width = h.width;
    value.height = h.height;
    value.dpi = h.dpi;
    value.cols = h.cols;
    value.rows = h.rows;
    value.base.count = h.count;
    value.base.descriptors.resize(h.count);
    value.cells.resize(size_t(h.cols) * h.rows);
    size_t offset = sizeof(h), count = h.count * sizeof(minutiae::Point);
    std::memcpy(value.base.points, data + offset, count);
    offset += count;
    count = size_t(h.count) * sizeof(std::array<float, minutiae::descriptorSize>);
    std::memcpy(value.base.descriptors.data(), data + offset, count);
    offset += count;
    std::memcpy(value.cells.data(), data + offset, value.cells.size() * sizeof(value.cells[0]));
    if (!minutiae::orientation::valid(value))
        throw AlgorithmError(InvalidTemplate, "Nonfinite or invalid template values");
    return value;
}
} // namespace wire
