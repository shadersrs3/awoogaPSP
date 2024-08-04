#include <Core/HLE/RTC.h>
#include <Core/Timing.h>

#include <Core/Logger.h>

#include <ctime>

#if defined(_WIN32)
#include <Windows.h>
#endif

namespace Core::HLE {
static const char *logType = "RTC";

static SceKernelTimeval rtcBase;
static uint64_t rtcTicks;

// Grabbed from JPSCP
// This is the # of microseconds between January 1, 0001 and January 1, 1970.
const uint64_t rtcMagicOffset = 62135596800000000ULL;
// This is the # of microseconds between January 1, 0001 and January 1, 1601 (for Win32 FILETIME.)
const uint64_t rtcFiletimeOffset = 50491123200000000ULL;

#if defined (_WIN32)
#define FILETIME_FROM_UNIX_EPOCH_US (rtcMagicOffset - rtcFiletimeOffset)
void gettimeofday(SceKernelTimeval *tv, timezone *ignore)
{
    FILETIME ft_utc, ft_local;
    GetSystemTimeAsFileTime(&ft_utc);
    ft_local = ft_utc;

    uint64_t from_1601_us = (((uint64_t) ft_local.dwHighDateTime << 32ULL) + (uint64_t) ft_local.dwLowDateTime) / 10ULL;
    uint64_t from_1970_us = from_1601_us - FILETIME_FROM_UNIX_EPOCH_US;

    tv->tv_sec = long(from_1970_us / 1000000UL);
    tv->tv_usec = from_1970_us % 1000000UL;
}
#endif

time_t start_time;

void rtcInit(bool update) {
    gettimeofday(&rtcBase, nullptr);

    time(&start_time);
    rtcTicks = (uint64_t)rtcBase.tv_sec * 1000000UL + (uint64_t)rtcBase.tv_usec;
    LOG_INFO(logType, "%s RTC seconds %d microseconds %d", update ? "updated" : "initialized", rtcBase.tv_sec, rtcBase.tv_usec);
}

void __rtc_updateTicks() {
    rtcInit(true);
}

uint64_t __rtc_getBaseTicks() {
    return rtcTicks;
}

void __rtc_time(uint32_t *time) {
    *time = (uint32_t)start_time + (uint32_t)(Core::Timing::getCurrentCycles() / Core::Timing::getClockFrequency());
}

void __rtc_gettimeofday(SceKernelTimeval *tv, timezone *timezone) {
    *tv = rtcBase;
    int64_t adjusted = tv->tv_usec + Core::Timing::getSystemTimeMicroseconds();
    tv->tv_sec += long(adjusted / 1000000UL);
    tv->tv_usec += adjusted % 1000000UL;
}
}