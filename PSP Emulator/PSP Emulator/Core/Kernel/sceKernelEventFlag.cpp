#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <Core/Kernel/sceKernelEventFlag.h>
#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelTypes.h>
#include <Core/Memory/MemoryAccess.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelEventFlag";

SceUID sceKernelCreateEventFlag(const char *name, SceUInt attr, int bits, uint32_t optionPtr) {
    EventFlag *e;
    int error;

    if (!name) {
        LOG_ERROR(logType, "can't create event flag if name is NULL");
        return SCE_KERNEL_ERROR_ERROR;
    }

    e = createKernelObject<EventFlag>(&error);
    if (error < 0)
        return error;

    saveKernelObject(e);

    std::strncpy(e->name, name, 32);
    e->name[31] = 0;

    e->initPattern = e->currentPattern = bits;
    e->attr = attr;
    e->numWaitThreads = 0;
    LOG_SYSCALL(logType, "sceKernelCreateEventFlag(name: %s, attr: 0x%08x, bits: 0x%08x, opt: 0x%08x) -> evid 0x%06x", name, attr, bits, optionPtr, e->getUID());
    return e->getUID();
}

int sceKernelDeleteEventFlag(int evid) {
    int error;
    EventFlag *e = getKernelObject<EventFlag>(evid, &error);

    if (!e) {
        LOG_ERROR(logType, "can't destroy event flag 0x%06x while object is invalid! error 0x%08x", evid, error);
        return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    }

    destroyKernelObject(evid);
    LOG_SYSCALL(logType, "sceKernelDeleteEventFlag(0x%06x)", evid);
    return 0;
}

int sceKernelClearEventFlag(SceUID evid, uint32_t bits) {
    int error;
    EventFlag *e = getKernelObject<EventFlag>(evid, &error);

    if (!e) {
        LOG_ERROR(logType, "can't clear event flag bits evid 0x%06x", evid);
        return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    }

    e->currentPattern &= bits;
    Core::Timing::consumeCycles(465);
    // LOG_SYSCALL(logType, "sceKernelClearEventFlag(0x%06x, 0x%08x)", evid, bits);
    return 0;
}

int sceKernelPollEventFlag(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr) {
    auto *e = getKernelObject<EventFlag>(evid);
    bool applied = false;
    uint32_t oldValue = 0;

    if (!e) {
        LOG_ERROR(logType, "can't poll event flag evid 0x%06x", evid);
        return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    }

    oldValue = e->currentPattern;

    auto applyClearPattern = [&]() -> void {
        if (wait & PSP_EVENT_WAITCLEAR)
            e->currentPattern &= ~bits;

        if (wait & PSP_EVENT_WAITCLEARALL)
            e->currentPattern = 0;
    };

    switch (wait & 1) {
    case PSP_EVENT_WAITOR:
        if (0 != (bits & e->currentPattern)) {
            applied = true;
            applyClearPattern();
        }
        break;
    case PSP_EVENT_WAITAND:
        if ((bits & e->currentPattern) == bits) {
            applied = true;
            applyClearPattern();
        }
        break;
    }

    if (outBitsPtr != 0)
        Core::Memory::write32(outBitsPtr, oldValue);
    // LOG_SYSCALL(logType, "sceKernelPollEventFlag(evid: 0x%06x, bits: 0x%08x, wait: 0x%08x, outBits: 0x%08x) -> %s event flag %s", evid, bits, wait, outBitsPtr, current->name, e->name);
    return applied ? 0 : 0x800201AF;
}

