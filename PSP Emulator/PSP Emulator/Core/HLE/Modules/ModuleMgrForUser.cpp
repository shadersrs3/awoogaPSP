#include <Core/Kernel/sceKernelThread.h>

#include <Core/HLE/Modules/ModuleMgrForUser.h>

#include <Core/Loaders/AbstractLoader.h>
#include <Core/Loaders/ELFLoader.h>

#include <Core/Logger.h>

using namespace Core::Kernel;

namespace Core::HLE {
static const char *logType = "ModuleMgrForUser";

int sceKernelGetModuleIdByAddress(uint32_t address) {
    LOG_WARN(logType, "sceKernelGetModuleIdByAddress(0x%08x)", address);
    return 0xffff;
}

SceUID sceKernelLoadModule(const char *path, int flags, uint32_t optionPtr) {
    LOG_WARN(logType, "unimplemented sceKernelLoadModule(%s, 0x%08x, 0x%08x)", path, flags, optionPtr);
    return 0x400;
}

SceUID sceKernelLoadModuleByID(SceUID fid, int flags, uint32_t optionPtr) {
    LOG_WARN(logType, "unimplemented sceKernelLoadModuleByID(0x%06x, 0x%08x, 0x%08x)", fid, flags, optionPtr);
    return 0x400;
}

SceUID sceKernelGetModuleId() {
    LOG_SYSCALL(logType, "sceKernelGetModuleId() = 0x%06x", current->moduleId);
    return current->moduleId;
}

int sceKernelStartModule(SceUID modid, SceSize size, uint32_t argp, uint32_t statusPtr, uint32_t optionPtr) {
    LOG_WARN(logType, "unimplemented sceKernelStartModule(0x%06x, 0x%08x, 0x%08x)", modid, size, argp, statusPtr, optionPtr);
    return 0;
}

int sceKernelStopModule(SceUID modid, uint32_t size, uint32_t argp, uint32_t statusPtr, uint32_t optionPtr) {
    LOG_WARN(logType, "unimplemented sceKernelStopModule(0x%06x, 0x%08x, 0x%08x)", modid, size, argp, statusPtr, optionPtr);
    return 0;
}

int sceKernelUnloadModule(SceUID modid) {
    LOG_WARN(logType, "unimplemented sceKernelUnloadModule(0x%06x)", modid);
    return 0;
}
}