#pragma once

#include <cstdint>

namespace Core::Allegrex {
typedef void (*InterpreterFunction)(uint32_t);
void _executeVFPU(uint32_t opcode);
}