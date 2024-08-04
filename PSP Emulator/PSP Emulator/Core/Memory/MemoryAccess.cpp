#include <Core/Memory/MemoryAccess.h>

#include <Core/PSP/MemoryMap.h>
#include <Core/Logger.h>
#include <Core/Allegrex/AllegrexState.h>

namespace Core::Memory {
static const char *logType = "MemoryAccess";

extern uint8_t *scratchpad, *userMemory, *videoMemory, *kernelMemory;

namespace Utility {
bool isValidAddress(uint32_t address) {
    if (address >= 0x00010000 && address <= 0x00013FFF)
        return true;

    if (address >= 0x08800000 && address <= 0x08800000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1)
        return true;

    if (address >= 0x48800000 && address <= 0x48800000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1)
        return true;

    if (address >= 0x04000000 && address <= 0x043FFFFF)
        return true;

    if (address >= 0x44000000 && address <= 0x443FFFFF)
        return true;

    if (address >= 0x08000000 && address <= 0x087FFFFF)
        return true;

    if (address >= 0x88000000 && address <= 0x887FFFFF)
        return true;

    return false;
}

bool isValidAddressRange(uint32_t start, uint32_t end) {
    if (start >= 0x00010000 && end <= 0x00013FFF)
        return true;

    if (start >= 0x08800000 && end <= 0x08800000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1)
        return true;

    if (start >= 0x48800000 && end <= 0x48800000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1)
        return true;

    if (start >= 0x04000000 && end <= 0x043FFFFF)
        return true;

    if (start >= 0x44000000 && end <= 0x443FFFFF)
        return true;

    if (start >= 0x08000000 && end <= 0x087FFFFF)
        return true;

    if (start >= 0x88000000 && end <= 0x887FFFFF)
        return true;

    return false;
}

bool copyMemoryGuestToGuest(uint32_t dst, uint32_t src, uint32_t size) {
    if (!isValidAddressRange(src, src + size)) {
        LOG_ERROR(logType, "%s: invalid source address", __func__);
        return false;
    }

    if (!isValidAddressRange(dst, dst + size)) {
        LOG_ERROR(logType, "%s: invalid destination address", __func__);
        return false;
    }

    std::memcpy(getPointer(dst), getPointer(src), size);
    // LOG_TRACE(logType, "%s: [copied 0x%08x to 0x%08x, size 0x%08x]", __func__, src, dst, size);
    return true;
}

bool copyMemoryHostToGuest(uint32_t dst, const void *src, uint32_t size) {
    if (!isValidAddressRange(dst, dst + size)) {
        LOG_ERROR(logType, "%s: invalid destination address", __func__);
        return false;
    }

    std::memcpy(getPointer(dst), src, size);
    // LOG_TRACE(logType, "%s: [copied %p to 0x%08x, size 0x%08x]", __func__, src, dst, size);
    return true;
}

bool copyMemoryGuestToHost(void *dst, uint32_t src, uint32_t size) {
    if (!isValidAddressRange(src, src + size)) {
        LOG_ERROR(logType, "%s: invalid source address", __func__);
        return false;
    }

    std::memcpy(dst, getPointer(src), size);
    // LOG_TRACE(logType, "%s: [copied 0x%08x to %p, size 0x%08x]", __func__, src, dst, size);
    return true;
}

bool memorySet(uint32_t address, uint8_t c, uint32_t size) {
    if (!isValidAddressRange(address, address + size)) {
        LOG_ERROR(logType, "%s: invalid address", __func__);
        return false;
    }

    std::memset(getPointer(address), c, size);
    // LOG_TRACE(logType, "%s: [0x%08x set to 0x%02x, size 0x%08x]", __func__, address, c, size);
    return true;
}
}

void *getPointerUnchecked(uint32_t address) {
    if (address >= 0x00010000 && address <= 0x00013FFF)
        return scratchpad + (address - 0x00010000);

    if (address >= 0x08800000 && address <= 0x08800000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1)
        return userMemory + (address - 0x08800000);

    if (address >= 0x48800000 && address <= 0x48800000 + Core::PSP::USERSPACE_MEMORY_SIZE - 1)
        return userMemory + (address - 0x48800000);

    if (address >= 0x04000000 && address <= 0x043FFFFF)
        return videoMemory + (address - 0x04000000);

    if (address >= 0x44000000 && address <= 0x443FFFFF)
        return videoMemory + (address - 0x44000000);

    if (address >= 0x08000000 && address <= 0x087FFFFF)
        return kernelMemory + (address - 0x08000000);

    if (address >= 0x88000000 && address <= 0x887FFFFF)
        return kernelMemory + (address - 0x88000000);

    return nullptr;
}

void *getPointer(uint32_t address) {
    void *p = getPointerUnchecked(address);
    if (!p)
        LOG_ERROR(logType, "%s: Invalid PSP memory address 0x%08x", __func__, address);
    return p;
}

uint8_t read8(uint32_t address) {
    uint8_t *ptr = (uint8_t *)getPointer(address);
    if (ptr)
        return *ptr;
    return 0;
}

void write8(uint32_t address, uint8_t value) {
    uint8_t *ptr = (uint8_t *)getPointer(address);

    if (ptr)
        *ptr = value;
}

uint16_t read16(uint32_t address) {
    uint16_t *ptr = (uint16_t *)getPointer(address);
    if (ptr)
        return *ptr;
    return 0;
}

void write16(uint32_t address, uint16_t value) {
    uint16_t *ptr = (uint16_t *)getPointer(address);

    if (ptr)
        *ptr = value;
}

uint32_t read32(uint32_t address) {
    uint32_t *ptr = (uint32_t *)getPointer(address);
    if (ptr)
        return *ptr;
    return 0;
}

void write32(uint32_t address, uint32_t value) {
    uint32_t *ptr = (uint32_t *)getPointer(address);

    if (ptr)
        *ptr = value;
}

float readFloat32(uint32_t address) {
    float *ptr = (float *)getPointer(address);

    if (ptr)
        return *ptr;

    printf("invalid float read\n");
    return 0.f;
}

void writeFloat32(uint32_t address, float value) {
    float *ptr = (float *)getPointer(address);

    if (ptr) {
        *ptr = value;
        return;
    }

    printf("invalid float write\n");
}

}