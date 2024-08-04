#pragma once

#include <cstdint>

namespace Core::HLE {
struct SceKernelTimeval {
    uint32_t tv_sec;
    uint32_t tv_usec;
};

struct timezone;

void rtcInit(bool update = false);
void __rtc_updateTicks();
uint64_t __rtc_getBaseTicks();
void __rtc_time(uint32_t *time);
void __rtc_gettimeofday(SceKernelTimeval *tv, timezone *timezone);
}