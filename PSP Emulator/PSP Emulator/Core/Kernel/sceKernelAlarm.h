#pragma once

#include <Core/Kernel/Objects/Alarm.h>

#include <vector>

namespace Core::Kernel {
std::vector<Alarm *>& hleSchedulerGetAlarmList();
SceUID sceKernelSetAlarm(SceUInt clock, uint32_t alarmHandler, uint32_t common);
SceUID sceKernelSetSysClockAlarm(uint32_t sysclockPtr, uint32_t alarmHandler, uint32_t common);
int sceKernelCancelAlarm(SceUID aid);
}