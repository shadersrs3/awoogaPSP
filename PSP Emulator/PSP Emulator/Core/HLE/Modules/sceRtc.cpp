#include <Core/PSP/Types.h>
#include <Core/HLE/Modules/sceRtc.h>
#include <Core/HLE/RTC.h>
#include <Core/Memory/MemoryAccess.h>
#include <Core/Timing.h>

#include <Core/Logger.h>

#include <ctime>

namespace Core::HLE {
static const char *logType = "sceRtc";

static uint64_t getRtcTicks() {
    return __rtc_getBaseTicks() + Core::Timing::getSystemTimeMicroseconds();
}

int sceRtcGetCurrentTick(uint32_t tickPtr) {
    uint64_t t = getRtcTicks();
    if (uint64_t *p = (uint64_t *) Core::Memory::getPointerUnchecked(tickPtr); p)
        *p = t;
    Core::Timing::consumeCycles(400);
    LOG_SYSCALL(logType, "sceRtcGetCurrentTick(0x%08x) = %lld", tickPtr, t);
    return 0;
}

int sceRtcGetTickResolution() {
    return 1000000;
}

int sceRtcGetCurrentClockLocalTime(uint32_t dateTimePtr) {
    ScePspDateTime *t, x;
    uint64_t ticks;
    t = (ScePspDateTime *) Core::Memory::getPointerUnchecked(dateTimePtr);
    if (!t) {
        LOG_WARN(logType, "sceRtcGetCurrentClockLocalTime pointer invalid!");
        return 0;
    }

    t = &x;
    ticks = getRtcTicks();
    uint32_t microsecond = ticks % 1000000;
    ticks /= 1000000UL;

    static char str[26];

    tm tm;
    localtime_s(&tm, (time_t *) &ticks);
    char buf[26];
    
    strftime(buf, 26, "%Y", &tm);
    t->year = atoi(buf);
    strftime(buf, 26, "%m", &tm);
    t->month = atoi(buf);
    strftime(buf, 26, "%d", &tm);
    t->day = atoi(buf);
    strftime(buf, 26, "%H", &tm);
    t->hour = atoi(buf);
    strftime(buf, 26, "%M", &tm);
    t->minute = atoi(buf);
    strftime(buf, 26, "%S", &tm);
    t->second = atoi(buf);
    t->microsecond = microsecond;
    LOG_SYSCALL(logType, "sceRtcGetCurrentClockLocalTime(0x%08x)", dateTimePtr);
    return 0;
}
}