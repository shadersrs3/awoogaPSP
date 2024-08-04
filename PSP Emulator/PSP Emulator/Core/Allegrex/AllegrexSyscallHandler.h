#pragma once

#include <cstdint>

namespace Core::Allegrex {
void resetSyscallAddressHandler();
void setSyscallAddressHandler(uint32_t address, uint32_t nid);
bool handleSyscallFromAddress(uint32_t address);
}