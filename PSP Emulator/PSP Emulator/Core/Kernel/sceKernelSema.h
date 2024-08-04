#pragma once

#include <Core/Kernel/Objects/Semaphore.h>

namespace Core::Kernel {
SceUID sceKernelCreateSema(const char *name, SceUInt attr, int initVal, int maxVal, uint32_t optionPtr);
int sceKernelDeleteSema(int semaid);
int sceKernelPollSema(SceUID semaid, int signal);
int sceKernelSignalSema(SceUID semaid, int signal);
int sceKernelWaitSema(SceUID semaid, int signal, uint32_t timeoutPtr);
int sceKernelWaitSemaCB(SceUID semaid, int signal, uint32_t timeoutPtr);
int sceKernelReferSemaStatus(SceUID semaid, uint32_t infoPtr);
}