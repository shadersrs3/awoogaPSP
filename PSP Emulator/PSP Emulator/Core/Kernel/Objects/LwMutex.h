#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct LwMutex : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_LW_MUTEX;
private:
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Lightweight Mutex"; }
    const char *getTypeName() override { return "Lightweight Mutex"; }
};
}