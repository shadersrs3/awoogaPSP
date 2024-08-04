#pragma once

#include <cstdint>
#include <vector>

namespace Core::Kernel {
struct Interrupt {
    uint32_t handler;
    uint32_t args[2];
    bool enabled;
    bool registered;
};

std::vector<Interrupt>& getPendingInterruptList();
void hleTriggerInterrupt(int intno, int subno = -1, uint32_t firstArgument = 0, bool enableArgument = false);
Interrupt *hleFindInterrupt(int intno, int subno);

int __hleInterruptsEnabled();
int sceKernelIsCpuIntrEnable();
int sceKernelCpuSuspendIntr();
int sceKernelCpuResumeIntr(uint32_t flags);
int sceKernelRegisterSubIntrHandler(int intno, int subno, uint32_t handler, uint32_t arg);
int sceKernelEnableSubIntr(int intno, int subno);
}