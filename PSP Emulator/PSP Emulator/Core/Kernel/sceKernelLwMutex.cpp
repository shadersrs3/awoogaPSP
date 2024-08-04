#include <Core/Kernel/sceKernelLwMutex.h>
#include <Core/Kernel/sceKernelThread.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelLwMutex";

int sceKernelCreateLwMutex(uint32_t workareaPtr, const char *name, uint32_t attr, int initialCount, uint32_t optPtr) {
    LOG_WARN(logType, "unimplemented sceKernelCreateLwMutex(workarea: 0x%08x, name: %s, attr: 0x%08x, initialCount: %d, option: 0x%08x)", workareaPtr, name, attr, initialCount, optPtr);
    return 0;
}

int sceKernelLockLwMutex(uint32_t workareaPtr, int lockCount, uint32_t timeoutPtr) {
    LOG_WARN(logType, "unimplemented sceKernelLockLwMutex(workarea: 0x%08x, lockCount: %d, timeoutPtr: 0x%08x)", workareaPtr, lockCount, timeoutPtr);
    return 0;
}

int sceKernelTryLockLwMutex(uint32_t workareaPtr, int lockCount) {
    LOG_WARN(logType, "unimplemented sceKernelTryLockLwMutex(workarea: 0x%08x, lockCount: %d)", workareaPtr, lockCount);
    return 0;
}

int sceKernelUnlockLwMutex(uint32_t workareaPtr) {
    LOG_WARN(logType, "unimplemented sceKernelUnlockLwMutex(workarea: 0x%08x)", workareaPtr);
    return 0;
}

int sceKernelDeleteLwMutex(uint32_t workareaPtr) {
    LOG_WARN(logType, "unimplemented sceKernelDeleteLwMutex(workarea: 0x%08x)", workareaPtr);
    return 0;
}
}