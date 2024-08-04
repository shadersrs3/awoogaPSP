#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct Semaphore : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_SEMAPHORE;
public:
    char name[32];
    uint32_t attr;
    int initCount;
    int maxCount;
    int counter;
    int waitingThreadsCount;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Semaphore"; }
    const char *getTypeName() override { return "Semaphore"; }
};
}