#pragma once

#include <cstdint>
#include <source_location>

namespace Core::Allegrex {
typedef uint64_t CyclesTaken;

void setProcessorFailed(bool state);
bool& isProcessorFailed();
void setDebugMode(bool state);
uint64_t interpreterBlockStep();
bool interpreterStep();
}