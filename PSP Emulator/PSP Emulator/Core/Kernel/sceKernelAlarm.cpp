#include <Core/Kernel/sceKernelAlarm.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

namespace Core::Kernel {
static const char *logType = "sceKernelAlarm";

std::vector<Alarm *> alarmList;

std::vector<Alarm *>& hleSchedulerGetAlarmList() {
    return alarmList;
}

static SceUID __setAlarm(uint64_t clock, uint32_t alarmHandler, uint32_t common) {
    int error;
    Alarm *a = createKernelObject<Alarm>(&error);
    if (!a)
        return error;
    
    a->clock = Core::Timing::getCurrentCycles() + Core::Timing::usToCycles(clock);
    a->alarmHandler = alarmHandler;
    a->common = common;
    alarmList.push_back(a);
    return a->getUID();
}

SceUID sceKernelSetAlarm(SceUInt clock, uint32_t alarmHandler, uint32_t common) {
    __setAlarm(clock, alarmHandler, common);
    LOG_SYSCALL(logType, "sceKernelSetAlarm(us: 0x%08x, handler: 0x%08x, common: 0x%08x)", clock, alarmHandler, common);
    return 0;
}

SceUID sceKernelSetSysClockAlarm(uint32_t sysclockPtr, uint32_t alarmHandler, uint32_t common) {
    return 0;
}

int sceKernelCancelAlarm(SceUID aid) {
    int error;
    Alarm *a = getKernelObject<Alarm>(aid, &error);
    if (a) {
        if (auto it = std::find_if(alarmList.begin(), alarmList.end(),
            [&](Alarm *alarm) { return alarm->getUID() == a->getUID(); }); it != alarmList.end()) {
            alarmList.erase(it);
        }

        destroyKernelObject(a->getUID());
    }

    LOG_SYSCALL(logType, "sceKernelCancelAlarm(aid: 0x%06x)", aid);
    return 0;
}
}