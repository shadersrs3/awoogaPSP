#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct Callback : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_CALLBACK;
public:
    char name[32];
    uint32_t handler;
    uint32_t common;
    SceUID threadId;
    int notifyArg;
    int notifyCount;
    bool ready;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Callback"; }
    const char *getTypeName() override { return "Callback"; }
};
}