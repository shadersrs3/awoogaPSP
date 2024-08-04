#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>
#include <deque>

#include <cassert>

#include <Core/Loaders/AbstractLoader.h>

#include <Core/Allegrex/CPURegisterName.h>

#include <Core/Memory/MemoryAccess.h>
#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelSema.h>
#include <Core/Kernel/sceKernelEventFlag.h>
#include <Core/Kernel/sceKernelTypes.h>
#include <Core/Kernel/sceKernelCallback.h>
#include <Core/Kernel/sceKernelSystemMemory.h>
#include <Core/Kernel/sceKernelInterrupt.h>
#include <Core/Kernel/sceKernelAlarm.h>

#include <Core/Allegrex/Allegrex.h>
#include <Core/Allegrex/AllegrexState.h>
#include <Core/Allegrex/AllegrexSyscallHandler.h>

#include <Core/Timing.h>

#include <Core/HLE/CustomSyscall.h>
#include <Core/HLE/CPUAssembler.h>
#include <Core/HLE/Modules/scePower.h>
#include <Core/HLE/Modules/sceUmd.h>
#include <Core/HLE/Modules/IoFileMgrForUser.h>

#include <Core/GPU/DisplayList.h>
#include <Core/GPU/GPU.h>

#include <Core/Logger.h>

#include <mutex>

using namespace Core::Allegrex;
using namespace Core::HLE;

