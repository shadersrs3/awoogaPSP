#pragma once

#include <cstdint>

namespace Core::PSP {
static constexpr uint32_t SCRATCHPAD_MEMORY_SIZE = 0x4000;
static constexpr uint32_t KERNELSPACE_MEMORY_SIZE = 0x800000;
static constexpr uint32_t USERSPACE_MEMORY_SIZE_2 = 0x1800000;
static constexpr uint32_t USERSPACE_MEMORY_SIZE = 0x3800000;
static constexpr uint32_t VRAM_MEMORY_SIZE = 0x400000;
}