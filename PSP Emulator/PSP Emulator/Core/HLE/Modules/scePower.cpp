#include <Core/HLE/Modules/scePower.h>

#include <Core/Kernel/sceKernelCallback.h>

#include <Core/Logger.h>

using namespace Core::Kernel;

namespace Core::HLE {
static const char *logType = "scePower";

struct PowerCallback {
    int cbid;
};

static PowerCallback powerCallbacks[16];

int scePowerSetClockFrequency(int pllfreq, int cpufreq, int busfreq) {
    LOG_WARN(logType, "unimplemented scePowerSetClockFrequency(%d, %d, %d)", pllfreq, cpufreq, busfreq);
    return 0;
}

const int PSP_POWER_ERROR_TAKEN_SLOT = 0x80000020;
const int PSP_POWER_ERROR_SLOTS_FULL = 0x80000022;
const int PSP_POWER_ERROR_EMPTY_SLOT = 0x80000025;
const int PSP_POWER_ERROR_INVALID_CB = 0x80000100;
const int PSP_POWER_ERROR_INVALID_SLOT = 0x80000102;

const int PSP_POWER_CB_AC_POWER = 0x00001000;
const int PSP_POWER_CB_BATTERY_EXIST = 0x00000080;
const int PSP_POWER_CB_BATTERY_FULL = 0x00000064;

std::deque<int> readyPowerCallbacks;

std::deque<int> *getReadyPowerCallbacks() {
    if (readyPowerCallbacks.size() != 0)
        return &readyPowerCallbacks;
    return nullptr;
}

int scePowerRegisterCallback(int slot, int cbid) {
    Callback *c = getKernelObject<Callback>(cbid);

    if (!c) {
        LOG_ERROR(logType, "can't get power callback id 0x%06x slot %d", cbid, slot);
        return PSP_POWER_ERROR_INVALID_CB;
    }

    if (slot < -1 || slot >= 16) {
        LOG_ERROR(logType, "scePowerRegisterCallback(%d, 0x%06x) invalid slot", slot, cbid);
        return PSP_POWER_ERROR_INVALID_SLOT;
    }

    if (slot == -1) {
        for (int i = 0; i < sizeof(powerCallbacks)/sizeof(*powerCallbacks); i++) {
            if (powerCallbacks[i].cbid == 0) {
                slot = i;
                break;
            }
        }

    }

    if (slot == -1) {
        LOG_ERROR(logType, "scePowerRegisterCallback(%d, 0x%06x) invalid slot", slot, cbid);
        return PSP_POWER_ERROR_INVALID_SLOT;
    }
#if 0

    if (powerCallbacks[slot].cbid != 0) {
        LOG_ERROR(logType, "scePowerRegisterCallback(%d, 0x%06x) taken slot", slot, cbid);
        return PSP_POWER_ERROR_TAKEN_SLOT;
    }
#endif

    powerCallbacks[slot].cbid = c->getUID();
    c->notifyArg = PSP_POWER_CB_AC_POWER | PSP_POWER_CB_BATTERY_EXIST | PSP_POWER_CB_BATTERY_FULL;
    c->ready = true;
    readyPowerCallbacks.push_back(cbid);
    LOG_SYSCALL(logType, "scePowerRegisterCallback(%d, 0x%06x)", slot, cbid);
    return 0;
}
}