#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct Alarm : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_ALARM;
public:
    uint64_t clock;
    uint32_t alarmHandler;
    uint32_t common;
    bool cancelled;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Alarm"; }
    const char *getTypeName() override { return "Alarm"; }
};
}