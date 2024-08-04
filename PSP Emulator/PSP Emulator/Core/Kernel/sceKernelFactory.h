#pragma once

#include <cstdint>

#include <vector>

#include <Core/PSP/Types.h>
#include <Core/Kernel/sceKernelError.h>
#include <Core/Kernel/sceKernelObjectTypes.h>

namespace Core::Kernel {
struct KernelObject;

void kernelTraceLogCreation(const char *string, KernelObject *object);
void kernelDebugLog(const char *string, KernelObject *object);

template<class T>
T *createKernelObject(int *error = nullptr);

template<typename T>
T *getKernelObject(SceUID uid, int *error = nullptr);

void saveKernelObject(KernelObject *object);

template<typename T>
T *createWaitObject(int waitType);

void destroyKernelObject(SceUID uid, int *error = nullptr, KernelObjectType type = KERNEL_OBJECT_UNKNOWN);

bool isValidKernelObject(SceUID uid);

std::vector<int> *getKernelObjectList(KernelObjectType type);

bool initialize();
bool reset();
bool destroy();
}

#include <Core/Kernel/sceKernelTemplateInstantiation.h>