#pragma once

#include <cstdint>

namespace Core::HLE {
int sceRtcGetCurrentTick(uint32_t tickPtr);
int sceRtcGetTickResolution();
int sceRtcGetCurrentClockLocalTime(uint32_t dateTimePtr);
}