#pragma once
#include "api.h"
#include <algorithm>
#include <chrono>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
namespace fs = std::filesystem;
// Surround untrusted DLL buffers with inaccessible pages. Put the requested
// byte range immediately before the trailing guard so one-byte overruns fault.
class GuardedBuffer {
    void* allocation = nullptr;
    void* pages = nullptr;
    size_t pageBytes = 0;

  public:
    unsigned char* data = nullptr;
    explicit GuardedBuffer(size_t size) {
        SYSTEM_INFO info{};
        GetSystemInfo(&info);
        size_t page = info.dwPageSize;
        pageBytes = ((size + page - 1) / page) * page;
        allocation =
            VirtualAlloc(nullptr, pageBytes + 2 * page, MEM_RESERVE | MEM_COMMIT, PAGE_NOACCESS);
        if (!allocation)
            throw std::runtime_error("Cannot allocate guarded buffer");
        pages = static_cast<unsigned char*>(allocation) + page;
        DWORD old = 0;
        if (!VirtualProtect(pages, pageBytes, PAGE_READWRITE, &old)) {
            VirtualFree(allocation, 0, MEM_RELEASE);
            allocation = nullptr;
            throw std::runtime_error("Cannot protect DLL buffer");
        }
        data = static_cast<unsigned char*>(pages) + pageBytes - size;
    }
    void readOnly() {
        DWORD old = 0;
        if (!VirtualProtect(pages, pageBytes, PAGE_READONLY, &old))
            throw std::runtime_error("Cannot protect DLL input");
    }
    ~GuardedBuffer() {
        if (allocation)
            VirtualFree(allocation, 0, MEM_RELEASE);
    }
    GuardedBuffer(const GuardedBuffer&) = delete;
    GuardedBuffer& operator=(const GuardedBuffer&) = delete;
};
inline std::string utf8(const fs::path& p) {
    return p.u8string();
}
inline std::string json(const std::string& s) {
    std::ostringstream o;
    o << '"';
    for (unsigned char c : s) {
        switch (c) {
        case '"':
            o << "\\\"";
            break;
        case '\\':
            o << "\\\\";
            break;
        case '\n':
            o << "\\n";
            break;
        case '\r':
            o << "\\r";
            break;
        case '\t':
            o << "\\t";
            break;
        default:
            if (c < 32)
                o << "\\u00" << std::hex << std::setw(2) << std::setfill('0') << int(c) << std::dec;
            else
                o << char(c);
        }
    }
    return o.str() + '"';
}
inline std::vector<std::string> split(const std::string& s) {
    std::vector<std::string> v;
    size_t start = 0;
    for (;;) {
        auto pos = s.find('\t', start);
        v.push_back(s.substr(start, pos - start));
        if (pos == std::string::npos)
            break;
        start = pos + 1;
    }
    return v;
}
inline void safeField(const std::string& s) {
    if (s.find_first_of("\t\r\n") != std::string::npos)
        throw std::runtime_error("Tabs/newlines in plan fields are forbidden");
}
struct Outcome {
    std::string status = "FAIL", stage = "worker", message;
    int error = 0, bytes = 0;
    double score = 0, ms = 0;
    DWORD exitCode = 0;
};
inline void saveOutcome(const fs::path& path, const Outcome& r) {
    std::ofstream o(path, std::ios::trunc);
    if (!o)
        throw std::runtime_error("Cannot write worker output");
    std::string msg = r.message;
    std::replace(msg.begin(), msg.end(), '\t', ' ');
    std::replace(msg.begin(), msg.end(), '\n', ' ');
    std::replace(msg.begin(), msg.end(), '\r', ' ');
    o << std::setprecision(17) << r.status << '\t' << r.stage << '\t' << r.error << '\t' << r.score
      << '\t' << r.ms << '\t' << r.bytes << '\t' << msg << '\n';
}
inline Outcome readOutcome(const fs::path& path) {
    Outcome r;
    std::ifstream in(path);
    std::string line;
    std::getline(in, line);
    auto f = split(line);
    if (f.size() != 7) {
        r.message = "Missing or malformed worker output";
        return r;
    }
    try {
        r.status = f[0];
        r.stage = f[1];
        r.error = std::stoi(f[2]);
        r.score = std::stod(f[3]);
        r.ms = std::stod(f[4]);
        r.bytes = std::stoi(f[5]);
        r.message = f[6];
        if ((r.status != "OK" && r.status != "FAIL" && r.status != "RUNNING") ||
            !std::isfinite(r.ms) || r.ms < 0 || !std::isfinite(r.score))
            throw std::runtime_error("Invalid worker fields");
    } catch (...) {
        r = Outcome{};
        r.message = "Malformed worker fields";
    }
    return r;
}
inline std::string outcomeJson(const Outcome& r) {
    std::ostringstream o;
    o << std::setprecision(17) << "{\"status\":" << json(r.status) << ",\"stage\":" << json(r.stage)
      << ",\"error_code\":" << r.error << ",\"message\":" << json(r.message) << ",\"score\":";
    if (r.status == "OK" && r.stage == "compare")
        o << r.score;
    else
        o << "null";
    o << ",\"call_ms\":" << r.ms << ",\"template_bytes\":" << r.bytes
      << ",\"exit_code\":" << r.exitCode << "}";
    return o.str();
}
inline std::vector<unsigned char> readTemplate(const fs::path& path) {
    auto size = fs::file_size(path);
    if (size == 0 || size >= capacity)
        throw std::runtime_error("Invalid template file size");
    std::vector<unsigned char> b(capacity, 0);
    std::ifstream f(path, std::ios::binary);
    f.read(reinterpret_cast<char*>(b.data()), size);
    if (!f)
        throw std::runtime_error("Cannot read template");
    return b;
}
