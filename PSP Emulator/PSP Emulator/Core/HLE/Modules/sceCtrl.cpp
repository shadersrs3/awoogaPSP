#include <Core/Timing.h>
#include <Core/HLE/Modules/sceCtrl.h>
#include <Core/Memory/MemoryAccess.h>

#include <Core/HostController.h>

#include <Core/Logger.h>

extern unsigned int buttons;

namespace Core::HLE {
static const char *logType = "sceCtrl";

#pragma pack(push, 1)

struct SceCtrlData {
    uint32_t timestamp;
    uint32_t buttons;
    uint8_t xAxis;
    uint8_t yAxis;
    uint8_t reserved[6];
};

#pragma pack(pop)

int sceCtrlSetSamplingCycle(int cycle) {
    LOG_WARN(logType, "unimplemented sceCtrlSetSamplingCycle(%d)", cycle);
    return 0;
}

int sceCtrlSetSamplingMode(int cycle) {
    LOG_WARN(logType, "unimplemented sceCtrlSetSamplingMode(%d)", cycle);
    return 0;
}

int sceCtrlReadBufferPositive(uint32_t ctrlDataPtr, int count) {
    for (int i = 0; i < count; i++) {
        auto data = (SceCtrlData *) Core::Memory::getPointerUnchecked(ctrlDataPtr + i * sizeof(SceCtrlData));
        if (!data) {
            LOG_ERROR(logType, "can't read positive buffer! got address 0x%08x", ctrlDataPtr);
            return -1;
        }

        data->timestamp = (uint32_t)Core::Timing::getCurrentCycles();
        data->buttons = Core::Controller::pspGetButtons();
        data->xAxis = Core::Controller::pspGetAnalogAxisX();
        data->yAxis = Core::Controller::pspGetAnalogAxisY();
        std::memset(data->reserved, 0, sizeof data->reserved);
    }

    Core::Timing::consumeCycles(100);
    // LOG_SYSCALL(logType, "sceCtrlReadBufferPositive(0x%08x, %d)", ctrlDataPtr, count);
    return count;
}

int sceCtrlPeekBufferPositive(uint32_t ctrlDataPtr, int count) {
    auto data = (SceCtrlData *) Core::Memory::getPointerUnchecked(ctrlDataPtr);
    if (!data) {
        LOG_ERROR(logType, "can't peek positive buffer! got address 0x%08x", ctrlDataPtr);
        return -1;
    }

    for (int i = 0; i < count; i++) {
        auto data = (SceCtrlData *) Core::Memory::getPointerUnchecked(ctrlDataPtr + i * sizeof(SceCtrlData));
        if (!data) {
            LOG_ERROR(logType, "can't read positive buffer! got address 0x%08x", ctrlDataPtr);
            return -1;
        }

        data->timestamp = (uint32_t)Core::Timing::getCurrentCycles();
        data->buttons = Core::Controller::pspGetButtons();
        data->xAxis = Core::Controller::pspGetAnalogAxisX();
        data->yAxis = Core::Controller::pspGetAnalogAxisY();
        std::memset(data->reserved, 0, sizeof data->reserved);
    }

    Core::Timing::consumeCycles(100);
    // LOG_SYSCALL(logType, "sceCtrlReadBufferPositive(0x%08x, %d)", ctrlDataPtr, count);
    return count;
}
}