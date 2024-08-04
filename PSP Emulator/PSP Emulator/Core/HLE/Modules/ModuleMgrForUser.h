#pragma once

#include <cstdint>

#include <Core/PSP/Types.h>

namespace Core::HLE {
int sceKernelGetModuleIdByAddress(uint32_t address);
SceUID sceKernelLoadModule(const char *path, int flags, uint32_t optionPtr);
SceUID sceKernelLoadModuleByID(SceUID fid, int flags, uint32_t optionPtr);
SceUID sceKernelGetModuleId();
int sceKernelStartModule(SceUID modid, SceSize size, uint32_t argp, uint32_t statusPtr, uint32_t optionPtr);
int sceKernelStopModule(SceUID modid, uint32_t size, uint32_t argp, uint32_t statusPtr, uint32_t optionPtr);
int sceKernelUnloadModule(SceUID modid);
}