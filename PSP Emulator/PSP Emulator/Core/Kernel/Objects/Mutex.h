#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct Mutex : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_MUTEX;
private:
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Mutex"; }
    const char *getTypeName() override { return "Mutex"; }
};
}