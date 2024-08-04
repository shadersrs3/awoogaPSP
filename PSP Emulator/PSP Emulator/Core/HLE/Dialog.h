#pragma once

#include <cstdint>

namespace Core::HLE {
#pragma pack(push, 1)
struct SceDialogCommon {
    uint32_t size;
    int language;
    int buttonSwap;
    int graphicsThread;
    int accessThread;
    int fontThread;
    int soundThread;
    int result;
    int reserved[4];
};
#pragma pack(pop)
}