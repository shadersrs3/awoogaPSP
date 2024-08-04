#pragma once

#include <string>
#include <vector>
#include <cstdint>

#include <Core/PSP/Types.h>

namespace Core::Kernel {
enum : int {
    PSP_SMEM_Low = 0,
    PSP_SMEM_High,
    PSP_SMEM_Addr,
    PSP_SMEM_LowAligned,
    PSP_SMEM_HighAligned
};

struct MemoryChunk {
    char name[32];
    uint32_t address;
    int size;
    int allocatedSize;
};

struct MemoryAllocator {
public:
    std::string allocatorName;
    uint32_t startAddress, endAddress;
    uint32_t memorySize;
    uint32_t topAddress;
    std::vector<MemoryChunk> memoryChunkLow;
public:
    void init(const std::string& name, uint32_t start, uint32_t end);
    void reset();
    void destroy();
    uint32_t allocHigh(int size, const std::string& name = "", int alignment = 255);
    uint32_t allocLow(int size, const std::string& name = "", int alignment = 255);
    uint32_t allocAt(uint32_t addr, int size, const std::string& name = "", int alignment = 255);
    bool free(uint32_t addr, int size = -1);
    void debug();

    int alignSize(int size, int alignment);
    // void debugUsedBlocks();
    // void debugFreeBlocks();
};

MemoryAllocator *getMemoryAllocatorFromID(int partitionId);
MemoryAllocator *getMemoryAllocatorFromAddress(uint32_t address);
uint32_t allocateMemory(const std::string& name, int partitionId, int type, int size, uint32_t addr);
void freeMemory(uint32_t addr, int size);

SceUID sceKernelAllocPartitionMemory(SceUID partitionId, const char *name, int type, SceSize size, uint32_t addr);
int sceKernelFreePartitionMemory(SceUID blockId);
uint32_t sceKernelGetBlockHeadAddr(SceUID blockId);
uint32_t sceKernelMaxFreeMemSize();

int sceKernelCreateFpl(const char *name, int part, int attr, uint32_t size, uint32_t blocks, uint32_t optPtr);
int sceKernelAllocateFpl(SceUID uid, uint32_t dataPtr, uint32_t timeoutPtr);
int sceKernelAllocateFplCB(SceUID uid, uint32_t dataPtr, uint32_t timeoutPtr);
int sceKernelTryAllocateFpl(SceUID uid, uint32_t dataPtr);

void systemMemoryInit();
void systemMemoryReset();
void systemMemoryDestroy();

}