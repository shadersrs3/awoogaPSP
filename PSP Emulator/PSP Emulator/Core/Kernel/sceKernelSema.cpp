#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelSema.h>
#include <Core/Kernel/sceKernelError.h>
#include <Core/Kernel/sceKernelTypes.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelSema";

SceUID sceKernelCreateSema(const char *name, SceUInt attr, int initVal, int maxVal, uint32_t optionPtr) {
    int error;
    Semaphore *s;
    if (!name) {
        LOG_ERROR(logType, "can't create semaphore if name is NULL");
        return SCE_KERNEL_ERROR_ERROR;
    }

    if (attr >= 0x200) {
        LOG_WARN(logType, "semaphore %s attribute 0x%08x is invalid", name, attr);
        return SCE_KERNEL_ERROR_ILLEGAL_ATTR;
    }

    s = createKernelObject<Semaphore>(&error);
    if (error < 0)
        return error;


    std::strncpy(s->name, name, 32);
    s->name[31] = 0;
    s->attr = attr;
    s->initCount = initVal;
    s->maxCount = maxVal;
    s->counter = initVal;
    s->waitingThreadsCount = 0;
    saveKernelObject(s);
    LOG_SYSCALL(logType, "sceKernelCreateSema(name: %s, attr: 0x%08x, initVal: %d, maxVal: %d, opt: 0x%08x) = 0x%06x", name, attr, initVal, maxVal, optionPtr, s->getUID());
    return s->getUID();
}

int sceKernelDeleteSema(int semaid) {
    int error;
    Semaphore *s = getKernelObject<Semaphore>(semaid, &error);

    if (!s) {
        LOG_ERROR(logType, "can't destroy semaphore 0x%06x while object is invalid! error 0x%08x", semaid, error);
        return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    }

    destroyKernelObject(semaid);
    LOG_SYSCALL(logType, "sceKernelDeleteSema(0x%06x)", semaid);
    return error;
}

int sceKernelPollSema(SceUID semaid, int signal) {
    if (signal <= 0) {
        LOG_WARN(logType, "can't poll semaphore if signal is invalid! semaid 0x%06x", semaid);
        return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    }

    Semaphore *s = getKernelObject<Semaphore>(semaid);
    if (!s) {
        LOG_WARN(logType, "can't poll semaid 0x%06x (unknown semaphore)", semaid);
        return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    }

    if (s->counter >= signal && s->waitingThreadsCount == 0) {
        s->counter -= signal;
    } else {
        LOG_WARN(logType, "can't poll (add) semaphore counter! semaid 0x%06x", semaid);
        return SCE_KERNEL_ERROR_SEMA_ZERO;
    }
    // LOG_SYSCALL(logType, "sceKernelPollSema(0x%06x, %d)", semaid, signal);
    return 0;
}

int sceKernelSignalSema(SceUID semaid, int signal) {
    int error;
    Semaphore *s = getKernelObject<Semaphore>(semaid, &error);

    if (error < 0) {
        LOG_ERROR(logType, "(%s) sceKernelSignalSema(0x%06x, %d) error 0x%08x", current->name, semaid, signal, error);
        return SCE_KERNEL_ERROR_UNKNOWN_SEMID;
    }

    if (s->counter + signal - s->waitingThreadsCount > s->maxCount) {
        LOG_WARN(logType, "(%s) sceKernelSignalSema overflow semaphore name %s UID 0x%06x", current->name, s->name, s->getUID());
        return SCE_KERNEL_ERROR_SEMA_OVF;
    }

    s->counter += signal;
    Core::Timing::consumeCycles(900);

    // LOG_SYSCALL(logType, "(%s) sceKernelSignalSema(0x%06x, %d) semaphore counter %d", current->name, semaid, signal, s->counter);
    return 0;
}

static uint64_t __getTimeoutData(uint32_t timeoutPtr) {
    int micro = (int) Core::Memory::read32(timeoutPtr);

    if (micro <= 3)
        micro = 24;
    else if (micro <= 249)
        micro = 245;

    return Core::Timing::usToCycles((uint64_t)micro + 1000000);
}

int __hleKernelWaitSemaphore(SceUID semaid, int signal, uint32_t timeoutPtr) {
    int error;
    Semaphore *s;

    Core::Timing::addIdleCycles(900);

    if (signal <= 0)
        return SCE_KERNEL_ERROR_ILLEGAL_COUNT;

    Core::Timing::addIdleCycles(500);

    s = getKernelObject<Semaphore>(semaid, &error);
    if (!s)
        return SCE_KERNEL_ERROR_UNKNOWN_SEMID;

    if (signal > s->maxCount) {
        LOG_WARN(logType, "signal is greater than max semaphore count in %s", __FUNCTION__);
        return SCE_KERNEL_ERROR_ILLEGAL_COUNT;
    }

    if (s->counter >= signal) {
        s->counter -= signal;
    } else {
        threadAddToWaitingList(current, semaid, WAITTYPE_SEMA);
        current->waitData[0] = signal;
        if (Core::Memory::Utility::isValidAddress(timeoutPtr)) {
            current->waitData[1] = (uint64_t)timeoutPtr;
            current->waitData[2] = Core::Timing::getCurrentCycles() + (uint64_t)__getTimeoutData(timeoutPtr);
        } else {
            current->waitData[1] = 0;
        }

        s->waitingThreadsCount++;
    }
    return 0;
}

int sceKernelWaitSema(SceUID semaid, int signal, uint32_t timeoutPtr) {
    int error = __hleKernelWaitSemaphore(semaid, signal, timeoutPtr);

    if (error < 0) {
        LOG_WARN(logType, "(%s) sceKernelWaitSema(0x%06x, %d, 0x%08x) error 0x%08x", current->name, semaid, signal, timeoutPtr, error);
    } else {
        // LOG_SYSCALL(logType, "(%s) sceKernelWaitSema(0x%06x, %d, 0x%08x)", current->name, semaid, signal, timeoutPtr);
    }
    return 0;
}

int sceKernelWaitSemaCB(SceUID semaid, int signal, uint32_t timeoutPtr) {
    int error = __hleKernelWaitSemaphore(semaid, signal, timeoutPtr);

    if (error < 0) {
        LOG_WARN(logType, "(%s) sceKernelWaitSemaCB(0x%06x, %d, 0x%08x) error 0x%08x", current->name, semaid, signal, timeoutPtr, error);
    } else {
        // LOG_SYSCALL(logType, "(%s) sceKernelWaitSemaCB(0x%06x, %d, 0x%08x)", current->name, semaid, signal, timeoutPtr);
    }
    return 0;
}

int sceKernelReferSemaStatus(SceUID semaid, uint32_t infoPtr);
}