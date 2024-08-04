#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct VPL : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_VPL;
private:
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "VPL"; }
    const char *getTypeName() override { return "VPL"; }
};
}