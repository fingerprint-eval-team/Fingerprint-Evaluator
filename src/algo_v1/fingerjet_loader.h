#pragma once
#include "errors.h"
#define FRFXLL_EXPORT
#include <FRFXLL.h>
#include <filesystem>
#include <string>
#include <vector>

class FingerJet {
    HMODULE module_ = nullptr;
    inline static const unsigned char moduleAnchor_ = 0;

    static const std::wstring& libraryPath() {
        static const std::wstring path = [] {
            HMODULE self = nullptr;
            if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                    reinterpret_cast<LPCWSTR>(&moduleAnchor_), &self)) {
                throw AlgorithmError(DependencyLoad, "Cannot locate algorithm module");
            }
            std::vector<wchar_t> name(32768);
            DWORD length = GetModuleFileNameW(self, name.data(), DWORD(name.size()));
            if (!length || length >= name.size())
                throw AlgorithmError(DependencyLoad, "Cannot read algorithm module path");
            return (std::filesystem::path(std::wstring(name.data(), length)).parent_path() /
                    L"FRFXLL.dll")
                .wstring();
        }();
        return path;
    }

    template <class T> T symbol(const char* name) {
        auto address = GetProcAddress(module_, name);
        if (!address)
            throw AlgorithmError(DependencySymbol, std::string("FRFXLL export missing: ") + name);
        return reinterpret_cast<T>(address);
    }

  public:
    decltype(&FRFXLLCreateLibraryContext) createContext = nullptr;
    decltype(&FRFXLLCreateFeatureSetFromRaw) createFeatures = nullptr;
    decltype(&FRFXLLGetMinutiaInfo) getInfo = nullptr;
    decltype(&FRFXLLGetMinutiae) getMinutiae = nullptr;
    decltype(&FRFXLLCloseHandle) close = nullptr;

    FingerJet() {
        // Called from extraction, never DllMain. Resolve relative to THIS DLL,
        // not the evaluator EXE, current directory or PATH.
        module_ =
            LoadLibraryExW(libraryPath().c_str(), nullptr,
                           LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
        if (!module_)
            throw AlgorithmError(DependencyLoad, "Cannot load sibling FRFXLL.dll; Windows error " +
                                                     std::to_string(GetLastError()));
        try {
            createContext = symbol<decltype(createContext)>("FRFXLLCreateLibraryContext");
            createFeatures = symbol<decltype(createFeatures)>("FRFXLLCreateFeatureSetFromRaw");
            getInfo = symbol<decltype(getInfo)>("FRFXLLGetMinutiaInfo");
            getMinutiae = symbol<decltype(getMinutiae)>("FRFXLLGetMinutiae");
            close = symbol<decltype(close)>("FRFXLLCloseHandle");
        } catch (...) {
            FreeLibrary(module_);
            module_ = nullptr;
            throw;
        }
    }
    FingerJet(const FingerJet&) = delete;
    FingerJet& operator=(const FingerJet&) = delete;
    ~FingerJet() noexcept {
        if (module_)
            FreeLibrary(module_);
    }
};

struct FeatureHandle {
    FingerJet& library;
    FRFXLL_HANDLE value = nullptr;
    ~FeatureHandle() noexcept {
        try {
            if (value)
                library.close(&value);
        } catch (...) {
            // Cleanup must not throw across a DLL boundary or during unwinding.
        }
    }
};
