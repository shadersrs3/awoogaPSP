#pragma once

#include <cstdint>
#include <Windows.h>

// Use this if you know the value is non-zero.
inline uint32_t clz32_nonzero(uint32_t value) {
    DWORD index;
    BitScanReverse(&index, value);
    return 31 ^ (uint32_t)index;
}

inline uint32_t clz32(uint32_t value) {
    if (!value)
        return 32;
    DWORD index;
    BitScanReverse(&index, value);
    return 31 ^ (uint32_t)index;
}
