#pragma once

#include <cstdint>

namespace Core::Memory {
namespace Utility {
bool isValidAddress(uint32_t address);
bool isValidAddressRange(uint32_t start, uint32_t end);
bool copyMemoryGuestToGuest(uint32_t dst, uint32_t src, uint32_t size);
bool copyMemoryHostToGuest(uint32_t dst, const void *src, uint32_t size);
bool copyMemoryGuestToHost(void *dst, uint32_t src, uint32_t size);
bool memorySet(uint32_t address, uint8_t c, uint32_t size);
}

void *getPointerUnchecked(uint32_t address);
void *getPointer(uint32_t address);
void write8(uint32_t address, uint8_t value);
uint8_t read8(uint32_t address);
void write16(uint32_t address, uint16_t value);
uint16_t read16(uint32_t address);
void write32(uint32_t address, uint32_t value);
uint32_t read32(uint32_t address);
float readFloat32(uint32_t address);
void writeFloat32(uint32_t address, float value);
}