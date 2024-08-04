#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <algorithm>

#include <cstring>

#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelSystemMemory.h>
#include <Core/Kernel/sceKernelError.h>

#include <Core/Kernel/Objects/Partition.h>
#include <Core/Kernel/Objects/FPL.h>

#include <Core/PSP/MemoryMap.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelSystemMemory";
static constexpr const int defaultSizeAlignment = 256;

static MemoryAllocator userMem, kernelMem, volatileMem;

void MemoryAllocator::init(const std::string& name, uint32_t start, uint32_t end) {
    allocatorName = name;
    startAddress = start;
    endAddress = end;
    memorySize = (endAddress - startAddress) + 1;
    topAddress = (endAddress - 0x10) + 1;
}

void MemoryAllocator::reset() {
}

void MemoryAllocator::destroy() {

}

uint32_t MemoryAllocator::allocHigh(int size, const std::string& name, int alignment) {
    uint32_t newSize = (size + 0xFF) & ~0xFF;
    uint32_t curAddress = topAddress;
    topAddress = topAddress - newSize - 0x5000;
    return curAddress;
}

uint32_t MemoryAllocator::allocLow(int size, const std::string& name, int alignment) {
    bool foundFreeMemory = false;
    uint32_t address = startAddress;
    MemoryChunk m;
    MemoryChunk *p;
    uint32_t allocatedSize = (size + alignment) & ~(alignment);
    std::sort(memoryChunkLow.begin(), memoryChunkLow.end(), [](MemoryChunk a, MemoryChunk b) { return a.address < b.address; });
    int idx = 0;

    for (auto it = memoryChunkLow.begin();
        it != memoryChunkLow.end(); it++) {
        p = &(*it);

        if (address < p->address) {
            if (idx + 1 == memoryChunkLow.size() && address + allocatedSize > p->address) { // reached the end
                address = p->address + p->allocatedSize;
                break;
            }

            if (address + allocatedSize <= p->address)
                break;
        }

        address = p->address + p->allocatedSize;
        idx++;
    }

    std::memset(m.name, 0, sizeof m.name);
    if (name.size() != 0) {
        std::strncpy(m.name, name.c_str(), sizeof m.name - 1);
    }

    m.address = address;
    m.size = size;
    m.allocatedSize = allocatedSize;
    memoryChunkLow.push_back(m);
    return m.address;
}

uint32_t MemoryAllocator::allocAt(uint32_t addr, int size, const std::string& name, int alignment) {
    bool validBlock = true;
    MemoryChunk m;
    uint32_t topAddress = (endAddress - ((endAddress - startAddress + 1) >> 3)) + 1;
    uint32_t curAddr = addr;

    if (addr >= topAddress) {
        LOG_ERROR(logType, "allocAt top address unimplemented addr 0x%08x size 0x%08x!", addr, size);
        return (uint32_t)-1;
    }

    std::sort(memoryChunkLow.begin(), memoryChunkLow.end(), [](MemoryChunk a, MemoryChunk b) { return a.address < b.address; });
    m.address = addr & ~(alignment - 1);
    m.size = size;
    m.allocatedSize = (size + alignment - 1) & ~(alignment - 1);
    std::memset(m.name, 0, sizeof m.name);
    if (name.size() != 0) {
        std::strncpy(m.name, name.c_str(), sizeof m.name - 1);
    }

    for (auto& i : memoryChunkLow) {
        if (addr < i.address + i.allocatedSize) {
            LOG_ERROR(logType, "MemoryAllocator::allocAt unimplemented!");
            validBlock = false;
            return (uint32_t)-1;
        }
    }

    if (validBlock)
        memoryChunkLow.push_back(m);
    return (uint32_t) addr;
}

bool MemoryAllocator::free(uint32_t addr, int size) {
    return false;
}

void MemoryAllocator::debug() {
    LOG_DEBUG(logType, "--------- %s debug ---------", allocatorName.c_str());
    LOG_DEBUG(logType, "start address 0x%08x, end address 0x%08x", startAddress, endAddress);
    LOG_DEBUG(logType, "----------------------------");
}

static bool isValidPartition(int partition) {
    return !(partition < 1 || partition > 9 || partition == 7);
}

MemoryAllocator *getMemoryAllocatorFromID(int partitionId) {
    switch (partitionId) {
    case 1:
        return &kernelMem;
    case 2:
        return &userMem;
    case 5:
        return &volatileMem;
    default:
        LOG_ERROR(logType, "unknown memory allocator partition id %d", partitionId);
    }
    return nullptr;
}

