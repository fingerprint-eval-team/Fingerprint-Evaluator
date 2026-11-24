#pragma once
#include <windows.h>

constexpr int capacity = 1000000;
using Extract = int(__cdecl*)(int, unsigned char*, unsigned char*, int*, int*, char*);
using Compare = int(__cdecl*)(unsigned char*, unsigned char*, double*, int*, char*);

enum ErrorCode {
    Success = 0,
    NullArgument = 100,
    UnsupportedType = 101,
    InvalidImage = 102,
    InvalidOutput = 103,
    DependencyLoad = 110,
    DependencySymbol = 111,
    ExtractorFailure = 120,
    TooFewMinutiae = 121,
    InvalidExtractorData = 122,
    InvalidTemplate = 200,
    UnsupportedTemplate = 201,
    TemplateChecksum = 202,
    OutOfMemory = 900,
    StandardException = 901,
    UnknownException = 999
};

inline bool dimensions(int type, unsigned& width, unsigned& height) {
    // Project iTypes 1/2/3 all contain 8-bit, 500 ppi FVC pixels.
    switch (type) {
    case 1:
        width = 448;
        height = 478;
        return true;
    case 2:
        width = 300;
        height = 300;
        return true;
    case 3:
        width = 640;
        height = 480;
        return true;
    default:
        return false;
    }
}