int sceKernelReferEventFlagStatus(SceUID evid, uint32_t statusPtr) {
    EventFlag *e = getKernelObject<EventFlag>(evid);
#pragma pack(push, 1)
    struct SceKernelEventFlagInfo {
        SceSize size;
        char name[32];
        SceUInt attr;
        SceUInt initPattern;
        SceUInt currentPattern;
        int numWaitThreads;
    };
#pragma pack(pop)
    if (!e) {
        LOG_ERROR(logType, "can't refer event flag evid 0x%06x", evid);
        return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    }

    void *data = Core::Memory::getPointerUnchecked(statusPtr);
    if (data) {
        SceKernelEventFlagInfo info;

        info.size = sizeof info;
        std::memcpy(info.name, e->name, sizeof info.name);
        info.attr = e->attr;
        info.initPattern = e->initPattern;
        info.currentPattern = e->currentPattern;
        info.numWaitThreads = e->numWaitThreads;
        std::memcpy(data, &info, sizeof info);
    }
    Core::Timing::consumeCycles(1005);
    // LOG_SYSCALL(logType, "sceKernelReferEventFlagStatus(0x%06x, 0x%08x)", evid, statusPtr);
    return 0;
}

int sceKernelSetEventFlag(SceUID evid, uint32_t bits) {
    EventFlag *e = getKernelObject<EventFlag>(evid);

    if (!e) {
        LOG_ERROR(logType, "can't set event flag bits evid 0x%06x", evid);
        return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    }

    e->currentPattern |= bits;
    Core::Timing::consumeCycles(465);
    // LOG_SYSCALL(logType, "sceKernelSetEventFlag(0x%06x, 0x%08x)", evid, bits);
    return 0;
}

static int hleWaitEventFlag(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr, uint32_t timeoutPtr, bool handleCallbacks = false) {
    if (bits == 0) {
        LOG_ERROR(logType, "illegal pattern for bits event flag uid 0x%06x", evid);
        return SCE_KERNEL_ERROR_EVF_ILPAT;
    }

    if ((wait & ~PSP_EVENT_WAITKNOWN) != 0) {
        LOG_ERROR(logType, "illegal mode for wait event flag uid 0x%06x", evid);
        return SCE_KERNEL_ERROR_ILLEGAL_MODE;
    }

    auto e = getKernelObject<EventFlag>(evid);
    if (!e) {
        LOG_ERROR(logType, "can't wait event flag 0x%06x (unknown evfid)", evid);
        return SCE_KERNEL_ERROR_UNKNOWN_EVFID;
    }

    Core::Timing::consumeCycles(810);
    current->waitData[0] = bits;
    current->waitData[1] = wait;
    current->waitData[2] = outBitsPtr;
    if (auto ptr = (uint32_t *)Core::Memory::getPointerUnchecked(timeoutPtr); ptr != nullptr)
        current->waitData[3] = *ptr;
    else
        current->waitData[3] = ~0u;

    e->numWaitThreads++;
    if (handleCallbacks)
        hleCurrentThreadEnableCallbackState();
    threadAddToWaitingList(current, evid, WAITTYPE_EVENTFLAG);
    return 0;
}

int sceKernelWaitEventFlag(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr, uint32_t timeoutPtr) {
    int error = hleWaitEventFlag(evid, bits, wait, outBitsPtr, timeoutPtr);
    // LOG_SYSCALL(logType, "sceKernelWaitEventFlag(evid: 0x%06x, bits: 0x%08x, wait: 0x%08x, outBits: 0x%08x, timeout: 0x%08x) -> %s error 0x%08x", evid, bits, wait, outBitsPtr, timeoutPtr, current->name, error);
    return error;
}

int sceKernelWaitEventFlagCB(SceUID evid, uint32_t bits, uint32_t wait, uint32_t outBitsPtr, uint32_t timeoutPtr) {
    int error = hleWaitEventFlag(evid, bits, wait, outBitsPtr, timeoutPtr);
    hleCurrentThreadEnableCallbackState();
    // LOG_SYSCALL(logType, "sceKernelWaitEventFlagCB(evid: 0x%06x, bits: 0x%08x, wait: 0x%08x, outBits: 0x%08x, timeout: 0x%08x) -> %s error 0x%08x", evid, bits, wait, outBitsPtr, timeoutPtr, current->name, error);
    return error;
}
}