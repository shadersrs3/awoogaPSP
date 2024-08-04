#include <unordered_map>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Allegrex/AllegrexState.h>
#include <Core/Allegrex/AllegrexSyscallHandler.h>
#include <Core/Allegrex/CPURegisterName.h>

#include <Core/HLE/CustomSyscall.h>
#include <Core/HLE/Modules/StdioForUser.h>
#include <Core/HLE/Modules/IoFileMgrForUser.h>
#include <Core/HLE/Modules/UtilsForUser.h>
#include <Core/HLE/Modules/ModuleMgrForUser.h>
#include <Core/HLE/Modules/sceCtrl.h>
#include <Core/HLE/Modules/sceUmd.h>
#include <Core/HLE/Modules/scePower.h>
#include <Core/HLE/Modules/sceUtility.h>
#include <Core/HLE/Modules/sceRtc.h>
#include <Core/HLE/Modules/sceDmac.h>
#include <Core/HLE/Modules/sceAtrac3plus.h>

#include <Core/GPU/GE.h>
#include <Core/GPU/sceDisplay.h>

#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelEventFlag.h>
#include <Core/Kernel/sceKernelSema.h>
#include <Core/Kernel/sceKernelCallback.h>
#include <Core/Kernel/sceKernelLwMutex.h>
#include <Core/Kernel/sceKernelSystemMemory.h>
#include <Core/Kernel/sceKernelAlarm.h>

#include <Core/Kernel/sceKernelInterrupt.h>

#include <Core/Audio/sceAudio.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

using namespace Core::Kernel;
using namespace Core::HLE;
using namespace Core::GPU;
using namespace Core::Audio;

#define reg cpu.reg
extern bool kernelTrace;

