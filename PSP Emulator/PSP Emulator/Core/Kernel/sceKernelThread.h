#pragma once

#include <Core/Kernel/sceKernelFactory.h>
#include <Core/Kernel/Objects/Thread.h>
#include <Core/Kernel/Objects/Module.h>
#include <Core/PSP/Types.h>

namespace Core::Kernel {
enum : int {
    THREAD_STATUS_RUNNING = 1,
    THREAD_STATUS_READY = 2,
    THREAD_STATUS_WAITING = 4,
    THREAD_STATUS_SUSPEND = 8,
    THREAD_STATUS_STOPPED = 16,
    THREAD_STATUS_KILLED = 32, /* Thread manager has killed the thread (stack overflow) */
    THREAD_STATUS_DORMANT = 64,
};

enum ThreadReturnAddressType {
    THREAD_RETURN_ADDRESS_IDLE,
    THREAD_RETURN_ADDRESS_RETURN_FROM_THREAD,
    THREAD_RETURN_ADDRESS_RETURN_FROM_CALLBACK,
    THREAD_RETURN_ADDRESS_RETURN_FROM_INTERRUPT,
    THREAD_RETURN_ADDRESS_ASIO
};

extern PSPThread *current;

// creation, start, etc.
SceUID sceKernelCreateThread(const char *name, uint32_t entry, int initPriority, int stackSize, SceUInt attr, uint32_t threadOptParamPtr, int modid = -1);
int sceKernelStartThread(SceUID thid, uint32_t arglen, uint32_t argPtr);
int sceKernelDeleteThread(SceUID thid);
int sceKernelExitDeleteThread(int status);
int sceKernelExitThread(int status);
int sceKernelTerminateDeleteThread(SceUID thid);
int sceKernelTerminateThread(SceUID thid);

// waits
int sceKernelWaitThreadEnd(SceUID thid, uint32_t timeoutPtr);
int sceKernelWaitThreadEndCB(SceUID thid, uint32_t timeoutPtr);

// wakeups
int sceKernelSleepThread();
int sceKernelSleepThreadCB();
int sceKernelWakeupThread(SceUID thid);
int sceKernelCancelWakeupThread(SceUID thid);

// attr, priority
int sceKernelChangeCurrentThreadAttr(SceUInt clearAttr, SceUInt setAttr);
int sceKernelChangeThreadPriority(SceUID thid, int priority);
int sceKernelRotateThreadReadyQueue(int priority);

// gets
int sceKernelGetThreadCurrentPriority();
int sceKernelGetThreadExitStatus(SceUID thid);
int sceKernelGetThreadId();
int sceKernelGetThreadmanIdList(int type, uint32_t readBufPtr, int readBufSize, uint32_t idPtr);
int sceKernelGetThreadmanIdType(SceUID uid);
int sceKernelGetThreadStackFreeSize(SceUID thid);
    
// unknown
int sceKernelCheckCallback();
int sceKernelCheckThreadStack();

// suspends, resumes
int sceKernelSuspendDispatchThread();
int sceKernelResumeDispatchThread();
int sceKernelSuspendThread(SceUID thid);
int sceKernelResumeThread(SceUID thid);

// delay
int sceKernelDelayThread(uint32_t us);
int sceKernelDelayThreadCB(uint32_t us);
int sceKernelDelaySysClockThread(uint32_t sysClockPtr);
int sceKernelDelaySysClockThreadCB(uint32_t sysClockPtr);

// referring
int sceKernelReferThreadEventHandlerStatus(SceUID uid, uint32_t infoPtr);
uint32_t sceKernelReferThreadProfiler();
uint32_t sceKernelReferGlobalProfiler();
int sceKernelReferThreadRunStatus(SceUID thid, uint32_t runStatusPtr);
int sceKernelReferThreadStatus(SceUID thid, uint32_t threadInfoPtr);

// event handler
SceUID sceKernelRegisterThreadEventHandler(const char *name, SceUID thid, int mask, uint32_t entry, uint32_t commonPtr);
int sceKernelReleaseThreadEventHandler(SceUID uid);

// interrupts

// system time / conversion
int sceKernelSysClock2USec(uint32_t sysClockPtr, uint32_t highPtr, uint32_t lowPtr);
int sceKernelSysClock2USecWide(uint64_t clock, uint32_t lowPtr, uint32_t highPtr);
int sceKernelGetSystemTime(uint32_t timePtr);
SceInt64 sceKernelGetSystemTimeWide();
uint32_t sceKernelGetSystemTimeLow();

// volatile memory
int sceKernelVolatileMemLock(int unk, uint32_t addressPtr, uint32_t sizePtr);
int sceKernelVolatileMemTryLock(int unk, uint32_t addressPtr, uint32_t sizePtr);
int sceKernelVolatileMemUnlock(int unk);

// callbacks
int sceKernelCheckCallback();

// hle
void createThreadFromModule(const char *name, Core::Kernel::PSPModule *pspModule);
uint32_t hleGetReturnFromAddress(ThreadReturnAddressType type);
void threadAddToWaitingList(PSPThread *thread, int waitId, int waitType);
std::vector<int> *getThreadWaitingList();
void hleCurrentThreadEnableCallbackState();

void hleSetReturnFromCallbackState(bool state);

bool createThreadFromModule(PSPModule *pspModule);

void hleThreadRunFor(uint64_t cycles);

void kernelThreadInitialize();
void kernelThreadReset();
void kernelThreadDestroy();
}