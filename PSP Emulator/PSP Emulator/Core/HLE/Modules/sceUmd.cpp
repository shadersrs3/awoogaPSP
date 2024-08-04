#include <Core/HLE/Modules/sceUmd.h>

#include <Core/Allegrex/AllegrexState.h>
#include <Core/Kernel/sceKernelError.h>
#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelCallback.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

using namespace Core::Kernel;
using namespace Core::Allegrex;

namespace Core::HLE {
static const char *logType = "sceUmd";

enum { 
    PSP_UMD_INIT        = 0x00,
    PSP_UMD_NOT_PRESENT = 0x01,
    PSP_UMD_PRESENT     = 0x02,
    PSP_UMD_CHANGED     = 0x04,
    PSP_UMD_NOT_READY   = 0x08,
    PSP_UMD_READY       = 0x10, 
    PSP_UMD_READABLE    = 0x20,
};

enum {
    SCE_UMD_ERROR_NOT_READY     = 0x80210001,
    SCE_UMD_ERROR_INVALID_PARAM = 0x80010016,
};

enum {
    PSP_UMD_TYPE_GAME  = 0x10,
    PSP_UMD_TYPE_VIDEO = 0x20,
    PSP_UMD_TYPE_AUDIO = 0x40,
};

static constexpr uint32_t UMD_STAT_ALLOW_WAIT = PSP_UMD_NOT_PRESENT | PSP_UMD_PRESENT | PSP_UMD_NOT_READY | PSP_UMD_READY | PSP_UMD_READABLE;

static uint32_t UMD_CONFIG_STAT = PSP_UMD_READABLE | PSP_UMD_PRESENT | PSP_UMD_READY;

static bool umdActivated = true;

static int umdCallbackID = -1;

Core::Kernel::Callback *__getRegisteredUMDCallback() {
    Callback *c = getKernelObject<Callback>(umdCallbackID);
    if (!c)
        return nullptr;

    c->notifyArg = UMD_CONFIG_STAT;
    c->notifyCount = 1;

    if (umdActivated)
        umdActivated = false;
    else
        c = nullptr;
    return c;
}

int sceUmdActivate(int unit, uint32_t drivePtr) {
    const char *name = (const char *) Core::Memory::getPointerUnchecked(drivePtr);
    if (unit < 1 || unit > 2) {
        LOG_ERROR(logType, "invalid UMD unit %d", unit);
        return SCE_UMD_ERROR_INVALID_PARAM;
    }

    LOG_SYSCALL(logType, "sceUmdActivate(%d, %s)", unit, name);
    umdActivated = true;
    return 0;
}

int sceUmdCancelWaitDriveStat() {
    LOG_WARN(logType, "unimplemented sceUmdCancelWaitDriveStat");
    return 0;
}

int sceUmdCheckMedium() {
    LOG_WARN(logType, "unimplemented sceUmdCheckMedium");
    return 1;
}

int sceUmdDeactivate(int unit, uint32_t drivePtr) {
    return 0;
}

int sceUmdGetDiscInfo(uint32_t infoPtr) {
    return 0;
}

int sceUmdGetDriveStat() {
    uint32_t umdStatus = UMD_CONFIG_STAT;

    if (umdActivated)
        umdStatus |= PSP_UMD_READABLE;

    LOG_SYSCALL(logType, "sceUmdGetDriveStat() %02x", umdStatus);
    return umdStatus;
}

int sceUmdGetErrorStat() {
    LOG_SYSCALL(logType, "sceUmdGetErrorStat()");
    return 0;
}

int sceUmdRegisterUMDCallBack(int cbid) {
    Callback *c = getKernelObject<Callback>(cbid);
    uint32_t ret = 0;

    if (!c)
        ret = 0x80010016;
    else
        umdCallbackID = cbid;
    LOG_SYSCALL(logType, "sceUmdRegisterUMDCallBack(0x%06x) ret 0x%08x", cbid, ret);
    return ret;
}

int sceUmdReplacePermit() {
    return 0;
}

int sceUmdReplaceProhibit() {
    return 0;
}

int sceUmdUnRegisterUMDCallBack(int cbid) {
    return 0;
}

static int __umdWaitDriveStat(int stat, bool handleCallbacks = false) {
    if ((stat & UMD_STAT_ALLOW_WAIT) == 0)
        return SCE_KERNEL_ERROR_ERRNO_INVALID_ARGUMENT;

    if (handleCallbacks)
        hleCurrentThreadEnableCallbackState();

    Core::Timing::consumeCycles(590);
    return 0;
}

int sceUmdWaitDriveStat(int stat) {
    int error = __umdWaitDriveStat(stat);
    LOG_SYSCALL(logType, "sceUmdWaitDriveStat(%d) error 0x%08x", stat, error);
    return error;
}

int sceUmdWaitDriveStatCB(int stat, uint32_t timeoutPtr) {
    int error = __umdWaitDriveStat(stat, true);
    LOG_SYSCALL(logType, "sceUmdWaitDriveStatCB(%d, 0x%08x) error 0x%08x", stat, timeoutPtr, error);
    return error;
}

int sceUmdWaitDriveStatWithTimer(int stat, uint32_t timeout) {
    return 0;
}
}