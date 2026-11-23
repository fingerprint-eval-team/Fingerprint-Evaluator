#pragma once
#include "core.h"
#include <random>
#include <wincodec.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
inline void hcheck(HRESULT hr) {
    if (FAILED(hr))
        throw std::runtime_error("WIC image decoding failed");
}
inline std::vector<unsigned char> loadImage(const fs::path& path, UINT w, UINT h) {
    auto extension = path.extension().wstring();
    std::transform(extension.begin(), extension.end(), extension.begin(), ::towlower);
    if (extension == L".raw") {
        if (fs::file_size(path) != size_t(w) * h)
            throw std::runtime_error("RAW byte length does not match iType");
        std::vector<unsigned char> pixels(size_t(w) * h);
        std::ifstream in(path, std::ios::binary);
        in.read(reinterpret_cast<char*>(pixels.data()), pixels.size());
        if (!in)
            throw std::runtime_error("Cannot read RAW pixels");
        return pixels;
    }
    ComPtr<IWICImagingFactory> factory;
    hcheck(CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
                            IID_PPV_ARGS(&factory)));
    ComPtr<IWICBitmapDecoder> decoder;
    hcheck(factory->CreateDecoderFromFilename(path.c_str(), nullptr, GENERIC_READ,
                                              WICDecodeMetadataCacheOnLoad, &decoder));
    ComPtr<IWICBitmapFrameDecode> frame;
    hcheck(decoder->GetFrame(0, &frame));
    UINT iw = 0, ih = 0;
    hcheck(frame->GetSize(&iw, &ih));
    if (iw != w || ih != h)
        throw std::runtime_error("Image dimensions do not match iType");
    ComPtr<IWICFormatConverter> converter;
    hcheck(factory->CreateFormatConverter(&converter));
    hcheck(converter->Initialize(frame.Get(), GUID_WICPixelFormat8bppGray, WICBitmapDitherTypeNone,
                                 nullptr, 0, WICBitmapPaletteTypeCustom));
    std::vector<unsigned char> pixels(w * h);
    hcheck(converter->CopyPixels(nullptr, w, UINT(pixels.size()), pixels.data()));
    return pixels;
}
inline void addNoise(std::vector<unsigned char>& p, UINT w, UINT h, const std::string& profile,
                     unsigned seed) {
    std::mt19937 rng(seed);
    auto byte = [](double v) {
        return static_cast<unsigned char>(std::clamp(std::round(v), 0.0, 255.0));
    };
    if (profile == "clean")
        return;
    if (profile == "gaussian15" || profile == "gaussian30") {
        std::normal_distribution<double> d(0, profile == "gaussian15" ? 15 : 30);
        for (auto& x : p)
            x = byte(x + d(rng));
    } else if (profile == "impulse2") {
        for (auto& x : p) {
            auto r = rng() % 10000;
            if (r < 100)
                x = 0;
            else if (r < 200)
                x = 255;
        }
    } else if (profile == "brightness") {
        for (auto& x : p)
            x = byte(x * 0.65 + 35);
    } else if (profile == "blur1") {
        auto src = p;
        int kernel[3] = {1, 2, 1};
        for (UINT y = 0; y < h; ++y)
            for (UINT x = 0; x < w; ++x) {
                int sum = 0;
                for (int dy = -1; dy <= 1; ++dy)
                    for (int dx = -1; dx <= 1; ++dx) {
                        auto yy = std::clamp(int(y) + dy, 0, int(h) - 1),
                             xx = std::clamp(int(x) + dx, 0, int(w) - 1);
                        sum += src[yy * w + xx] * kernel[dy + 1] * kernel[dx + 1];
                    }
                p[y * w + x] = byte(sum / 16.0);
            }
    } else
        throw std::runtime_error("Unknown noise profile");
}
