#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct EventFlag : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_EVENT_FLAG;
public:
    char name[32];
    SceUInt attr;
    SceUInt initPattern;
    SceUInt currentPattern;
    int numWaitThreads;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Event Flag"; }
    const char *getTypeName() override { return "Event Flag"; }
};
}