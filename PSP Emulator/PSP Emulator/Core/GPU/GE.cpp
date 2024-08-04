#include <Core/GPU/GE.h>
#include <Core/GPU/DisplayList.h>

#include <Core/Kernel/sceKernelError.h>
#include <Core/Kernel/sceKernelInterrupt.h>

#include <Core/PSP/MemoryMap.h>
#include <Core/Memory/MemoryAccess.h>

#include <Core/Allegrex/AllegrexState.h>

#include <Core/Logger.h>

using namespace Core::Kernel;

namespace Core::GPU {
static const char *logType = "GraphicsEngine";

static int displayListId;
static constexpr const int DISPLAY_LIST_MAX_QUEUED_SIZE = 64;

static int generateDisplayList() {
    if (displayListId >= 64)
        displayListId = 0;
    return 2000 + (displayListId++ % DISPLAY_LIST_MAX_QUEUED_SIZE);
}

int sceGeListEnQueue(uint32_t listAddress, uint32_t stallAddress, int cbid, uint32_t argPtr) {
    DisplayList dl;
    dl.id = generateDisplayList();
    dl.callbackId = cbid;
    dl.currentAddress = dl.initialAddress = listAddress & 0x0FFFFFFC;
    dl.stallAddress = stallAddress & 0x0FFFFFFC;
    dl.callbackArgument = argPtr;
    std::memset(dl.callStack, 0, sizeof dl.callStack);
    dl.callStackCurrentIndex = 0;
    dl.state = 1;
    addDisplayListToQueue(dl);
    LOG_SYSCALL(logType, "0x%08x %d = sceGeListEnQueue(list: 0x%08x, stall: 0x%08x, cbid: %d, args: 0x%08x)", Core::Allegrex::cpu.pc, dl.id, listAddress, stallAddress, cbid, argPtr);
    return dl.id;
}

int sceGeListDeQueue(int qid) {
    LOG_SYSCALL(logType, "sceGeListDeQueue(qid: 0x%08x)", qid);
    return 0;
}

int sceGeListUpdateStallAddr(int qid, uint32_t stallAddress) {
    auto dl = getDisplayListFromQueue(qid);
    if (!dl)
        return SCE_KERNEL_ERROR_ALREADY;

    dl->stallAddress = stallAddress & 0x0FFFFFFC;
    // LOG_WARN(logType, "sceGeListUpdateStallAddr(qid: %d, stall: 0x%08x)", qid, stallAddress);
    return 0;
}

int sceGeEdramGetSize() {
    LOG_SYSCALL(logType, "sceGeEdramGetSize()");
    return Core::PSP::VRAM_MEMORY_SIZE;
}

uint32_t sceGeEdramGetAddr() {
    LOG_SYSCALL(logType, "sceGeEdramGetAddr()");
    return 0x04000000;
}

int sceGeSetCallback(uint32_t cbPtr) {
    struct GECallbackData {
        uint32_t signalHandler;
        uint32_t signalArg;
        uint32_t finishHandler;
        uint32_t finishArg;
    };

    GECallbackData *data = (GECallbackData *) Core::Memory::getPointerUnchecked(cbPtr);

    if (!data) {
        LOG_ERROR(logType, "can't set GE callback got address 0x%08x", cbPtr);
        return -1;
    }

    // sceKernelRegisterSubIntrHandler(25, 0, data->signalHandler, data->signalArg);
    sceKernelRegisterSubIntrHandler(25, 1, data->finishHandler, data->finishArg);
    sceKernelEnableSubIntr(25, 1);

    LOG_SYSCALL(logType, "sceGeSetCallback(0x%08x)", cbPtr);
    return 0;
}

int sceGeUnsetCallback(int cbid) {
    LOG_WARN(logType, "unimplemented sceGeUnsetCallback(%d)", cbid);
    return 0;
}

int sceGeDrawSync(int syncType) {
    // LOG_WARN(logType, "unimplemented sceGeDrawSync(%d)", syncType);
    return 0;
}

int sceGeListSync(int qid, int syncType) {
    // LOG_WARN(logType, "unimplemented sceGeListSync(0x%08x, %d)", qid, syncType);
    return 0;
}
}