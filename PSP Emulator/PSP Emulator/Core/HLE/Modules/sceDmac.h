#pragma once

#include <Core/PSP/Types.h>

namespace Core::HLE {
int sceDmacMemcpy(uint32_t dst, uint32_t src, SceSize n);
int sceDmacTryMemcpy(uint32_t dst, uint32_t src, SceSize n);
}