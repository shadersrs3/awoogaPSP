#pragma once

#include <Core/Kernel/Objects/KernelObject.h>

#include <vector>

namespace Core::Kernel {
struct MemoryBlock {
    uint32_t addr;
    uint32_t size;
    bool isFree;
};

struct FPL : public KernelObject {
private:
    static constexpr KernelObjectType staticTypeId = KERNEL_OBJECT_FPL;
public:
    char name[256];
    uint32_t size;
    int numBlocks;
    uint32_t addressBottom, addressTop;
    std::vector<MemoryBlock> blocks;
public:
    constexpr static KernelObjectType getStaticTypeId() { return staticTypeId; }

    static constexpr const char *getStaticTypeName() { return "FPL"; }
    const char *getTypeName() override { return "FPL"; }
};
}