namespace Core::Allegrex {
static const char *logType = "AllegrexSyscallHandler";
static std::unordered_map<uint32_t, uint32_t> hleAddressHandler;

void resetSyscallAddressHandler() {
    hleAddressHandler.clear();
}

void setSyscallAddressHandler(uint32_t address, uint32_t nid) {
    hleAddressHandler[address] = nid;
}

bool handleSyscallFromAddress(uint32_t address) {
    auto i = hleAddressHandler.find(address);

    if (i == hleAddressHandler.end()) {
        LOG_ERROR(logType, "can't find HLE function at address 0x%08x!", address);
        return false;
    }

    uint32_t nid = i->second;
    if (nid != 0xE0D68148 && nid != 0x71EC4271 &&
        nid != 0x79D1C3FA && nid != 0x1F803938 &&
        nid != 0x00001000 && nid != 0xb287bd61 &&
        nid != 0x289D82FE && nid != 0xAB49E76A &&
        nid != 0x984C27E7 && nid != 0xeadb1bd7 && nid != 0x3aee7261 && nid != 0x68da9e36) {
        // printf("syscall 0x%08x %s 0x%08x\n", nid, current->name, reg[MIPS_REG_RA]);
    }

    switch (nid) {
    // -- IoFileMgrForUser --

    case 0x42EC03AC: // sceIoWrite
    {
        reg[MIPS_REG_V0] = sceIoWrite(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x54F5FB11: // sceIoDevctl
    {
        reg[MIPS_REG_V0] = sceIoDevctl((const char *) Core::Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4], reg[MIPS_REG_A5]);
        return true;
    }
    case 0x6A638D83: // sceIoRead
    {
        reg[MIPS_REG_V0] = sceIoRead(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x71B19E77: // sceIoLseekAsync
    {
        reg[MIPS_REG_V0] = sceIoLseekAsync(reg[MIPS_REG_A0], (uint64_t) reg[MIPS_REG_A2] | (uint64_t)reg[MIPS_REG_A3] << 32, reg[MIPS_REG_A4]);
        return true;
    }
    case 0xB293727F: // sceIoChangeAsyncPriority 
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0x779103A0: // sceIoRename
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x810C4BC3: // sceIoClose
    {
        reg[MIPS_REG_V0] = sceIoClose(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xFF5940B6: // sceIoCloseAsync
    {
        reg[MIPS_REG_V0] = sceIoCloseAsync(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x89AA9906: // sceIoOpenAsync
    {
        reg[MIPS_REG_V0] = sceIoOpenAsync((const char *) Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xA0B5A7C2: // sceIoReadAsync
    {
        reg[MIPS_REG_V0] = sceIoReadAsync(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x109F50BC: // sceIoOpen
    {
        reg[MIPS_REG_V0] = sceIoOpen((const char *) Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x27EB27B8: // sceIoLseek
    {
        SceOff pos = sceIoLseek(reg[MIPS_REG_A0], (uint64_t) reg[MIPS_REG_A2] | (uint64_t)reg[MIPS_REG_A3] << 32, reg[MIPS_REG_A4]);
        reg[MIPS_REG_V0] = uint32_t(pos);
        reg[MIPS_REG_V1] = uint32_t(pos >> 32);
        return true;
    }
    case 0x68963324: // sceIoLseek32
    {
        reg[MIPS_REG_V0] = sceIoLseek32(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x1B385D8F: // sceIoLseek32Async
    {
        reg[MIPS_REG_V0] = sceIoLseek32Async(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xACE946E8: // sceIoGetstat
    {
        reg[MIPS_REG_V0] = sceIoGetstat((const char *) Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1]);
        return true;
    }
    case 0x3251EA56: // sceIoPollAsync
    {
        reg[MIPS_REG_V0] = sceIoPollAsync(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x35DBD746: // sceIoWaitAsyncCB
    {
        reg[MIPS_REG_V0] = sceIoWaitAsyncCB(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x55F4717D: // sceIoChdir
    {
        reg[MIPS_REG_V0] = sceIoChdir((const char *) Memory::getPointerUnchecked(reg[MIPS_REG_A0]));
        return true;
    }
    case 0xB29DDF9C: // sceIoDopen
    {
        reg[MIPS_REG_V0] = sceIoDopen((const char *) Memory::getPointerUnchecked(reg[MIPS_REG_A0]));
        return true;
    }
    case 0xE3EB004C: // sceIoDread
    {
        reg[MIPS_REG_V0] = sceIoDread(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xEB092469: // sceIoDclose
    {
        reg[MIPS_REG_V0] = sceIoDclose(reg[MIPS_REG_A0]);
        return true;
    }

    // -- Kernel_Library --

    case 0xB55249D2: // sceKernelIsCpuIntrEnable
    {
        reg[MIPS_REG_V0] = sceKernelIsCpuIntrEnable();
        return true;
    }
    case 0x092968F4: // sceKernelCpuSuspendIntr
    {
        reg[MIPS_REG_V0] = sceKernelCpuSuspendIntr();
        return true;
    }
    case 0x5F10D406: // sceKernelCpuResumeIntr
    {
        reg[MIPS_REG_V0] = sceKernelCpuResumeIntr(reg[MIPS_REG_A0]);
        return true;
    }

    // -- InterruptManager --

    case 0xCA04A2B9: // sceKernelRegisterSubIntrHandler
    {
        reg[MIPS_REG_V0] = sceKernelRegisterSubIntrHandler(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3]);
        return true;
    }
    case 0xD61E6961: // sceKernelReleaseSubIntrHandler
    {
        reg[MIPS_REG_V0] = 0;
        LOG_WARN(logType, "unimplemented sceKernelReleaseSubIntrHandler");
        break;
    }
    case 0xFB8E22EC: // sceKernelEnableSubIntr
    {
        reg[MIPS_REG_V0] = sceKernelEnableSubIntr(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }

    // -- LoadExecForUser --

    case 0x05572A5F: // sceKernelExitGame
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x4AC57943: // sceKernelRegisterExitCallback
    {
        reg[MIPS_REG_V0] = 0;
        LOG_WARN(logType, "unimplemented sceKernelRegisterExitCallback");
        return true;
    }


    // -- ModuleMgrForUser --

    case 0xD8B73127: // sceKernelGetModuleIdByAddress
    {
        reg[MIPS_REG_V0] = sceKernelGetModuleIdByAddress(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xF0A26395: // sceKernelGetModuleId
    {
        reg[MIPS_REG_V0] = sceKernelGetModuleId();
        return true;
    }
    case 0x977DE386: // sceKernelLoadModule
    {
        reg[MIPS_REG_V0] = sceKernelLoadModule((const char *)Core::Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xB7F46618: // sceKernelLoadModuleByID
    {
        reg[MIPS_REG_V0] = sceKernelLoadModuleByID(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x8F2DF740: // sceKernelStopUnloadSelfModuleWithStatus
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x50F0C1EC: // sceKernelStartModule
    {
        reg[MIPS_REG_V0] = sceKernelStartModule(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0xD1FF982A: // sceKernelStopModule
    {
        reg[MIPS_REG_V0] = sceKernelStopModule(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0x2E0911AA: // sceKernelUnloadModule
    {
        reg[MIPS_REG_V0] = sceKernelUnloadModule(reg[MIPS_REG_A0]);
        return true;
    }

    // -- StdioForUser --

    case 0x172D316E: // sceKernelStdin
    {
        reg[MIPS_REG_V0] = sceKernelStdin();
        return true;
    }
    case 0xA6BAB2E9: // sceKernelStdout
    {
        reg[MIPS_REG_V0] = sceKernelStdout();
        return true;
    }
    case 0xF78BA90A: // sceKernelStderr
    {
        reg[MIPS_REG_V0] = sceKernelStderr();
        return true;
    }


    // -- SysMemUserForUser --

    case 0xA291F107: // sceKernelMaxFreeMemSize
    {
        reg[MIPS_REG_V0] = sceKernelMaxFreeMemSize();
        return true;
    }

    case 0x13A5ABEF: // sceKernelPrintf
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x237DBD4F: // sceKernelAllocPartitionMemory
    {
        reg[MIPS_REG_V0] = sceKernelAllocPartitionMemory(reg[MIPS_REG_A0], (const char *)Memory::getPointerUnchecked(reg[MIPS_REG_A1]), reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0x7591C7DB: // sceKernelSetCompiledSdkVersion
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x91DE343C: // sceKernelSetCompiledSdkVersion500_505
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x342061E5: // sceKernelSetCompiledSdkVersion370
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x9D9A5BA1: // sceKernelGetBlockHeadAddr
    {
        reg[MIPS_REG_V0] = sceKernelGetBlockHeadAddr(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xB6D61D02: // sceKernelFreePartitionMemory
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xF77D77CB: // sceKernelSetCompilerVersion
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }


    // -- ThreadManForUser --
    case 0x52089CA1: // sceKernelGetThreadStackFreeSize
    {
        reg[MIPS_REG_V0] = sceKernelGetThreadStackFreeSize(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x349D6D6C: // sceKernelCheckCallback 
    {
        reg[MIPS_REG_A0] = sceKernelCheckCallback();
        return true;
    }
    case 0x6652B8CA: // sceKernelSetAlarm
    {
        reg[MIPS_REG_V0] = sceKernelSetAlarm(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x3B183E26: // sceKernelGetThreadExitStatus
    {
        reg[MIPS_REG_V0] = sceKernelGetThreadExitStatus(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x19CFF145: // sceKernelCreateLwMutex
    {
        reg[MIPS_REG_V0] = sceKernelCreateLwMutex(reg[MIPS_REG_A0], (const char *)Memory::getPointerUnchecked(reg[MIPS_REG_A1]), reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0xBEA46419: // sceKernelLockLwMutex
    {
        reg[MIPS_REG_V0] = sceKernelLockLwMutex(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x15B6446B: // sceKernelUnlockLwMutex
    {
        reg[MIPS_REG_V0] = sceKernelUnlockLwMutex(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x94AA61EE: // sceKernelGetThreadCurrentPriority 
    {
        reg[MIPS_REG_V0] = sceKernelGetThreadCurrentPriority();
        return true;
    }
    case 0x912354A7: // sceKernelRotateThreadReadyQueue
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0xA14F40B2: // sceKernelVolatileMemTryLock
    {
        reg[MIPS_REG_V0] = sceKernelVolatileMemTryLock(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x3E0271D3: // sceKernelVolatileMemLock
    {
        reg[MIPS_REG_V0] = sceKernelVolatileMemLock(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xA569E425: // sceKernelVolatileMemUnlock
    {
        reg[MIPS_REG_V0] = sceKernelVolatileMemUnlock(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xC07BB470: // sceKernelCreateFpl
    {
        reg[MIPS_REG_V0] = sceKernelCreateFpl((const char *)Memory::getPointer(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4], reg[MIPS_REG_A5]);
        return true;
    }
    case 0xD979E9BF: // sceKernelAllocateFpl
    {
        reg[MIPS_REG_V0] = sceKernelAllocateFpl(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xE7282CB6: // sceKernelAllocateFplCB
    {
        reg[MIPS_REG_V0] = sceKernelAllocateFplCB(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x623AE665: // sceKernelTryAllocateFpl
    {
        reg[MIPS_REG_V0] = sceKernelTryAllocateFpl(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xEFD3C963: // sceKernelPowerTick
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x17C1684E: // sceKernelReferThreadStatus
    {
        reg[MIPS_REG_V0] = sceKernelReferThreadStatus(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x64D4540E: // sceKernelReferThreadProfiler
    {
        reg[MIPS_REG_V0] = sceKernelReferThreadProfiler();
        return true;
    }
    case 0x8218B4DD: // sceKernelReferGlobalProfiler
    {
        reg[MIPS_REG_V0] = sceKernelReferGlobalProfiler();
        return true;
    }
    case 0xA66B0120: // sceKernelReferEventFlagStatus
    {
        reg[MIPS_REG_V0] = sceKernelReferEventFlagStatus(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xCEADEB47: // sceKernelDelayThread
    {
        reg[MIPS_REG_V0] = sceKernelDelayThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xBD123D9E: // sceKernelDelaySysClockThread
    {
        reg[MIPS_REG_V0] = sceKernelDelaySysClockThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x3AD58B8C: // sceKernelSuspendDispatchThread
    {
        reg[MIPS_REG_V0] = sceKernelSuspendDispatchThread();
        return true;
    }
    case 0x27E22EC2: // sceKernelResumeDispatchThread
    {
        reg[MIPS_REG_V0] = sceKernelResumeDispatchThread();
        return true;
    }
    case 0x1FB15A32: // sceKernelSetEventFlag
    {
        reg[MIPS_REG_V0] = sceKernelSetEventFlag(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xD6DA4BA1: // sceKernelCreateSema
    {
        reg[MIPS_REG_V0] = sceKernelCreateSema((const char *)Core::Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0xE81CAF8F: // sceKernelCreateCallback
    {
        reg[MIPS_REG_V0] = sceKernelCreateCallback((const char *)Core::Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xD59EAD2F: // sceKernelWakeupThread
    {
        reg[MIPS_REG_V0] = sceKernelWakeupThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xFCCFAD26: // sceKernelCancelWakeupThread
    {
        reg[MIPS_REG_V0] = sceKernelCancelWakeupThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xEA748E31: // sceKernelChangeCurrentThreadAttr
    {
        reg[MIPS_REG_V0] = sceKernelChangeCurrentThreadAttr(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xEDBA5844: // sceKernelDeleteCallback
    {
        reg[MIPS_REG_V0] = sceKernelDeleteCallback(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xEF9E4C70: // sceKernelDeleteEventFlag
    {
        reg[MIPS_REG_V0] = sceKernelDeleteEventFlag(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xF475845D: // sceKernelStartThread
    {
        reg[MIPS_REG_V0] = sceKernelStartThread(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x278C0DF5: // sceKernelWaitThreadEnd
    {
        reg[MIPS_REG_V0] = sceKernelWaitThreadEnd(reg[MIPS_REG_V0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x28B6489C: // sceKernelDeleteSema
    {
        reg[MIPS_REG_V0] = sceKernelDeleteSema(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x293B45B8: // sceKernelGetThreadId
    {
        reg[MIPS_REG_V0] = sceKernelGetThreadId();
        return true;
    }
    case 0xDB738F35: // sceKernelGetSystemTime
    {
        reg[MIPS_REG_V0] = sceKernelGetSystemTime(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x82BC5777: // sceKernelGetSystemTimeWide
    {
        uint64_t result = sceKernelGetSystemTimeWide();
        reg[MIPS_REG_V0] = (uint32_t)(result);
        reg[MIPS_REG_V1] = (uint32_t)(result >> 32);
        return true;
    }
    case 0x369ED59D: // sceKernelGetSystemTimeLow
    {
        reg[MIPS_REG_V0] = sceKernelGetSystemTimeLow();
        return true;
    }
    case 0xBA6B92E2: // sceKernelSysClock2USec
    {
        reg[MIPS_REG_V0] = sceKernelSysClock2USec(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xE1619D7C: // sceKernelSysClock2USecWide
    {
        reg[MIPS_REG_V0] = sceKernelSysClock2USecWide((uint64_t)reg[MIPS_REG_A0] | (uint64_t)reg[MIPS_REG_A1] << 32, reg[MIPS_REG_A2], reg[MIPS_REG_A3]);
        return true;
    }
    case 0x9944F31F: // sceKernelSuspendThread
    {
        reg[MIPS_REG_V0] = sceKernelSuspendThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x616403BA: // sceKernelTerminateThread
    {
        reg[MIPS_REG_V0] = sceKernelTerminateThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x383F7BCC: // sceKernelTerminateDeleteThread
    {
        reg[MIPS_REG_V0] = sceKernelTerminateDeleteThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x809CE29B: // sceKernelExitDeleteThread
    {
        reg[MIPS_REG_V0] = sceKernelExitDeleteThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x3F53E640: // sceKernelSignalSema
    {
        reg[MIPS_REG_V0] = sceKernelSignalSema(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x402FCF22: // sceKernelWaitEventFlag
    {
        reg[MIPS_REG_V0] = sceKernelWaitEventFlag(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0x328C546A: // sceKernelWaitEventFlagCB
    {
        reg[MIPS_REG_V0] = sceKernelWaitEventFlagCB(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0x30FD48F0: // sceKernelPollEventFlag
    {
        reg[MIPS_REG_V0] = sceKernelPollEventFlag(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3]);
        return true;
    }
    case 0x446D8DE6: // sceKernelCreateThread
    {
        reg[MIPS_REG_V0] = sceKernelCreateThread((const char *) Memory::getPointerUnchecked(reg[MIPS_REG_A0]),
            reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4], reg[MIPS_REG_A5], current->moduleId);
        return true;
    }
    case 0x4E3A1105: // sceKernelWaitSema
    {
        reg[MIPS_REG_V0] = sceKernelWaitSema(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x6D212BAC: // sceKernelWaitSemaCB
    {
        reg[MIPS_REG_V0] = sceKernelWaitSemaCB(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x55C20A00: // sceKernelCreateEventFlag
    {
        reg[MIPS_REG_V0] = sceKernelCreateEventFlag((const char *) Core::Memory::getPointerUnchecked(reg[MIPS_REG_A0]), reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3]);
        return true;
    }
    case 0x58B1F937: // sceKernelPollSema
    {
        reg[MIPS_REG_V0] = sceKernelPollSema(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x68DA9E36: // sceKernelDelayThreadCB
    {
        reg[MIPS_REG_V0] = sceKernelDelayThreadCB(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x71BC9871: // sceKernelChangeThreadPriority
    {
        reg[MIPS_REG_V0] = sceKernelChangeThreadPriority(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x812346E4: // sceKernelClearEventFlag
    {
        reg[MIPS_REG_V0] = sceKernelClearEventFlag(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x9FA03CD3: // sceKernelDeleteThread
    {
        reg[MIPS_REG_V0] = sceKernelDeleteThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xAA73C935: // sceKernelExitThread
    {
        reg[MIPS_REG_V0] = sceKernelExitThread(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x9ACE131E: // sceKernelSleepThreadCB
    {
        reg[MIPS_REG_V0] = sceKernelSleepThread();
        return true;
    }
    case 0x82826F70: // sceKernelSleepThreadCB
    {
        reg[MIPS_REG_V0] = sceKernelSleepThreadCB();
        return true;
    }

    // -- UtilsForUser --

    case 0x71EC4271: // sceKernelLibcGettimeofday
    {
        reg[MIPS_REG_V0] = sceKernelLibcGettimeofday(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xBFA98062: // sceKernelDcacheInvalidateRange
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x79D1C3FA: // sceKernelDcacheWritebackAll
    {
        reg[MIPS_REG_V0] = sceKernelDcacheWritebackAll();
        return true;
    }
    case 0xB435DEC5: // sceKernelDcacheWritebackInvalidateAll
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x34B9FA9E: // sceKernelDcacheWritebackInvalidateRange 
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x91E4F6A7: // sceKernelLibcClock
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x27CC57F0: // sceKernelLibcTime
    {
        reg[MIPS_REG_V0] = sceKernelLibcTime(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x3EE30821: // sceKernelDcacheWritebackRange
    {
        reg[MIPS_REG_V0] = sceKernelDcacheWritebackRange(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }

    // -- sceSuspendForUser --

    case 0x090CCB3F: // sceKernelPowerTick
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x3AEE7261: // sceKernelPowerUnlock
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xEADB1BD7: // sceKernelPowerLock
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }


    // -- sceGe_user --

    case 0x03444EB4: // sceGeListSync
    {
        reg[MIPS_REG_V0] = sceGeListSync(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x05DB22CE: // sceGeUnsetCallback
    {
        reg[MIPS_REG_V0] = sceGeUnsetCallback(reg[MIPS_REG_A0]);
        break;
    }
    case 0x1C0D95A6: // sceGeListEnQueueHead
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x1F6752AD: // sceGeEdramGetSize
    {
        reg[MIPS_REG_V0] = sceGeEdramGetSize();
        return true;
    }
    case 0x4C06E472: // sceGeContinue
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xA4FC06A4: // sceGeSetCallback
    {
        reg[MIPS_REG_V0] = sceGeSetCallback(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xAB49E76A: // sceGeListEnQueue
    {
        reg[MIPS_REG_V0] = sceGeListEnQueue(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3]);
        return true;
    }
    case 0xB287BD61: // sceGeDrawSync
    {
        reg[MIPS_REG_V0] = sceGeDrawSync(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xB448EC0D: // sceGeBreak
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xE0D68148: // sceGeListUpdateStallAddr
    {
        reg[MIPS_REG_V0] = sceGeListUpdateStallAddr(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xE47E40E4: // sceGeEdramGetAddr
    {
        reg[MIPS_REG_V0] = sceGeEdramGetAddr();
        return true;
    }


    // -- sceDisplay --

    case 0x0E20F177: // sceDisplaySetMode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x289D82FE: // sceDisplaySetFrameBuf
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xEEDA2E54: // sceDisplayGetFrameBuf
    {
        reg[MIPS_REG_V0] = 0;
        LOG_WARN(logType, "unimplemented sceDisplayGetFrameBuf");
        return true;
    }
    case 0x8EB9EC49: // sceDisplayWaitVblankCB
    case 0x46F186C3: // sceDisplayWaitVblankStartCB
    {
        reg[MIPS_REG_V0] = sceDisplayWaitVblankStartCB();
        return true;
    }
    case 0x984C27E7: // sceDisplayWaitVblankStart
    {
        reg[MIPS_REG_V0] = sceDisplayWaitVblankStart();
        return true;
    }
    case 0x36CDFADE: // sceDisplayWaitVblank
    {
        reg[MIPS_REG_V0] = sceDisplayWaitVblank();
        return true;
    }
    case 0x4D4E10EC: // sceDisplayIsVblank
    {
        reg[MIPS_REG_V0] = sceDisplayIsVblank();
        return true;
    }
    case 0x773DD3A3: // sceDisplayGetCurrentHcount
    {
        reg[MIPS_REG_V0] = (Core::Timing::getCurrentCycles() % 3704040) / 272; // unimplemented
        return true;
    }
    case 0x9C6EAAD7: // sceDisplayGetVcount
    {
        reg[MIPS_REG_V0] = (Core::Timing::getCurrentCycles() % 3704040) / 480; // unimplemented
        return true;
    }


    // -- sceDmac --
    case 0x617F3FE6: // sceDmacMemcpy
    {
        reg[MIPS_REG_V0] = sceDmacMemcpy(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }

    // -- sceCtrl --

    case 0x1F4011E6: // sceCtrlSetSamplingMode
    {
        reg[MIPS_REG_V0] = sceCtrlSetSamplingMode(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x1F803938: // sceCtrlReadBufferPositive
    {
        reg[MIPS_REG_V0] = sceCtrlReadBufferPositive(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x6A2774F3: // sceCtrlSetSamplingCycle
    {
        reg[MIPS_REG_V0] = sceCtrlSetSamplingCycle(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x3A622550: // sceCtrlPeekBufferPositive
    {
        reg[MIPS_REG_V0] = sceCtrlPeekBufferPositive(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }


    // -- sceUmdUser --

    case 0x20628E6F: // sceUmdGetErrorStat
    {
        reg[MIPS_REG_V0] = sceUmdGetErrorStat();
        return true;
    }
    case 0x6B4A146C: // sceUmdGetDriveStat
    {
        reg[MIPS_REG_V0] = sceUmdGetDriveStat();
        return true;
    }
    case 0x46EBB729: // sceUmdCheckMedium
    {
        reg[MIPS_REG_V0] = sceUmdCheckMedium();
        return true;
    }
    case 0x6AF9B50A: // sceUmdCancelWaitDriveStat
    {
        reg[MIPS_REG_V0] = sceUmdCancelWaitDriveStat();
        return true;
    }
    case 0x8EF08FCE: // sceUmdWaitDriveStat
    {
        reg[MIPS_REG_V0] = sceUmdWaitDriveStat(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x56202973: // sceUmdWaitDriveStatWithTimer
    {
        reg[MIPS_REG_V0] = sceUmdWaitDriveStat(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x4A9E5E29: // sceUmdWaitDriveStatCB
    {
        reg[MIPS_REG_V0] = sceUmdWaitDriveStatCB(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xAEE7404D: // sceUmdRegisterUMDCallBack
    {
        reg[MIPS_REG_V0] = sceUmdRegisterUMDCallBack(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xBD2BDE07: // sceUmdUnRegisterUMDCallBack
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xC6183D47: // sceUmdActivate
    {
        reg[MIPS_REG_V0] = sceUmdActivate(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xE83742BA: // sceUmdDeactivate
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }

    // -- sceLibFont --

    case 0x67F17ED7: // sceFontNewLib
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }
    case 0x27F6E642: // sceFontGetNumFontList
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xBC75D85B: // sceFontGetFontList
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x099EF33C: // sceFontFindOptimumFont
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xA834319D: // sceFontOpen
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x0DA7535E: // sceFontGetFontInfo
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }

    // -- sceHprm --
    case 0x1910B327: // sceHprmPeekCurrentKey
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }

    // -- sceUtility --

    // unimplemented
    case 0x2A2B3DE0: // sceUtilityLoadModule
    {
        reg[MIPS_REG_V0] = sceUtilityLoadModule(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xE49BFE92: // sceUtilityUnloadModule
    {
        reg[MIPS_REG_V0] = sceUtilityUnloadModule(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x1579A159: // sceUtilityLoadNetModule
    {
        reg[MIPS_REG_V0] = sceUtilityLoadNetModule(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x2AD8E239: // sceUtilityMsgDialogInitStart
    {
        reg[MIPS_REG_V0] = sceUtilityMsgDialogInitStart(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x4DB1E739: // sceUtilityNetconfInitStart
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x50C4CD57: // sceUtilitySavedataInitStart
    {
        reg[MIPS_REG_V0] = sceUtilitySavedataInitStart(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x6332AA39: // sceUtilityNetconfGetStatus
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x64D50C56: // sceUtilityUnloadNetModule
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x67AF3428: // sceUtilityMsgDialogShutdownStart
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x8874DBE0: // sceUtilitySavedataGetStatus
    {
        reg[MIPS_REG_V0] = sceUtilitySavedataGetStatus();
        return true;
    }
    case 0x91E70E35: // sceUtilityNetconfUpdate
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x95FC253B: // sceUtilityMsgDialogUpdate
    {
        reg[MIPS_REG_V0] = sceUtilityMsgDialogUpdate(reg[MIPS_REG_A0]);
        break;
    }
    case 0x9790B33C: // sceUtilitySavedataShutdownStart
    {
        reg[MIPS_REG_V0] = sceUtilitySavedataShutdownStart();
        return true;
    }
    case 0x9A1C91D7: // sceUtilityMsgDialogGetStatus
    {
        reg[MIPS_REG_V0] = sceUtilityMsgDialogGetStatus();
        return true;
    }
    case 0xA5DA2406: // sceUtilityGetSystemParamInt
    {
        reg[MIPS_REG_V0] = sceUtilityGetSystemParamInt(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x34B78343: // sceUtilityGetSystemParamString
    {
        reg[MIPS_REG_V0] = sceUtilityGetSystemParamString(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xC629AF26: // sceUtilityLoadAvModule
    {
        reg[MIPS_REG_V0] = sceUtilityLoadAvModule(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xD4B95FFB: // sceUtilitySavedataUpdate
    {
        reg[MIPS_REG_V0] = sceUtilitySavedataUpdate();
        return true;
    }
    case 0xF7D8D092: // sceUtilityUnloadAvModule
    {
        reg[MIPS_REG_V0] = sceUtilityUnloadAvModule(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xF88155F6: // sceUtilityNetconfShutdownStart
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }


    // -- scePower --
    case 0x87440F5E: // scePowerIsPowerOnline
    {
        reg[MIPS_REG_V0] = 1;
        LOG_SYSCALL(logType, "scePowerIsPowerOnline()");
        return true;
    }
    case 0x0AFD0D8B: // scePowerIsBatteryExist
    {
        reg[MIPS_REG_V0] = 1;
        LOG_SYSCALL(logType, "scePowerIsBatteryExist()");
        return true;
    }
    case 0xD3075926: // scePowerIsLowBattery
    {
        reg[MIPS_REG_V0] = 0;
        LOG_SYSCALL(logType, "scePowerIsLowBattery()");
        return true;
    }
    case 0x2085D15D: // scePowerGetBatteryLifePercent
    {
        reg[MIPS_REG_V0] = 100;
        LOG_SYSCALL(logType, "scePowerGetBatteryLifePercent()");
        return true;
    }
    case 0x34F9C463: // scePowerGetPllClockFrequencyInt
    {
        LOG_WARN(logType, "unimplemented scePowerGetPllClockFrequencyInt");
        reg[MIPS_REG_V0] = 0xDE;
        return true;
    }
    case 0xFDB5BFE9: // scePowerGetCpuClockFrequencyInt
    {
        LOG_WARN(logType, "unimplemented scePowerGetCpuClockFrequencyInt");
        reg[MIPS_REG_V0] = 0xDE;
        return true;
    }
    case 0xBD681969: // scePowerGetBusClockFrequencyInt
    {
        LOG_WARN(logType, "unimplemented scePowerGetBusClockFrequencyInt");
        reg[MIPS_REG_V0] = 0x6F;
        return true;
    }
    case 0xEBD177D6: // scePowerSetClockFrequency350
    {
        LOG_WARN(logType, "unimplemented scePowerSetClockFrequency350");
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x737486F2: // scePowerSetClockFrequency
    {
        reg[MIPS_REG_V0] = scePowerSetClockFrequency(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x04B7766E: // scePowerRegisterCallback
    {
        reg[MIPS_REG_V0] = scePowerRegisterCallback(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x843FBF43: // scePowerSetCpuClockFrequency
    {
        reg[MIPS_REG_V0] = 0; // unimplemented
        return true;
    }


    // -- sceImpose --
    case 0x24FD7BCF: // sceImposeGetLanguageMode
    {
        reg[MIPS_REG_V0] = 0;
        LOG_WARN(logType, "unimplemented sceImposeGetLanguageMode");
        return true;
    }
    case 0x36AA6E91: // sceImposeSetLanguageMode
    {
        reg[MIPS_REG_V0] = 0;
        LOG_WARN(logType, "unimplemented sceImposeSetLanguageMode");
        return true;
    }
    case 0x8C943191: // sceImposeGetBatteryIconStatus
    {
        reg[MIPS_REG_V0] = 0;
        Core::Memory::write32(reg[MIPS_REG_A0], 0x80000000);
        Core::Memory::write32(reg[MIPS_REG_A1], 3);
        LOG_WARN(logType, "unimplemented sceImposeGetBatteryIconStatus");
        return true;
    }

    // -- sceRtc --
    case 0x3F7AD767: // sceRtcGetCurrentTick
    {
        reg[MIPS_REG_V0] = sceRtcGetCurrentTick(reg[MIPS_REG_A0]);
        return true;
    }
    case 0xC41C2853: // sceRtcGetTickResolution
    {
        reg[MIPS_REG_V0] = sceRtcGetTickResolution();
        return true;
    }
    case 0xE7C27D1B: // sceRtcGetCurrentClockLocalTime
    {

        return true;
    }
    // -- sceAudio --

    case 0x63F2889C: // sceAudioOutput2ChangeLength
    {
        reg[MIPS_REG_A0] = sceAudioOutput2ChangeLength(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x136CAF51: // sceAudioOutputBlocking
    {
        reg[MIPS_REG_V0] = sceAudioOutputBlocking(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x13F592BC: // sceAudioOutputPannedBlocking
    {
        reg[MIPS_REG_V0] = sceAudioOutputPannedBlocking(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3]);
        return true;
    }
    case 0x5EC81C55: // sceAudioChReserve
    {
        reg[MIPS_REG_V0] = sceAudioChReserve(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0x6FC46853: // sceAudioChRelease
    {
        reg[MIPS_REG_V0] = sceAudioChRelease(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x95FD0C2D: // sceAudioChangeChannelConfig
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xB011922F: // sceAudioGetChannelRestLength
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xB7E1D8E7: // sceAudioChangeChannelVolume
    {
        reg[MIPS_REG_V0] = sceAudioChangeChannelVolume(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2]);
        return true;
    }
    case 0xCB2E439E: // sceAudioSetChannelDataLen
    {
        reg[MIPS_REG_V0] = sceAudioSetChannelDataLen(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0xE2D56B2D: // sceAudioOutputPanned
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x01562BA3: // sceAudioOutput2Reserve
    {
        reg[MIPS_REG_V0] = sceAudioOutput2Reserve(reg[MIPS_REG_A0]);
        return true;
    }
    case 0x2D53F36E: // sceAudioOutput2OutputBlocking
    {
        reg[MIPS_REG_V0] = sceAudioOutput2OutputBlocking(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }

    // -- sceSasCore --

    // unimplemented

    case 0x07F58C24: // __sceSasGetAllEnvelopeHeights
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x019B25EB: // __sceSasSetADSR
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x267A6DD2: // __sceSasRevParam
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x2C8E6AB3: // __sceSasGetPauseFlag
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x33D4AB37: // __sceSasRevType
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x42778A9F: // __sceSasInit
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x440CA7D8: // __sceSasSetVolume
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x50A14DFC: // __sceSasCoreWithMix
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x5F9529F6: // __sceSasSetSL
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x68A46B95: // __sceSasGetEndFlag
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x74AE582A: // __sceSasGetEnvelopeHeight
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x76F01ACA: // __sceSasSetKeyOn
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x787D04D5: // __sceSasSetPause
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x99944089: // __sceSasSetVoice
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x9EC3676A: // __sceSasSetADSRmode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xA0CF2FA4: // __sceSasSetKeyOff
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xA3589D81: // __sceSasCore
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xAD84D37F: // __sceSasSetPitch
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xB7660A23: // __sceSasSetNoise
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xBD11B7C2: // __sceSasGetGrain
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xCBCD4F79: // __sceSasSetSimpleADSR
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xD1E0A01E: // __sceSasSetGrain
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xD5A229C9: // __sceSasRevEVOL
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xE175EF66: // __sceSasGetOutputmode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xE855BF76: // __sceSasSetOutputmode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xF983B186: // __sceSasRevVON
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }


    // -- sceMpeg --

    //  unimplemented

    case 0x0E3C2E9D: // sceMpegAvcDecode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x13407F13: // sceMpegRingbufferDestruct
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x167AFD9E: // sceMpegInitAu
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x21FF80E4: // sceMpegQueryStreamOffset
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x37295ED8: // sceMpegRingbufferConstruct
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x42560F23: // sceMpegRegistStream
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x591A4AA2: // sceMpegUnRegistStream
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x606A4649: // sceMpegDelete
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x611E9E11: // sceMpegQueryStreamSize
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x682A619B: // sceMpegInit
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x707B7629: // sceMpegFlushAllStream
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x740FCCD1: // sceMpegAvcDecodeStop
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x800C44DF: // sceMpegAtracDecode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0x874624D6: // sceMpegFinish
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xA11C7026: // sceMpegAvcDecodeMode
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xA780CF7E: // sceMpegMallocAvcEsBuf
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xB240A59E: // sceMpegRingbufferPut
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xB5F6DC87: // sceMpegRingbufferAvailableSize
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xC132E22F: // sceMpegQueryMemSize
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xCEB870B1: // sceMpegFreeAvcEsBuf
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xD7A29F46: // sceMpegRingbufferQueryMemSize
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xD8C5F121: // sceMpegCreate
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xE1CE83A7: // sceMpegGetAtracAu
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xF8DCB679: // sceMpegQueryAtracEsSize
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }
    case 0xFE246728: // sceMpegGetAvcAu
    {
        reg[MIPS_REG_V0] = 0;
        return true;
    }


    // -- sceNet --

    case 0x0BF0A3AE: // sceNetGetLocalEtherAddr
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x281928A9: // sceNetTerm
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x39AF39A6: // sceNetInit
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x89360950: // sceNetEtherNtostr
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }


    // -- sceNetAdhoc --

    case 0x157E6225: // sceNetAdhocPtpClose
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x4DA4C788: // sceNetAdhocPtpSend
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x6F92741B: // sceNetAdhocPdpCreate
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x7F27BB5E: // sceNetAdhocPdpDelete
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x877F6D66: // sceNetAdhocPtpOpen
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x8BEA2B3E: // sceNetAdhocPtpRecv
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x9AC2EEAC: // sceNetAdhocPtpFlush
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x9DF81198: // sceNetAdhocPtpAccept
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xA62C6F57: // sceNetAdhocTerm
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xABED3790: // sceNetAdhocPdpSend
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xDFE53E03: // sceNetAdhocPdpRecv
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xE08BDAC1: // sceNetAdhocPtpListen
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xE1D621D7: // sceNetAdhocInit
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xFC6FC07B: // sceNetAdhocPtpConnect
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }

    // -- sceAtrac3plus --
    
    // unimplemented
    case 0xFAA4F89B:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0xE88F759B:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0x61EB33F5:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0xE23E3A35:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0x868120B5:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0xA2BBA8BE:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0x83BF7AFD:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0x83E85EA0:
        reg[MIPS_REG_V0] = 0;
        return true;
    case 0x7A20E7AF: // sceAtracSetDataAndGetID
    {
        reg[MIPS_REG_V0] = 0;sceAtracSetDataAndGetID(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x6A8C3CD5: // sceAtracDecodeData
    {
        reg[MIPS_REG_V0] = 0;sceAtracDecodeData(reg[MIPS_REG_A0], reg[MIPS_REG_A1], reg[MIPS_REG_A2], reg[MIPS_REG_A3], reg[MIPS_REG_A4]);
        return true;
    }
    case 0x9AE849A7: // sceAtracGetRemainFrame
    {
        reg[MIPS_REG_V0] = 0;sceAtracGetRemainFrame(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }
    case 0x36FAABFB: // sceAtracGetNextSample
    {
        reg[MIPS_REG_V0] = 0;sceAtracGetNextSample(reg[MIPS_REG_A0], reg[MIPS_REG_A1]);
        return true;
    }

    // -- sceNetAdhocctl --

    case 0x08FFF7A0: // sceNetAdhocctlScan
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x20B317A0: // sceNetAdhocctlAddHandler
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x34401D65: // sceNetAdhocctlDisconnect
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x6402490B: // sceNetAdhocctlDelHandler
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x75ECD386: // sceNetAdhocctlGetState
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x9D689E13: // sceNetAdhocctlTerm
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xE162CB14: // sceNetAdhocctlGetPeerList
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xE26F226E: // sceNetAdhocctlInit
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }


    // -- sceNetAdhocMatching --

    case 0x2A2A1E07: // sceNetAdhocMatchingInit
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x32B156B3: // sceNetAdhocMatchingStop
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x5E3D4B79: // sceNetAdhocMatchingSelectTarget
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x7945ECDA: // sceNetAdhocMatchingTerm
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0x93EF3843: // sceNetAdhocMatchingStart
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xB58E61B7: // sceNetAdhocMatchingSetHelloOpt
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xCA5EDA6F: // sceNetAdhocMatchingCreate
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xEA3C6108: // sceNetAdhocMatchingCancelTarget
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }
    case 0xF16EAF4F: // sceNetAdhocMatchingDelete
    {
        reg[MIPS_REG_V0] = 0;
        break;
    }


    // unimplemented/unknown
    case 0x6AD345D7: // sceKernelSetGPO
    {
        reg[MIPS_REG_V0] = 0;
        // LOG_WARN(logType, "unimplemented sceKernelSetGPO");
        return true;
    }

    // Custom
    case Core::HLE::CUSTOM_NID_IDLE:
    {
        uint64_t consumedCycles;
        if (Core::Timing::getIdleCycles() != 0) {
            consumedCycles = Core::Timing::getIdleCycles() - 2;
        } else {
            consumedCycles = Core::Timing::getTimesliceQuantum() - 2;
        }

        Core::Timing::consumeCycles(consumedCycles);
        Core::Timing::consumeIdleCycles();
        return true;
    }
    case Core::HLE::CUSTOM_NID_RETURN_FROM_THREAD:
        sceKernelExitDeleteThread(0);
        return true;
    case Core::HLE::CUSTOM_NID_RETURN_FROM_CALLBACK:
        hleSetReturnFromCallbackState(true);
        return true;
    default:
    ;
    }

    LOG_ERROR(logType, "unknown NID 0x%08X @ PC 0x%08x!", nid, cpu.pc);
    return false;
}
}