#include <map>
#include <sstream>

#include <Core/Kernel/sceKernelInterrupt.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelInterrupt";

static const char *PspInterruptNames[67] = { //67 interrupts
    0, 0, 0, 0, "GPIO", "ATA_ATAPI", "UmdMan", "MScm0",
    "Wlan", 0, "Audio", 0, "I2C", 0, "SIRCS_IrDA",
    "Systimer0", "Systimer1", "Systimer2", "Systimer3",
    "Thread0", "NAND", "DMACPLUS", "DMA0", "DMA1",
    "Memlmd", "GE", 0, 0, 0, 0, "Display", "MeCodec", 0,
    0, 0, 0, "HP_Remote", 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, "MScm1", "MScm2",
    0, 0, 0, "Thread1", "Interrupt"
};

static std::map<uint32_t, std::map<uint32_t, Interrupt>> interruptMap;
static bool interruptsEnabled = true;
std::vector<Interrupt> pendingInterrupts;

Interrupt *hleFindInterrupt(int intno, int subno) {
    auto it = interruptMap.find(intno);
    if (it != interruptMap.end()) {
        auto it2 = it->second.find(subno);
        if (it2 != it->second.end())
            return &it2->second;
    }
    return nullptr;
}

static std::string getInterruptTypeName(int index) {
    if (index < 0 || index >= 67) {
        return (std::stringstream() << "Unknown Interrupt " << index).str();
    }

    const char *intname = PspInterruptNames[index];
    return intname != nullptr ? intname : "Unknown Registered Interrupt";
}

static void addPendingInterrupt(const Interrupt& intr) {
    pendingInterrupts.push_back(intr);
}

std::vector<Interrupt>& getPendingInterruptList() {
    return pendingInterrupts;
}

void hleTriggerInterrupt(int intno, int subno, uint32_t firstArgument, bool enableArgument) {
    if (intno >= 0 && intno < 67 && subno == -1) { // all interrupts
        auto subnoMap = interruptMap.find(intno);
        if (subnoMap != interruptMap.end()) {
            for (auto i = subnoMap->second.begin(); i != subnoMap->second.end(); ++i) {
                if (i->second.enabled) {
                    if (enableArgument)
                        i->second.args[0] = firstArgument;
                    addPendingInterrupt(i->second);
                }
            }
        }
        return;
    }

    Interrupt *intr = hleFindInterrupt(intno, subno);
    if (intr) {
        if (intr->enabled) {
            if (enableArgument)
                intr->args[0] = firstArgument;

            addPendingInterrupt(*intr);
        }
    }
}

int __hleInterruptsEnabled() {
    return interruptsEnabled;
}

static void __setInterruptState(bool state) {
    interruptsEnabled = state;
}

int sceKernelIsCpuIntrEnable() {
    // LOG_WARN(logType, "unimplemented sceKernelIsCpuIntrEnable");
    return __hleInterruptsEnabled();
}

int sceKernelCpuSuspendIntr() {
    int rv;

    if (__hleInterruptsEnabled()) {
        rv = 1;
        __setInterruptState(false);
    } else {
        rv = 0;
    }

    // LOG_WARN(logType, "sceKernelCpuSuspendIntr");
    return rv;
}

int sceKernelCpuResumeIntr(uint32_t flags) {
    if (flags)
        __setInterruptState(true);
    // LOG_WARN(logType, "sceKernelCpuResumeIntr(0x%08x)", flags);
    return 0;
}

int sceKernelRegisterSubIntrHandler(int intno, int subno, uint32_t handler, uint32_t arg) {
    Interrupt i;
    std::string registeredInterruptName = getInterruptTypeName(intno);

    if (intno >= 67) {
        LOG_WARN(logType, "sceKernelRegisterSubIntrHandler(int: %d, sub: %d, handler: 0x%08x, arg: 0x%08x) registered interrupt %s", intno, subno, handler, arg, registeredInterruptName.c_str());
        return -1;
    }

    i.handler = handler;
    i.args[0] = subno;
    i.args[1] = arg;
    i.enabled = false;
    i.registered = true;

    interruptMap[intno][subno] = i;
    LOG_SYSCALL(logType, "sceKernelRegisterSubIntrHandler(int: %d, sub: %d, handler: 0x%08x, arg: 0x%08x) registered interrupt %s", intno, subno, handler, arg, registeredInterruptName.c_str());
    return 0;
}

int sceKernelEnableSubIntr(int intno, int subno) {
    if (intno >= 67) {
        LOG_ERROR(logType, "sceKernelEnableSubIntr(int: %d, sub: %d) unknown interrupt", intno, subno);
        return -1;
    }
    
    Interrupt *intr = hleFindInterrupt(intno, subno);
    if (intr && intr->registered) {
        intr->enabled = true;
        LOG_SYSCALL(logType, "sceKernelEnableSubIntr(int: %d, sub: %d)", intno, subno);
    } else {
        LOG_ERROR(logType, "sceKernelEnableSubIntr(int: %d, sub: %d) not registered interrupt", intno, subno);
    }
    return 0;
}
}