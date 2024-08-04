#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

namespace Core::Kernel {
struct Partition : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_PARTITION;
private:
    char name[256];
    uint32_t partitionAddress;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }
    static constexpr const char *getStaticTypeName() { return "Partition"; }
    const char *getTypeName() override { return "Partition"; }

    void setName(const char *name);
    const uint32_t& getPartitionAddress() { return partitionAddress; }
    void setPartitionAddress(uint32_t address) { partitionAddress = address; }
};
}