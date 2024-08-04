#include <Core/GPU/sceDisplay.h>

#include <Core/Kernel/sceKernelTypes.h>
#include <Core/Kernel/sceKernelThread.h>
#include <Core/Kernel/sceKernelInterrupt.h>

#include <Core/Logger.h>

#include <Core/Timing.h>

using namespace Core::Kernel;

namespace Core::GPU {
static const char *logType = "sceDisplay";

static void __waitVblankStart(bool handleCallbacks = false) {
    if (handleCallbacks)
        hleCurrentThreadEnableCallbackState();

    current->waitData[0] = Core::Timing::msToCycles(16.67f) + Core::Timing::getCurrentCycles();
    threadAddToWaitingList(current, 0, WAITTYPE_VBLANK);
}

int sceDisplayWaitVblankStartCB() {
    __waitVblankStart(true);
    // LOG_WARN(logType, "unimplemented sceDisplayWaitVblankStartCB");
    return 0;
}

int sceDisplayWaitVblankStart() {
    __waitVblankStart();
    // LOG_WARN(logType, "unimplemented sceDisplayWaitVblankStart");
    return 0;
}

int sceDisplayWaitVblank() {
    Core::Timing::consumeCycles(1110);
    // LOG_WARN(logType, "unimplemented sceDisplayWaitVblank");
    return 0;
}

int sceDisplayIsVblank() {
    return 1;
}

int sceDisplayGetCurrentHcount() {
    return 0;
}

int sceDisplayGetVcount() {
    return 0;
}
}