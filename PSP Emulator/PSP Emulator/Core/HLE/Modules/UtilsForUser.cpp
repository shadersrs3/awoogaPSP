#include <Core/HLE/RTC.h>
#include <Core/HLE/Modules/UtilsForUser.h>
#include <Core/Timing.h>
#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

#include <Windows.h>

namespace Core::HLE {
static const char *logType = "UtilsForUser";

uint32_t sceKernelLibcGettimeofday(uint32_t tpPtr, uint32_t tzpPtr) {
    if (SceKernelTimeval *ptr = (SceKernelTimeval *) Core::Memory::getPointerUnchecked(tpPtr); ptr != nullptr) {
        __rtc_gettimeofday((SceKernelTimeval *) ptr, nullptr);
    }

    Core::Timing::consumeCycles(1885);
    // LOG_SYSCALL(logType, "sceKernelLibcGettimeofday(timevalAddress: 0x%08x, timezoneAddress: 0x%08x) tv_sec %d tv_usec %d", tpPtr, tzpPtr, ptr->tv_sec, ptr->tv_usec);
    return 0;
}

uint32_t sceKernelLibcTime(uint32_t timePtr) {
    uint32_t *outTime = (uint32_t *) Core::Memory::getPointerUnchecked(timePtr);

    uint32_t _time = 0;

    if (outTime) {
        __rtc_time(outTime);
        _time = *outTime;
    }

    Core::Timing::consumeCycles(3885);
    // LOG_SYSCALL(logType, "sceKernelLibcTime(timePtr: 0x%08x) time = 0x%08x", timePtr, _time);
    return 0;
}

int sceKernelDcacheWritebackAll() {
    // LOG_WARN(logType, "unimplemented sceKernelDcacheWritebackAll");
    return 0;
}

int sceKernelDcacheWritebackRange(uint32_t ptr, uint32_t size) {
    // LOG_WARN(logType, "unimplemented sceKernelDcacheWritebackRange(0x%08x, 0x%08x)", ptr, size);
    return 0;
}
}