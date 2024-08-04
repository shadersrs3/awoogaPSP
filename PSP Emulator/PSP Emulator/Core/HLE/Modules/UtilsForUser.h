#pragma once

#include <cstdint>

namespace Core::HLE {
uint32_t sceKernelLibcGettimeofday(uint32_t tpPtr, uint32_t tzpPtr);
uint32_t sceKernelLibcTime(uint32_t timePtr);
int sceKernelDcacheWritebackAll();
int sceKernelDcacheWritebackRange(uint32_t ptr, uint32_t size);
}