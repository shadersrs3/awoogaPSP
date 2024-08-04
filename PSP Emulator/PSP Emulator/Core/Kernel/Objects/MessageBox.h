#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct MessageBox : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_MESSAGE_BOX;
private:
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Message Box"; }
    const char *getTypeName() override { return "Message Box"; }
};
}