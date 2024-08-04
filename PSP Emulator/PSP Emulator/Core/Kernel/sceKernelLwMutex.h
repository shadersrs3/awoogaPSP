#pragma once

#include <Core/Kernel/Objects/LwMutex.h>

namespace Core::Kernel {
int sceKernelCreateLwMutex(uint32_t workareaPtr, const char *name, uint32_t attr, int initialCount, uint32_t optPtr);
int sceKernelLockLwMutex(uint32_t workareaPtr, int lockCount, uint32_t timeoutPtr);
int sceKernelTryLockLwMutex(uint32_t workareaPtr, int lockCount);
int sceKernelUnlockLwMutex(uint32_t workareaPtr);
int sceKernelDeleteLwMutex(uint32_t workareaPtr);
}