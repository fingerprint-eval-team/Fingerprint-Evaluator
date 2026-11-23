#pragma once
#include <windows.h>
using Extract = int (*)(int, unsigned char*, unsigned char*, int*, int*, char*);
using Compare = int (*)(unsigned char*, unsigned char*, double*, int*, char*);
constexpr int capacity = 1000000;
inline bool dimensions(int type, UINT& w, UINT& h) {
    switch (type) {
    case 1:
        w = 448;
        h = 478;
        return true;
    case 2:
        w = 300;
        h = 300;
        return true;
    case 3:
        w = 640;
        h = 480;
        return true;
    // K-NBTC guide table 3-3; project types 1/2/3 remain unchanged.
    case 5:
        w = 280;
        h = 320;
        return true;
    case 7:
        w = 292;
        h = 525;
        return true;
    case 8:
        w = 248;
        h = 292;
        return true;
    case 9:
        w = 288;
        h = 320;
        return true;
    default:
        return false;
    }
}
inline bool supportedType(int type) {
    UINT w = 0, h = 0;
    return dimensions(type, w, h);
}