MemoryAllocator *getMemoryAllocatorFromAddress(uint32_t address) {
    if (address >= userMem.startAddress && address <= userMem.endAddress) {
        return &userMem;
    } else if (address >= volatileMem.startAddress && address <= volatileMem.endAddress) {
        return &volatileMem;
    } else if (address >= kernelMem.startAddress && address <= kernelMem.endAddress) {
        return &kernelMem;
    }
    return nullptr;
}

uint32_t allocateMemory(const std::string& name, int partitionId, int type, int size, uint32_t addr) {
    int allocatedSize;
    int alignment;
    uint32_t address;
    auto allocator = getMemoryAllocatorFromID(partitionId);

    if (!allocator)
        return (uint32_t)-1;

    alignment = defaultSizeAlignment - 1;
    allocatedSize = (size + alignment) & ~alignment;
    if (type == PSP_SMEM_LowAligned || type == PSP_SMEM_HighAligned) {
        if (addr >= 256)
            alignment = addr - 1;
    }

    switch (type) {
    case PSP_SMEM_LowAligned:
    case PSP_SMEM_Low:
        address = allocator->allocLow(allocatedSize, name, alignment);
        break;
    case PSP_SMEM_HighAligned:
    case PSP_SMEM_High:
        address = allocator->allocHigh(allocatedSize, name, alignment);
        break;
    case PSP_SMEM_Addr:
        address = allocator->allocAt(addr, allocatedSize, name);
        break;
    default:
        address = (uint32_t)-1;
    }
    
    if (addr >= 256)
        address = (address + alignment) & ~alignment;
    return address;
}

void freeMemory(uint32_t address, int size) {
    auto allocator = getMemoryAllocatorFromAddress(address);
    if (allocator)
        allocator->free(address, size);
}

SceUID sceKernelAllocPartitionMemory(SceUID partitionId, const char *name, int type, SceSize size, uint32_t addr) {
    int error;
    Partition *p;

    if (!name) {
        LOG_ERROR(logType, "can't create partition name!");
        return SCE_KERNEL_ERROR_ERROR;
    }


    if (partitionId < 1 || partitionId > 9 || partitionId == 7) {
        LOG_ERROR(logType, "illegal partition id %d", partitionId);
        return SCE_KERNEL_ERROR_ILLEGAL_ARGUMENT;
    }

    MemoryAllocator *allocator = getMemoryAllocatorFromID(partitionId);
    if (!allocator) {
        LOG_ERROR(logType, "can't get partition allocator id %d", partitionId);
        return SCE_KERNEL_ERROR_ILLEGAL_PARTITION;
    }

    if (size == 0) {
        LOG_ERROR(logType, "partition size invalid", partitionId);
        return SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED;
    }

    uint32_t address = allocateMemory(name, partitionId, type, size, addr);

    if (address == -1) {
        LOG_ERROR(logType, "can't allocate partition memory");
        return SCE_KERNEL_ERROR_MEMBLOCK_ALLOC_FAILED;
    }

    p = createKernelObject<Partition>(&error);
    if (!p) {
        LOG_ERROR(logType, "can't create partition object");
        return error;
    }

    p->setName(name);
    p->setPartitionAddress(address);
    saveKernelObject(p);
    LOG_SYSCALL(logType, "sceKernelAllocPartitionMemory(%d, %s, 0x%08x, 0x%08x, 0x%08x) = 0x%06x", partitionId, name, type, size, addr, p->getUID());
    return p->getUID();
}

int sceKernelFreePartitionMemory(SceUID blockId) {
    return 0;
}

uint32_t sceKernelGetBlockHeadAddr(SceUID blockId) {
    Partition *p = getKernelObject<Partition>(blockId);
    uint32_t address;
    if (p) {
        address = p->getPartitionAddress();
        LOG_SYSCALL(logType, "sceKernelGetBlockHeadAddr(0x%06x) = 0x%08x", blockId, address);
        return address;
    } else {
        LOG_ERROR(logType, "can't get memory head address for block id 0x%06x", blockId);
    }
    return 0;
}

uint32_t sceKernelMaxFreeMemSize() {
    auto allocator = getMemoryAllocatorFromID(2);
    uint32_t addressSize = (allocator->endAddress + 1) - allocator->topAddress;
    uint32_t freeSize = allocator->memorySize - addressSize;
    for (auto& i : allocator->memoryChunkLow) {
        freeSize -= i.allocatedSize;
    }

    freeSize = (freeSize - 0xFF) & ~0xFF;
    LOG_SYSCALL(logType, "sceKernelMaxFreeMemSize() = 0x%08x : %d KB", freeSize, freeSize >> 10);
    return freeSize;
}

