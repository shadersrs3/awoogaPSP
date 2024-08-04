#pragma once

#include <cstdint>

namespace Core::Kernel { struct Callback; }

namespace Core::HLE {
Core::Kernel::Callback *__getRegisteredUMDCallback();
int sceUmdActivate(int unit, uint32_t drivePtr);
int sceUmdCancelWaitDriveStat();
int sceUmdCheckMedium();
int sceUmdDeactivate(int unit, uint32_t drivePtr);
int sceUmdGetDiscInfo(uint32_t infoPtr);
int sceUmdGetDriveStat();
int sceUmdGetErrorStat();
int sceUmdRegisterUMDCallBack(int cbid);
int sceUmdReplacePermit();
int sceUmdReplaceProhibit();
int sceUmdUnRegisterUMDCallBack(int cbid);
int sceUmdWaitDriveStat(int stat);
int sceUmdWaitDriveStatCB(int stat, uint32_t timeoutPtr);
int sceUmdWaitDriveStatWithTimer(int stat, uint32_t timeout);
}