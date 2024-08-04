#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {

enum PSPThreadState {
    PSP_THREAD_STATE_NORMAL,
    PSP_THREAD_STATE_CALLBACK,
};

struct PSPThread : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_THREAD;
public:
    char name[32];
    uint32_t entryPoint;
    int status;
    uint32_t attr;
    uint32_t stackPointer;
    int stackSize;
    int initialPriority;
    int currentPriority;
    int waitType;
    SceUID waitId;
    int wakeupCount;
    int exitStatus;
    uint64_t clockCyclesRan;
    SceUInt interruptPreemptCount;
    SceUInt threadPreemptCount;
    SceUInt releaseCount;
    uint32_t gpReg;
    int moduleId;
    struct Context {
        uint32_t gpr[32];
        uint32_t fpr[32];
        uint32_t vpr[128];
        uint32_t vfpuCtrl[16];
        uint32_t fcr31;
        uint32_t hi, lo;
        uint32_t pc;
        bool fpcond;
        bool llBit;
    } context;
    int threadState;
    uint64_t waitData[16];
    std::vector<int> registeredCallbacks;
    uint32_t stackFrame[128];
    int stackFrameIndex;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }
    static constexpr const char *getStaticTypeName() { return "PSP Thread"; }
    const char *getTypeName() override { return "PSP Thread"; }
    void resetContext();
};
}