int sceKernelCreateFpl(const char *name, int part, int attr, uint32_t size, uint32_t blocks, uint32_t optPtr) {
    auto allocator = getMemoryAllocatorFromID(part);

    if (!allocator) {
        LOG_ERROR(logType, "unknown FPL memory partition %d", part);
        return SCE_KERNEL_ERROR_ILLEGAL_PARTITION;
    }

    uint32_t addr = allocateMemory("FPL_" + std::string(name), part, PSP_SMEM_LowAligned, size, 0x100);
    
    if (addr == -1) {
        LOG_ERROR(logType, "FPL partition memory full!");
        return SCE_KERNEL_ERROR_OUT_OF_MEMORY;
    }

    FPL *fpl = createKernelObject<FPL>();
    if (!fpl) {
        return -1;
    }

    fpl->numBlocks = blocks;
    fpl->size = size;
    fpl->addressTop = addr;
    fpl->addressBottom = addr;
    saveKernelObject(fpl);
    LOG_SYSCALL(logType, "sceKernelCreateFpl(name: %s, part: %d, attr: 0x%08x, size: 0x%08x, blocks: 0x%08x, opt: 0x%08x) = 0x%06x addr bottom 0x%08x", name, part, attr, size, blocks, optPtr, fpl->getUID(), fpl->addressBottom);
    return fpl->getUID();
}

static int __kernelAllocateFpl(SceUID uid, uint32_t dataPtr, uint32_t timeoutPtr) {
    auto fpl = getKernelObject<FPL>(uid);
    if (!fpl) {
        LOG_ERROR(logType, "unknown allocate FPL uid 0x%06x", uid);
        return SCE_KERNEL_ERROR_UNKNOWN_FPLID;
    }

    if (fpl->addressTop >= fpl->addressBottom + fpl->size * fpl->numBlocks) {
        LOG_ERROR(logType, "allocate FPL out of memory!");
        return SCE_KERNEL_ERROR_OUT_OF_MEMORY;
    }

    Core::Memory::write32(dataPtr, fpl->addressTop);
    fpl->addressTop += fpl->size;
    return 0;
}

int sceKernelAllocateFpl(SceUID uid, uint32_t dataPtr, uint32_t timeoutPtr) {
    int error = __kernelAllocateFpl(uid, dataPtr, timeoutPtr);

    if (error < 0) {
        LOG_ERROR(logType, "sceKernelAllocateFpl: can't allocate FPL error 0x%08x", error);
    } else {
        LOG_SYSCALL(logType, "sceKernelAllocateFpl(0x%06x, 0x%08x, 0x%08x)", uid, dataPtr, timeoutPtr);
    }
    return 0;
}

int sceKernelAllocateFplCB(SceUID uid, uint32_t dataPtr, uint32_t timeoutPtr) {
    int error = __kernelAllocateFpl(uid, dataPtr, timeoutPtr);

    hleCurrentThreadEnableCallbackState();
    if (error < 0) {
        LOG_ERROR(logType, "sceKernelAllocateFplCB: can't allocate FPL error 0x%08x", error);
    } else {
        LOG_SYSCALL(logType, "sceKernelAllocateFplCB(0x%06x, 0x%08x, 0x%08x)", uid, dataPtr, timeoutPtr);
    }
    return 0;
}

int sceKernelTryAllocateFpl(SceUID uid, uint32_t dataPtr) {
    int error = __kernelAllocateFpl(uid, dataPtr, 0);

    if (error < 0) {
        LOG_ERROR(logType, "sceKernelAllocateFpl: can't allocate FPL error 0x%08x", error);
    } else {
        LOG_SYSCALL(logType, "sceKernelTryAllocateFpl(0x%06x, 0x%08x)", uid, dataPtr);
    }
    return 0;
}


void systemMemoryInit() {
    userMem.init("user memory", 0x0880'0000, 0x0880'0000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1);
    kernelMem.init("kernel memory", 0x0800'0000, 0x0840'0000 - 1);
    volatileMem.init("volatile memory", 0x0840'0000, 0x0880'0000 - 1);

    userMem.allocLow(0x4000, "reservedUserMemoryArea");
}

void systemMemoryReset() {
    userMem.reset();
    kernelMem.reset();
    volatileMem.reset();
}

void systemMemoryDestroy() {
    userMem.destroy();
    kernelMem.destroy();
    volatileMem.destroy();
}
}