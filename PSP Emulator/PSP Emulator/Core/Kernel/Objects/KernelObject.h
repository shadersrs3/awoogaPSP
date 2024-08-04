#pragma once

#include <Core/Kernel/sceKernelFactory.h>
#include <Core/Kernel/sceKernelObjectTypes.h>

namespace Core::Kernel {
struct KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_UNKNOWN;
private:
    SceUID uid;
    KernelObjectType type;
public:
    KernelObject() : uid(0), type(KERNEL_OBJECT_UNKNOWN) {}
    virtual ~KernelObject() {}
    int& getUID() { return uid; }
    KernelObjectType& getType() { return type; }
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "Kernel Object"; }
    virtual const char *getTypeName() { return "Kernel Object"; }
    void destroyFromPool();
};
}