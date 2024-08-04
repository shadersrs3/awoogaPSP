#pragma once

#include <cstdint>

namespace Core::HLE::Assembly {
uint32_t makeJumpReturnAddress();
uint32_t makeSyscallInstruction(uint32_t nid);
uint32_t makeBranch(uint16_t offset);
uint32_t makeNop();
}