#include "../src/algo_v1/api.h"
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace fs = std::filesystem;
struct Library {
    HMODULE handle = nullptr;
    ~Library() {
        if (handle)
            FreeLibrary(handle);
    }
};
std::vector<unsigned char> readRaw(const fs::path& path, size_t expected) {
    if (fs::file_size(path) != expected)
        throw std::runtime_error("RAW size does not match the selected type");
    std::vector<unsigned char> bytes(expected);
    std::ifstream input(path, std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size()))
        throw std::runtime_error("Cannot read RAW image");
    return bytes;
}
int wmain(int argc, wchar_t** argv) {
    try {
        if (argc != 5)
            throw std::runtime_error("Usage: fp_compare DLL TYPE ENROLL.raw ATTEMPT.raw");
        const int type = std::stoi(argv[2]);
        UINT width = 0, height = 0;
        if (type < 1 || type > 3 || !dimensions(type, width, height))
            throw std::runtime_error("TYPE must be 1, 2 or 3");
        auto firstImage = readRaw(argv[3], size_t(width) * height);
        auto secondImage = readRaw(argv[4], size_t(width) * height);
        Library library;
        auto dll = fs::absolute(argv[1]);
        library.handle =
            LoadLibraryExW(dll.c_str(), nullptr,
                           LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!library.handle)
            throw std::runtime_error(
                "Cannot load DLL; check architecture, FRFXLL and VC++ runtime");
        auto enroll = reinterpret_cast<Extract>(GetProcAddress(library.handle, "KISA_Enroll"));
        auto attempt = reinterpret_cast<Extract>(GetProcAddress(library.handle, "KISA_Attempt"));
        auto compare = reinterpret_cast<Compare>(GetProcAddress(library.handle, "KISA_Compare"));
        if (!enroll || !attempt || !compare)
            throw std::runtime_error("Required KISA export is missing");
        std::vector<unsigned char> first(capacity), second(capacity);
        auto extract = [&](Extract fn, std::vector<unsigned char>& image,
                           std::vector<unsigned char>& result) {
            int bytes = 0, error = 0;
            char message[256]{};
            if (fn(type, image.data(), result.data(), &bytes, &error, message) != 1 || error)
                throw std::runtime_error(message[0] ? message : "Extraction failed");
            if (bytes != 551512)
                throw std::runtime_error("Expected the selected v1 template format");
        };
        extract(enroll, firstImage, first);
        extract(attempt, secondImage, second);
        double score = 0;
        int error = 0;
        char message[256]{};
        if (compare(first.data(), second.data(), &score, &error, message) != 1 || error)
            throw std::runtime_error(message[0] ? message : "Comparison failed");
        if (!std::isfinite(score) || score < 0 || score > 1)
            throw std::runtime_error("Invalid comparison score");
        std::cout << std::setprecision(17) << score << '\n';
        return 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
