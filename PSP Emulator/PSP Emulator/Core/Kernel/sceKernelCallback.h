#pragma once

#include <Core/Kernel/Objects/Callback.h>

namespace Core::Kernel {
int sceKernelCreateCallback(const char *name, uint32_t handler, uint32_t arg);
int sceKernelCancelCallback(SceUID cb);
int sceKernelDeleteCallback(SceUID cb);
int sceKernelNotifyCallback(SceUID cb, int arg2);

void callbackInit();
void callbackReset();
void callbackDestroy();
}