namespace Core::Kernel {
static const char *logType = "sceKernelThread";

static PSPThread *rootThread, *idle[2];

static const int MAX_RUNNING_THREADS = 128;

// TODO: add a sanity check for only ready threads even if it's (deleted)
std::vector<int> startedThreadList;

static std::deque<PSPThread *> threadReadyQueue;
static std::vector<SceUID> threadWaitingList;
static std::vector<SceUID> threadExitList;
// static std::vector<SceUID> thread

PSPThread *current = nullptr;

static void doWakeup(SceUID thid);

static void kernelClearThreadState() {
    threadReadyQueue.clear();
    threadWaitingList.clear();
    threadExitList.clear();
    startedThreadList.clear();
}

static bool threadReadyQueueIsEmpty() {
    return threadReadyQueue.size() == 0;
}

static void threadReadyQueueAddFromThreadList() {
    if (!threadReadyQueueIsEmpty())
        return;

    for (auto& i : startedThreadList) {
        PSPThread *thr = getKernelObject<PSPThread>(i);

        if (!thr)
            continue;

        if (thr->status == THREAD_STATUS_READY)
            threadReadyQueue.push_back(thr);
    }

    std::sort(threadReadyQueue.begin(), threadReadyQueue.end(), [](PSPThread *a, PSPThread *b) { return a->currentPriority < b->currentPriority; });
}

static PSPThread *threadReadyQueueGet() {
    PSPThread *thread;
    threadReadyQueueAddFromThreadList();

    if (threadReadyQueue.size() == 0) {
        LOG_ERROR(logType, "thread ready queue is empty!");
        return nullptr;
    }

    do {
        thread = threadReadyQueue[0];
        threadReadyQueue.pop_front();

        if (!thread)
            continue;

        if (thread->status == THREAD_STATUS_READY)
            break;
    } while (true);
    return thread;
}

static void threadReadyQueueRotate(int priority);

static void hleThreadExit(PSPThread *thr, int status) {
    thr->exitStatus = status;
    thr->status = THREAD_STATUS_DORMANT;
}

static void hleThreadExitAndDelete(PSPThread *thr, int status) {
    hleThreadExit(thr, status);
    threadExitList.push_back(thr->getUID());
}

static void hleThreadDelete(PSPThread *thread) {
    threadExitList.push_back(thread->getUID());
}

void threadAddToWaitingList(PSPThread *thread, int waitId, int waitType) {
    if (auto it = std::find(threadWaitingList.begin(), threadWaitingList.end(), thread->getUID()); it == threadWaitingList.end()) {
        thread->status = THREAD_STATUS_WAITING;
        thread->waitId = waitId;
        thread->waitType = waitType;
        threadWaitingList.push_back(thread->getUID());
    } else {
        LOG_ERROR(logType, "can't add thread to waiting list again (got thread %s UID 0x%06x)", thread->name, thread->getUID());
    }
}

std::vector<int> *getThreadWaitingList() {
    return &threadWaitingList;
}

enum {
    PSP_THREAD_ATTR_KERNEL       = 0x00001000,
    PSP_THREAD_ATTR_VFPU         = 0x00004000,
    PSP_THREAD_ATTR_SCRATCH_SRAM = 0x00008000, // Save/restore scratch as part of context???
    PSP_THREAD_ATTR_NO_FILLSTACK = 0x00100000, // No filling of 0xff.
    PSP_THREAD_ATTR_CLEAR_STACK  = 0x00200000, // Clear thread stack when deleted.
    PSP_THREAD_ATTR_LOW_STACK    = 0x00400000, // Allocate stack from bottom not top.
    PSP_THREAD_ATTR_USER         = 0x80000000,
    PSP_THREAD_ATTR_USBWLAN      = 0xa0000000,
    PSP_THREAD_ATTR_VSH          = 0xc0000000,
};

// creation, start, etc.
SceUID sceKernelCreateThread(const char *name, uint32_t entry, int initPriority, int stackSize, SceUInt attr, uint32_t threadOptParamPtr, int modid) {
    int error;
    PSPThread *thread;

    if (!name) {
        LOG_WARN(logType, "thread name is NULL");
        return SCE_KERNEL_ERROR_ERROR;
    }

    if (!Core::Memory::Utility::isValidAddress(entry)) {
        LOG_WARN(logType, "invalid thread entry address");
        return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    }

    if (stackSize < 0x100) {
        LOG_WARN(logType, "stack size should be >= 256 received 0x%x", stackSize);
        return SCE_KERNEL_ERROR_ILLEGAL_STACK_SIZE;
    }

    if (initPriority < 0x08 || initPriority > 0x77) {
        LOG_WARN(logType, "invalid thread priority %d thread name %s", initPriority, name);
        return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
    }

    if (attr & PSP_THREAD_ATTR_KERNEL)
        LOG_WARN(logType, "creating a kernel thread");

    thread = createKernelObject<PSPThread>(&error);
    if (!thread) {
        LOG_WARN(logType, "failed to create thread");
        return error;
    }

    thread->resetContext();

    strncpy_s(thread->name, name, 31);
    thread->name[31] = '\0';

    thread->status = THREAD_STATUS_DORMANT;
    thread->entryPoint = entry;
    thread->initialPriority = thread->currentPriority = initPriority;
    thread->attr = attr;
    thread->stackPointer = getMemoryAllocatorFromID(2)->allocHigh(stackSize);
    thread->stackSize = stackSize;

    thread->waitType = 0;
    thread->waitId = 0;
    thread->clockCyclesRan = 0;
    thread->wakeupCount = 0;
    thread->exitStatus = 0;
    thread->clockCyclesRan = 0;
    thread->interruptPreemptCount = 0;
    thread->threadPreemptCount = 0;
    thread->releaseCount = 0;
    thread->gpReg = 0;
    thread->moduleId = modid;
    thread->threadState = PSP_THREAD_STATE_NORMAL;

    if (modid != -1) {
        PSPModule *mod = getKernelObject<PSPModule>(modid);
        if (mod) {
            thread->gpReg = mod->gp;
            thread->moduleId = mod->getUID();
        }
    }

    saveKernelObject(thread);
    LOG_SYSCALL(logType, "0x%08x = sceKernelCreateThread(name: %s, entry: 0x%08x, init prio: 0x%08x, stack size: 0x%08x, attr: 0x%08x, opt: 0x%08x) stack pointer 0x%08x", thread->getUID(), name, entry, initPriority, stackSize, attr, threadOptParamPtr, thread->stackPointer);
    return thread->getUID();
}

int sceKernelStartThread(SceUID thid, uint32_t arglen, uint32_t argPtr) {
    int error;
    PSPThread *thread = getKernelObject<PSPThread>(thid, &error);

    if (!thread) {
        LOG_WARN(logType, "can't start thread while thread is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    if (argPtr != 0 && !Core::Memory::Utility::isValidAddress(argPtr)) {
        LOG_WARN(logType, "invalid memory address");
        return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    }

    if (argPtr != 0 && !Core::Memory::Utility::isValidAddressRange(argPtr, argPtr + arglen)) {
        LOG_WARN(logType, "invalid memory address");
        return SCE_KERNEL_ERROR_ILLEGAL_ADDR;
    }

    if (thread->status != THREAD_STATUS_DORMANT) {
        LOG_WARN(logType, "thread %s is already in ready state", thread->name);
        // return SCE_KERNEL_ERROR_NOT_DORMANT;
    }

    thread->status = THREAD_STATUS_READY;

    if ((thread->attr & PSP_THREAD_ATTR_NO_FILLSTACK) == 0)
        Core::Memory::Utility::memorySet(thread->stackPointer - thread->stackSize, 0xFF, thread->stackSize);

    thread->context.pc = thread->entryPoint;
    thread->context.gpr[MIPS_REG_GP] = thread->gpReg;

    uint32_t& sp = thread->context.gpr[MIPS_REG_SP];
    sp = thread->stackPointer;

    if (argPtr != 0 && arglen > 0) {
        sp = (sp - 0xF) & ~0xF;
        thread->context.gpr[MIPS_REG_A0] = arglen;
        thread->context.gpr[MIPS_REG_A1] = sp;

        Core::Memory::Utility::copyMemoryGuestToGuest(sp, argPtr, arglen);
        // Core::Memory::write8(sp + arglen + 1, 0);
    } else {
        thread->context.gpr[MIPS_REG_A0] = 0;
        thread->context.gpr[MIPS_REG_A1] = 0;
    }

    sp -= 64;

    Core::Memory::write32(sp, Assembly::makeJumpReturnAddress());
    Core::Memory::write32(sp + 0x4, 0x40C);
    Core::Memory::write32(sp + 0x8, Assembly::makeBranch(0xFFFF));
    Core::Memory::write32(sp + 0xC, Assembly::makeNop());
    Core::Allegrex::setSyscallAddressHandler(sp + 0x4, CUSTOM_NID_RETURN_FROM_THREAD);

    thread->context.gpr[MIPS_REG_K0] = thread->stackPointer + 0x20;
    thread->context.gpr[MIPS_REG_RA] = hleGetReturnFromAddress(THREAD_RETURN_ADDRESS_RETURN_FROM_THREAD);
    thread->context.gpr[MIPS_REG_FP] = sp;

    startedThreadList.push_back(thid);
    LOG_SYSCALL(logType, "sceKernelStartThread(thid: 0x%06x, arglen: 0x%08x, argPtr: 0x%08x) -> thread %s", thid, arglen, argPtr, thread->name);
    return error;
}

int sceKernelDeleteThread(SceUID thid) {
    int error = 0;
    auto thr = getKernelObject<PSPThread>(thid, &error);

    if (!thr) {
        LOG_WARN(logType, "sceKernelDeleteThread: can't find thid 0x%06x", thid);
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    if (0 == thid || current->getUID() == thid) {
        LOG_WARN(logType, "attempting to delete current thread!");
        return SCE_KERNEL_ERROR_NOT_DORMANT;
    }

    startedThreadList.erase(std::find(startedThreadList.begin(), startedThreadList.end(), thr->getUID()));
    destroyKernelObject(thid);
    LOG_SYSCALL(logType, "sceKernelDeleteThread(0x%06x)", thid);
    return 0;
}

int sceKernelExitDeleteThread(int status) {
    hleThreadExitAndDelete(current, status);
    LOG_SYSCALL(logType, "sceKernelExitDeleteThread(0x%08x) thread name %s", status, current->name);
    return 0;
}

int sceKernelExitThread(int status) {
    hleThreadExit(current, status);
    LOG_SYSCALL(logType, "sceKernelExitThread(0x%08x) thread name %s", status, current->name);
    return 0;
}

int sceKernelTerminateDeleteThread(SceUID thid) {
    PSPThread *thr;

    if (0 == thid || current->getUID() == thid) {
        LOG_WARN(logType, "attempting to delete current thread!");
        return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }

    thr = getKernelObject<PSPThread>(thid);
    if (!thr) {
        LOG_WARN(logType, "can't terminate delete thread if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    LOG_SYSCALL(logType, "sceKernelTerminateDeleteThread(0x%06x) -> thread %s", thr->getUID(), thr->name);
    hleThreadExitAndDelete(thr, 0);
    return 0;
}

int sceKernelTerminateThread(SceUID thid) {
    PSPThread *thr;

    if (0 == thid || current->getUID() == thid) {
        LOG_WARN(logType, "attempting to delete current thread!");
        return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }

    thr = getKernelObject<PSPThread>(thid);
    if (!thr) {
        LOG_WARN(logType, "can't terminate thread if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    thr->currentPriority = thr->initialPriority;
    hleThreadExit(thr, 0);
    LOG_SYSCALL(logType, "sceKernelTerminateThread(0x%06x) -> thread %s", thid, thr->name);
    return 0;
}

// waits
int sceKernelWaitThreadEnd(SceUID thid, uint32_t timeoutPtr) {
    if (0 == thid || current->getUID() == thid) {
        LOG_WARN(logType, "attempting to wait thread end from current thread!");
        return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }

    PSPThread *thr = getKernelObject<PSPThread>(thid);
    if (!thr) {
        LOG_WARN(logType, "can't wait thread end if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    uint32_t *data = (uint32_t *)Core::Memory::getPointerUnchecked(timeoutPtr);

    current->waitData[0] = timeoutPtr;
    threadAddToWaitingList(current, thr->getUID(), WAITTYPE_THREADEND);
    LOG_SYSCALL(logType, "sceKernelWaitThreadEnd(0x%06x, 0x%08x) -> %s waited til %s finished", thid, timeoutPtr, current->name, thr->name);
    return 0;
}

int sceKernelWaitThreadEndCB(SceUID thid, uint32_t timeoutPtr) {
    return 0;
}

// wakeups

static void __hleSleepThread(bool handleCallbacks = false) {
    if (handleCallbacks)
        hleCurrentThreadEnableCallbackState();
    threadAddToWaitingList(current, 0, WAITTYPE_SLEEP);
}

int sceKernelSleepThread() {
    __hleSleepThread();
    // LOG_SYSCALL(logType, "sceKernelSleepThread() -> thread %s", current->name);
    return 0;
}

int sceKernelSleepThreadCB() {
    __hleSleepThread(true);
    // LOG_SYSCALL(logType, "sceKernelSleepThreadCB() -> thread %s", current->name);
    return 0;
}

int sceKernelWakeupThread(SceUID thid) {
    PSPThread *t = getKernelObject<PSPThread>(thid);
    if (!t) {
        LOG_WARN(logType, "can't wakeup thread if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    doWakeup(thid);
    // LOG_SYSCALL(logType, "sceKernelWakeupThread(0x%06x) -> %s", thid, t->name);
    return 0;
}

int sceKernelCancelWakeupThread(SceUID thid) {
    int _thid = thid;
    if (_thid == 0)
        _thid = current->getUID();

    PSPThread *t = getKernelObject<PSPThread>(_thid);
    if (!t) {
        LOG_WARN(logType, "can't cancel wakeup thread if thid is unknown got 0x%06x", thid);
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    doWakeup(_thid);
    LOG_WARN(logType, "unimplemented sceKernelCancelWakeupThread(0x%06x) -> %s", thid, t->name);
    return 0;
}

// attr, priority
int sceKernelChangeCurrentThreadAttr(SceUInt clearAttr, SceUInt setAttr) {
    LOG_WARN(logType, "unimplemented sceKernelChangeCurrentThreadAttr(0x%08x, 0x%08x)", clearAttr, setAttr);
    current->attr = (current->attr & ~clearAttr) | setAttr;
    return 0;
}

int sceKernelChangeThreadPriority(SceUID thid, int priority) {
    int error;
    PSPThread *thr = getKernelObject<PSPThread>(thid, &error);

    if (!thr) {
        LOG_WARN(logType, "can't change thread priority if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    if (priority < 0x08 || priority > 0x77) {
        LOG_WARN(logType, "changing thread priority is invalid! got %d thread name %s", priority, thr->name);
        return SCE_KERNEL_ERROR_ILLEGAL_PRIORITY;
    }

    thr->currentPriority = priority;
    LOG_SYSCALL(logType, "sceKernelChangeThreadPriority(0x%06x, %d) %s", thid, priority, thr->name);
    return 0;
}

int sceKernelRotateThreadReadyQueue(int priority) {
    LOG_WARN(logType, "unimplemented sceKernelRotateThreadReadyQueue(%d)", priority);
    return 0;
}

// gets
int sceKernelGetThreadCurrentPriority() {
    Core::Timing::consumeCycles(155);
    LOG_SYSCALL(logType, "sceKernelGetThreadCurrentPriority() = %d", current->currentPriority);
    return current->currentPriority;
}

int sceKernelGetThreadExitStatus(SceUID thid) {
    PSPThread *thr = getKernelObject<PSPThread>(thid);

    if (!thr) {
        LOG_WARN(logType, "can't get exit status if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }
    return thr->exitStatus;
}

int sceKernelGetThreadId() {
    Core::Timing::consumeCycles(365);
    // LOG_SYSCALL(logType, "sceKernelGetThreadId() = 0x%06x (%s)", current->getUID(), current->name);
    return current->getUID();
}

int sceKernelGetThreadmanIdList(int type, uint32_t readBufPtr, int readBufSize, uint32_t idPtr);
int sceKernelGetThreadmanIdType(SceUID uid);
int sceKernelGetThreadStackFreeSize(SceUID thid) {
    SceUID threadID = thid == 0 ? current->getUID() : thid;
    PSPThread *t = getKernelObject<PSPThread>(threadID);
    int freeSize = 0;
    if (!t)
        return SCE_KERNEL_ERROR_UNKNOWN_THID;

    if (thid == 0)
        freeSize = t->stackPointer - cpu.reg[MIPS_REG_SP];
    else
        freeSize = t->stackPointer - t->context.gpr[MIPS_REG_SP];

    LOG_SYSCALL(logType, "sceKernelGetThreadStackFreeSize(0x%06x) = %s UID 0x%06x", thid, t->name, t->getUID());
    return freeSize;
}

// unknown
int sceKernelCheckCallback();
int sceKernelCheckThreadStack();

// suspends, resumes
int sceKernelSuspendDispatchThread() {
    // LOG_WARN(logType, "unimplemented sceKernelSuspendDispatchThread");
    return 0;
}

int sceKernelResumeDispatchThread() {
    // LOG_WARN(logType, "unimplemented sceKernelResumeDispatchThread");
    return 0;
}

int sceKernelSuspendThread(SceUID thid) {
    if (0 == thid || current->getUID() == thid) {
        LOG_WARN(logType, "attempting to suspend current thread!");
        return SCE_KERNEL_ERROR_ILLEGAL_THID;
    }

    auto thr = getKernelObject<PSPThread>(thid);
    if (!thr) {
        LOG_WARN(logType, "sceKernelSuspendThread: can't find thid 0x%06x", thid);
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

    thr->status = THREAD_STATUS_SUSPEND;
    LOG_SYSCALL(logType, "sceKernelSuspendThread(0x%06x)", thid);
    return 0;
}

int sceKernelResumeThread(SceUID thid);

// delay
static uint64_t getAdjustedMicroseconds(uint64_t us) {
    if (us < 200)
        return 210;

    if (us > 0x8000000000000000ULL)
        us -= 0x8000000000000000ULL;
    else if (us > 0x0010000000000000ULL)
        us >>= 12;
    return us + 15;
}

static void __hleDelayCurrentThread(uint64_t us, bool handleCallbacks = false) {
    if (handleCallbacks)
        hleCurrentThreadEnableCallbackState();

    uint64_t cycles = Timing::usToCycles(getAdjustedMicroseconds(us));
    current->waitData[0] = Core::Timing::getCurrentCycles() + cycles;
    threadAddToWaitingList(current, 0, WAITTYPE_DELAY);
}

int sceKernelDelayThread(uint32_t us) {
    __hleDelayCurrentThread((uint32_t)us);
    // LOG_SYSCALL(logType, "sceKernelDelayThread(0x%08x:%d) thread %s", us, us, current->name);
    return 0;
}

int sceKernelDelayThreadCB(uint32_t us) {
    __hleDelayCurrentThread((uint32_t)us, true);
    // LOG_WARN(logType, "unimplemented sceKernelDelayThreadCB(0x%08x:%d) thread %s", us, us, current->name);
    return 0;
}

int sceKernelDelaySysClockThread(uint32_t sysclockPtr) {
    if (auto cycles = (uint64_t *) Memory::getPointerUnchecked(sysclockPtr); cycles) {
        __hleDelayCurrentThread(*cycles);
    }

    // LOG_SYSCALL(logType, "sceKernelDelaySysClockThread(0x%08x) thread -> %s (0x%06x)", sysclockPtr, current->name, current->getUID());
    return 0;
}

int sceKernelDelaySysClockThreadCB(uint32_t sysclockPtr) {
    return 0;
}

// referring
int sceKernelReferThreadEventHandlerStatus(SceUID uid, uint32_t infoPtr);

uint32_t sceKernelReferThreadProfiler() {
    LOG_WARN(logType, "unimplemented sceKernelReferThreadProfiler");
    return 0;
}

uint32_t sceKernelReferGlobalProfiler() {
    LOG_WARN(logType, "unimplemented sceKernelReferGlobalProfiler");
    return 0;
}

int sceKernelReferThreadRunStatus(SceUID thid, uint32_t runStatusPtr);

int sceKernelReferThreadStatus(SceUID thid, uint32_t threadInfoPtr) {
    int error;
    PSPThread *thr;
    if (threadInfoPtr == 0)
        return -1;

    if (thid == 0)
        thid = current->getUID();

    thr = getKernelObject<PSPThread>(thid, &error);
    if (!thr) {
        LOG_WARN(logType, "can't refer thread status if thid is unknown");
        return SCE_KERNEL_ERROR_UNKNOWN_THID;
    }

#pragma pack(push, 1)
    struct {
        SceSize size;
        char name[32];
        SceUInt attr;
        int status;
        uint32_t entryPoint;
        uint32_t stackPointer;
        int stackSize;
        uint32_t gp;
        int initPriority;
        int currentPriority;
        int waitType;
        SceUID waitId;
        int wakeupCount;
        int exitStatus;
        uint64_t runClocks;
        SceUInt interruptPreemptCount;
        SceUInt threadPreemptCount;
        SceUInt releaseCount;
    } info;
#pragma pack(pop)

    info.size = sizeof info;
    std::memcpy(info.name, thr->name, 32);
    info.attr = thr->attr;
    info.status = thr->status;
    info.entryPoint = thr->entryPoint;
    info.stackPointer = thr->stackPointer;
    info.stackSize = thr->stackSize;
    info.gp = thr->gpReg;
    info.initPriority = thr->initialPriority;
    info.currentPriority = thr->currentPriority;
    info.waitType = thr->waitType;
    info.waitId = thr->waitId;
    info.wakeupCount = thr->wakeupCount;
    info.exitStatus = thr->exitStatus;
    info.runClocks = thr->clockCyclesRan;
    info.interruptPreemptCount = thr->interruptPreemptCount;
    info.threadPreemptCount = thr->threadPreemptCount;
    info.releaseCount = thr->releaseCount;

    Core::Timing::consumeCycles(3225);

    Memory::Utility::copyMemoryHostToGuest(threadInfoPtr, &info, sizeof info);
    // LOG_SYSCALL(logType, "sceKernelReferThreadStatus(0x%06x, 0x%08x) thread name %s", thid, threadInfoPtr, thr->name);
    return 0;
}

// system time / conversion

int sceKernelSysClock2USec(uint32_t sysClockPtr, uint32_t lowPtr, uint32_t highPtr) {
    uint32_t *lowValue = (uint32_t *) Core::Memory::getPointer(lowPtr);
    uint32_t *highValue = (uint32_t *) Core::Memory::getPointer(highPtr);
    if (uint64_t *sysClock = (uint64_t *)Core::Memory::getPointerUnchecked(sysClockPtr); sysClock != nullptr) {
        if (lowValue)
            *lowValue = (uint32_t)(*sysClock % 1'000'000);
        if (highValue)
            *highValue = (uint32_t)(*sysClock / 1'000'000);
    } else {
        LOG_WARN(logType, "can't convert system clock to microseconds if system clock is invalid!");
    }

    Core::Timing::consumeCycles(415);
    // LOG_SYSCALL(logType, "sceKernelSysClock2USec(sysclock: 0x%08x, low: 0x%08x, high: 0x%08x)", sysClockPtr, lowPtr, highPtr);
    return 0;
}

int sceKernelSysClock2USecWide(uint64_t clock, uint32_t lowPtr, uint32_t highPtr) {
    uint32_t *lowValue = (uint32_t *) Core::Memory::getPointer(lowPtr);
    uint32_t *highValue = (uint32_t *) Core::Memory::getPointer(highPtr);

    if (lowValue)
        *lowValue = (uint32_t)(clock / 1'000'000);
    if (highValue)
        *highValue = (uint32_t)(clock % 1'000'000);

    Core::Timing::consumeCycles(515);
    // LOG_SYSCALL(logType, "sceKernelSysClock2USecWide(clock: %lld, low: 0x%08x, high: 0x%08x)", clock, lowPtr, highPtr);
    return 0;
}

int sceKernelGetSystemTime(uint32_t timePtr) {
    if (uint64_t *sysClock = (uint64_t *)Core::Memory::getPointerUnchecked(timePtr); sysClock != nullptr) {
        uint64_t t = Core::Timing::getSystemTimeMicroseconds();
        *sysClock = t;
    }

    Core::Timing::consumeCycles(3565);
    // LOG_SYSCALL(logType, "sceKernelGetSystemTime(0x%08x) = %lld", timePtr, Core::Timing::getSystemTimeMicroseconds());
    return 0;
}

SceInt64 sceKernelGetSystemTimeWide() {
    uint64_t t = Core::Timing::getSystemTimeMicroseconds();
    Core::Timing::consumeCycles(3565);
    // LOG_SYSCALL(logType, "sceKernelGetSystemTimeWide() = %lld", t);
    return t;
}

uint32_t sceKernelGetSystemTimeLow() {
    uint32_t t = (uint32_t) Core::Timing::getSystemTimeMicroseconds();
    Core::Timing::consumeCycles(2565);
    // LOG_SYSCALL(logType, "sceKernelGetSystemTimeLow() = %d", t);
    return t;
}

static bool volatileMemoryLocked;

// volatile memory
int sceKernelVolatileMemLock(int unk, uint32_t addressPtr, uint32_t sizePtr) {
    uint32_t *ptr = (uint32_t *) Core::Memory::getPointerUnchecked(addressPtr);
    uint32_t *size = (uint32_t *) Core::Memory::getPointerUnchecked(sizePtr);

    if (volatileMemoryLocked) {
        threadAddToWaitingList(current, 0, WAITTYPE_VMEM);
        LOG_WARN(logType, "sceKernelVolatileMemLock() locking volatile mem");
        Core::Allegrex::setProcessorFailed(true);
    }

    if (ptr && size) {
        *ptr = 0x08400000;
        *size = 0x400000;
    }
    LOG_WARN(logType, "sceKernelVolatileMemLock()");
    return 0;
}

int sceKernelVolatileMemTryLock(int unk, uint32_t addressPtr, uint32_t sizePtr) {
    uint32_t *ptr = (uint32_t *) Core::Memory::getPointerUnchecked(addressPtr);
    uint32_t *size = (uint32_t *) Core::Memory::getPointerUnchecked(sizePtr);

    if (!volatileMemoryLocked && ptr && size) {
        *ptr = 0x08400000;
        *size = 0x400000;
        LOG_SYSCALL(logType, "sceKernelVolatileTryMemLock(%d, 0x%08x, 0x%08x)", unk, addressPtr, sizePtr);
    } else {
        LOG_WARN(logType, "can't try-lock volatile memory");
        return -1;
    }
    return 0;
}

int sceKernelVolatileMemUnlock(int unk) {
    volatileMemoryLocked = false;
    LOG_SYSCALL(logType, "sceKernelVolatileMemUnlock(%d)", unk);
    return 0;
}

int sceKernelCheckCallback() {
    LOG_SYSCALL(logType, "sceKernelCheckCallback()");
    return 0;
}

static void doWakeup(SceUID thid) {
    bool found = false;

    auto it = std::find(threadWaitingList.begin(), threadWaitingList.end(), thid);
    
    if (it != threadWaitingList.end()) {
        auto thr = getKernelObject<PSPThread>(thid);
        if (!thr) {
            LOG_ERROR(logType, "can't wakeup thread while it's not in waiting list!");
            threadWaitingList.erase(it);
            return;
        }

        thr->waitId = 0;
        thr->waitType = WAITTYPE_NONE;
        thr->status = THREAD_STATUS_READY;
        threadWaitingList.erase(it);
    }
}

bool createThreadFromModule(PSPModule *pspModule) {
    SceUID threadUID = sceKernelCreateThread("root", pspModule->entryAddress, 32, 0x100, PSP_THREAD_ATTR_USER, 0, pspModule->getUID());
    sceKernelStartThread(threadUID, 0, 0);

    auto thread = getKernelObject<PSPThread>(threadUID);
    if (thread->moduleId != -1) {
        PSPModule *pspModule = getKernelObject<PSPModule>(thread->moduleId);
        if (pspModule) {
            std::string stackFilename;
            if (pspModule->isFromUMD) {
                stackFilename += "disc0:/";
            } else {
                stackFilename += "umd0:/";
            }

            stackFilename += pspModule->fileNameForStack;

            Core::Memory::Utility::copyMemoryHostToGuest(thread->context.gpr[MIPS_REG_SP] + 64, stackFilename.c_str(), (uint32_t)stackFilename.length() + 1);
            thread->context.gpr[MIPS_REG_A0] = uint32_t(stackFilename.length() + 1);
            thread->context.gpr[MIPS_REG_A1] = thread->context.gpr[MIPS_REG_SP] + 64;
        }
    }

    for (auto& mod : pspModule->moduleImport) {
        // LOG_INFO(logType, "-- importing from module %s -- number of functions %d", mod.name, mod.stubIndex);
        printf("%s %d\n", mod.name, mod.stubIndex);

        for (int i = 0; i < mod.stubIndex; i++) {
            uint32_t nid = mod.stub[i].nid;
            uint32_t addr = mod.stub[i].address;

            // LOG_INFO(logType, "importing NID 0x%08x -> 0x%08x", nid, addr);
            printf("0x%08X 0x%08X\n", nid, addr);

            Core::Memory::write32(addr, Core::HLE::Assembly::makeJumpReturnAddress());
            Core::Memory::write32(addr + 0x4, 0xc);
            setSyscallAddressHandler(addr + 0x4, nid);
        }
    }
    return true;
}

uint32_t hleGetReturnFromAddress(ThreadReturnAddressType type) {
    switch (type) {
    case THREAD_RETURN_ADDRESS_IDLE: return 0x88000000;
    case THREAD_RETURN_ADDRESS_RETURN_FROM_THREAD: return 0x88000008;
    case THREAD_RETURN_ADDRESS_RETURN_FROM_CALLBACK: return 0x88000010;
    case THREAD_RETURN_ADDRESS_RETURN_FROM_INTERRUPT: return 0x88000018;
    case THREAD_RETURN_ADDRESS_ASIO: return 0x88000020;
    }
    return 0;
}

static void hleCurrentThreadSetRunningState(int state) {
    current->threadState = state;
}

void hleCurrentThreadEnableCallbackState() {
    hleCurrentThreadSetRunningState(PSP_THREAD_STATE_CALLBACK);
}

static void hleThreadSwitchContext() {
    current = threadReadyQueueGet();

    if (!current)
        return;

    Core::Allegrex::loadState(current);
    current->status = THREAD_STATUS_RUNNING;
}

static void hleThreadEndContext() {
    Core::Allegrex::saveCurrentState();
    if (current->status == THREAD_STATUS_RUNNING)
        current->status = THREAD_STATUS_READY;
}

static bool isCurrentThreadStateInvalid() {
    return current->status != THREAD_STATUS_RUNNING;
}

static bool returnFromCallbackState;

void hleSetReturnFromCallbackState(bool state) {
    returnFromCallbackState = state;
}

static void __hleRunAllegrexCallbackLoop(uint32_t entryPoint, uint32_t args[], int argSize) {
    uint32_t oldArgs[3];
    for (int i = 0; i < argSize; i++) {
        oldArgs[i] = cpu.reg[MIPS_REG_A0 + i];
        cpu.reg[MIPS_REG_A0 + i] = args[i];
    }

    uint32_t oldSP = cpu.reg[MIPS_REG_SP];
    uint32_t oldV[2] = { cpu.reg[MIPS_REG_V0], cpu.reg[MIPS_REG_V1] };

    uint32_t pc = cpu.pc;
    uint32_t ra = cpu.reg[MIPS_REG_RA];
    cpu.pc = entryPoint;
    cpu.reg[MIPS_REG_RA] = hleGetReturnFromAddress(THREAD_RETURN_ADDRESS_RETURN_FROM_CALLBACK);

    while (!returnFromCallbackState) {
        Core::GPU::displayListRun(Core::GPU::getDisplayListFromQueue());

        uint64_t cycles = Core::Allegrex::interpreterBlockStep();
        if (cycles == 0)
            break;

        Core::Timing::consumeCycles(cycles);
    }

    hleSetReturnFromCallbackState(false);
    cpu.pc = pc;
    cpu.reg[MIPS_REG_RA] = ra;
    for (int i = 0; i < argSize; i++)
        cpu.reg[MIPS_REG_A0 + i] = oldArgs[i];
    for (int i = 0; i < 2; i++)
        cpu.reg[MIPS_REG_V0 + i] = oldV[i];
    cpu.reg[MIPS_REG_SP] = oldSP;
}

static void __hleRunAllegrexInterruptLoop(uint32_t entryPoint, uint32_t args[], int argSize) {
    cpu.reg[0] = 0;
    for (int i = 1; i < 32; i++)
        cpu.reg[i] = 0xDEADBEEF;

    for (int i = 0; i < argSize; i++)
        cpu.reg[MIPS_REG_A0 + i] = args[i];

    cpu.reg[MIPS_REG_K0] = cpu.reg[MIPS_REG_SP] = 0x083FFF00;
    cpu.pc = entryPoint;
    cpu.reg[MIPS_REG_RA] = hleGetReturnFromAddress(THREAD_RETURN_ADDRESS_RETURN_FROM_CALLBACK);

    while (!returnFromCallbackState) {
        Core::GPU::displayListRun(Core::GPU::getDisplayListFromQueue());

        uint64_t cycles = Core::Allegrex::interpreterBlockStep();
        if (cycles == 0)
            break;

        Core::Timing::consumeCycles(cycles);
    }

    hleSetReturnFromCallbackState(false);
}

static void hleThreadRunCallbacks() {
    if (Core::Allegrex::isProcessorInDelaySlot()) {
        Core::Timing::consumeCycles(1);
        Core::Allegrex::interpreterStep();
    }

    if (auto powerCb = getReadyPowerCallbacks(); powerCb) {
        while (powerCb->size() != 0) {
            auto cbid = (*powerCb)[0];
            Callback *c = getKernelObject<Callback>(cbid);
            if (!c) {
                powerCb->pop_front();
                continue;
            }

            uint32_t args[] = { 1, (uint32_t) c->notifyArg, c->common };

            __hleRunAllegrexCallbackLoop(c->handler, args, 3);
            c->notifyCount = 0;
            c->notifyArg = 0;

            powerCb->pop_front();
        }
    }

    Callback *umdCallback = __getRegisteredUMDCallback();
    if (umdCallback) {
        uint32_t args[] = { 1, (uint32_t) umdCallback->notifyArg, umdCallback->common };

        __hleRunAllegrexCallbackLoop(umdCallback->handler, args, 3);
        umdCallback->notifyCount = 0;
        umdCallback->notifyArg = 0;
    }

#if 0
    Callback *memstick = getRegisteredMemStickCallback();
    if (memstick) {
        uint32_t args[] = { 1, (uint32_t) memstick->notifyArg, memstick->common };
        __hleRunAllegrexCallbackLoop(memstick->handler, args, 3);
        setMemStickCallback(nullptr);
    }
#endif

    hleCurrentThreadSetRunningState(PSP_THREAD_STATE_NORMAL);
}

static void hleThreadHandleInterrupts() {
    if (!__hleInterruptsEnabled())
        return;

    auto& pendingInterruptList = getPendingInterruptList();

    if (pendingInterruptList.size() != 0) {
        if (Core::Allegrex::isProcessorInDelaySlot()) {
            Core::Timing::consumeCycles(1);
            Core::Allegrex::interpreterStep();
        }

        saveCurrentState();
        PSPThread thr = *current;

        for (auto it = pendingInterruptList.begin(); it != pendingInterruptList.end(); ) {
            __hleRunAllegrexInterruptLoop(it->handler, it->args, 2);
            break;
        }

        pendingInterruptList.clear();
        loadState(&thr);
    }
}

static void hleHandleEvents() {
    std::vector<int> awakenedThreads;

    for (auto thrUID = threadWaitingList.begin(); thrUID != threadWaitingList.end(); ) {
        auto i = getKernelObject<PSPThread>(*thrUID);

        if (!i || i->status != THREAD_STATUS_WAITING) {
            thrUID = threadWaitingList.erase(thrUID);
            continue;
        }

        switch (i->waitType) {
        case WAITTYPE_AUDIOCHANNEL:
            if (i->waitData[0] < Core::Timing::getCurrentCycles())
                awakenedThreads.push_back(i->getUID());
            break;
        case WAITTYPE_VBLANK:
        {
            int64_t vblankRefreshCycleCount = Core::Timing::msToCycles(16.67f);
            static uint64_t oldVblankCycleCounter = vblankRefreshCycleCount;

            if (auto cycles = Core::Timing::getBaseCycles(); oldVblankCycleCounter < cycles) {
                int64_t cyclesCount = Core::Timing::getCurrentCycles() - Core::Timing::getBaseCycles();
                Core::Timing::consumeCycles(cyclesCount);
                oldVblankCycleCounter = cycles + vblankRefreshCycleCount;
                awakenedThreads.push_back(i->getUID());
            }
            break;
        }
        case WAITTYPE_DELAY:
        {
            auto futureCycles = i->waitData[0];
            if (Core::Timing::getCurrentCycles() >= futureCycles) {
                awakenedThreads.push_back(i->getUID());
            }

            break;
        }
        case WAITTYPE_SEMA:
        {
            auto s = getKernelObject<Semaphore>(i->waitId);
            if (!s) {
                thrUID = threadWaitingList.erase(thrUID);
                awakenedThreads.push_back(i->getUID());
                break;
            }

            int threadSignal = (int) i->waitData[0];

            if (uint32_t *p = (uint32_t*) Core::Memory::getPointerUnchecked((uint32_t)current->waitData[1]); p) {
                if (Core::Timing::getCurrentCycles() >= current->waitData[2]) {
                    s->counter -= threadSignal;
                    awakenedThreads.push_back(i->getUID());
                    s->waitingThreadsCount--;
                    break;
                }
            }

            if (s->counter >= threadSignal) {
                s->counter -= threadSignal;
                awakenedThreads.push_back(i->getUID());
                s->waitingThreadsCount--;
            }
            break;
        }
        case WAITTYPE_THREADEND:
        {
            auto thr = getKernelObject<PSPThread>(i->waitId);
            if (!thr) {
                awakenedThreads.push_back(i->getUID());
                break;
            }

            if (thr->status == THREAD_STATUS_DORMANT) {
                awakenedThreads.push_back(i->getUID());
            }
            break;
        }
        case WAITTYPE_EVENTFLAG:
        {
            auto e = getKernelObject<EventFlag>(i->waitId);

            auto applyClearPattern = [&](uint32_t wait, uint32_t threadPattern) {
                if ((wait & PSP_EVENT_WAITCLEAR) == PSP_EVENT_WAITCLEAR)
                    e->currentPattern &= ~threadPattern;

                if ((wait & PSP_EVENT_WAITCLEARALL) == PSP_EVENT_WAITCLEARALL)
                    e->currentPattern = 0;

                e->numWaitThreads--;
            };

            if (!e) {
                awakenedThreads.push_back(i->getUID());
                break;
            }

            uint32_t threadPattern = (uint32_t) i->waitData[0];
            uint32_t eventFlagCurrentPattern = e->currentPattern;

            switch (((int) i->waitData[1]) & 1) {
            case PSP_EVENT_WAITAND:
                if ((threadPattern & eventFlagCurrentPattern) == threadPattern) {
                    applyClearPattern((uint32_t)i->waitData[1], (uint32_t)threadPattern);
                    awakenedThreads.push_back(i->getUID());
                }
                break;
            case PSP_EVENT_WAITOR:
                if ((threadPattern & eventFlagCurrentPattern) != 0) {
                    applyClearPattern((uint32_t)i->waitData[1], (uint32_t)threadPattern);
                    awakenedThreads.push_back(i->getUID());
                }
                break;
            }
            break;
        }
        case WAITTYPE_VMEM:
        {
            printf("volatile memory\n");
            std::exit(0);
            break;
        }
        }
        thrUID++;
    }

    auto& alarmList = hleSchedulerGetAlarmList();
    for (auto it = alarmList.begin(); it != alarmList.end(); ) {
        bool exceededAmount = (uint64_t)(*it)->clock < (uint64_t)Core::Timing::getCurrentCycles();
        if (exceededAmount) {
            uint32_t args[] = { (*it)->common };
            saveCurrentState();
            __hleRunAllegrexInterruptLoop((*it)->alarmHandler, args, 1);

            if (cpu.reg[MIPS_REG_V0] == 0) {
                it = alarmList.erase(it);
                sceKernelCancelAlarm((*it)->getUID());
                printf("cancel alarm?\n");
                std::exit(0);
            } else {
                (*it)->clock = Core::Timing::getCurrentCycles() + Core::Timing::usToCycles((uint64_t)cpu.reg[MIPS_REG_V0]);
            }

            loadState(current);
        } else
            ++it;
    }

    if (__hleInterruptsEnabled()) {
        hleTriggerInterrupt(30, -1);
    }

    for (auto& i : awakenedThreads)
        doWakeup(i);

    for (auto i = threadExitList.begin(); i != threadExitList.end(); ) {
        auto thr = getKernelObject<PSPThread>(*i);
        if (!thr) {
            i = threadExitList.erase(i);
            continue;
        }

        startedThreadList.erase(std::find(startedThreadList.begin(), startedThreadList.end(), thr->getUID()));

        if (auto it = std::find_if(threadReadyQueue.begin(), threadReadyQueue.end(),
            [&](PSPThread *a) { return a->getUID() == thr->getUID(); }); it != threadReadyQueue.end()) {
            threadReadyQueue.erase(it);
        }

        destroyKernelObject(*i);
        i = threadExitList.erase(i);
    }
}

void hleThreadRunFor(uint64_t cycles) {
    if (Core::Allegrex::isProcessorFailed())
        return;

    Core::Timing::setBaseCycles(Core::Timing::getCurrentCycles());
    uint64_t cyclesFuture = Core::Timing::getCurrentCycles() + cycles;

    for (uint64_t k = Core::Timing::getCurrentCycles(); !Core::Allegrex::isProcessorFailed() && k < cyclesFuture; k = Core::Timing::getCurrentCycles()) {
        hleThreadSwitchContext();

        if (!current)
            break;

        Core::Timing::getCurrentCycles();
        Core::Timing::setDowncount(Core::Timing::getTimesliceQuantum());
        int64_t& dc = Core::Timing::getDowncount();

        // printf("%d %d\n", Core::Timing::getSystemTimeMilliseconds(), Core::Timing::getCurrentCycles());
        do {
            Core::GPU::displayListRun(Core::GPU::getDisplayListFromQueue());

            if (isCurrentThreadStateInvalid())
                break;

            uint64_t cycles = Core::Allegrex::interpreterBlockStep();
            if (cycles == 0)
                break;

            Core::Timing::consumeCycles(cycles);

            hleThreadHandleInterrupts();
            switch (current->threadState) {
            case PSP_THREAD_STATE_CALLBACK:
                hleThreadRunCallbacks();
                break;
            }
        } while (dc > 0);

        if (dc > 0)
            Core::Timing::addIdleCycles(dc);

        hleHandleEvents();
        hleThreadEndContext();
    }
}

static void createIdle() {
    SceUID uid;
    uint32_t idleAddress = hleGetReturnFromAddress(THREAD_RETURN_ADDRESS_IDLE);

    uid = sceKernelCreateThread("_IdleThread", idleAddress, 115, 0x200, PSP_THREAD_ATTR_KERNEL, 0x0);
    if (uid > 0) {
        sceKernelStartThread(uid, 0, 0);
        idle[0] = getKernelObject<PSPThread>(uid);
        idle[0]->context.gpr[MIPS_REG_RA] = idleAddress;
        idle[0]->context.gpr[MIPS_REG_SP] = 0x803FFFC;
    }

    current = idle[0];
}

static bool kernelThreadInitialized = false;

void kernelThreadInitialize() {
    if (kernelThreadInitialized)
        return;

    LOG_INFO(logType, "initializing kernel thread");

    // states
    current = nullptr;

    // behaviours
    createIdle();

    kernelThreadInitialized = true;
}

void kernelThreadReset() {
    if (!kernelThreadInitialized)
        return;

    LOG_INFO(logType, "resetting kernel thread state");

    // states
    current = nullptr;

    // behaviours
    kernelClearThreadState();
    createIdle();
}

void kernelThreadDestroy() {
    LOG_INFO(logType, "destroying kernel thread state");

    // states
    current = rootThread = idle[0] = idle[1] = nullptr;

    // behaviours
    kernelClearThreadState();
    kernelThreadInitialized = false;
}
}