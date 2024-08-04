#pragma once

#include <cstdint>

namespace Core::Memory {
bool initialize();
void reset();
void destroy();

void *allocateMemory(size_t memorySize, bool executable = false);
void deallocateMemory(void *ptr, size_t memorySize);
}