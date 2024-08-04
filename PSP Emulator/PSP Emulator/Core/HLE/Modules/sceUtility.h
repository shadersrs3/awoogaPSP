#pragma once

#include <cstdint>

namespace Core::HLE {
// module
int sceUtilityLoadModule(int _module);
int sceUtilityUnloadModule(int _module);

// net module
int sceUtilityLoadNetModule(int _module);
int sceUtilityUnloadNetModule(int _module);

// savedata
int sceUtilitySavedataInitStart(uint32_t paramPtr);
int sceUtilitySavedataGetStatus();
int sceUtilitySavedataShutdownStart();
int sceUtilitySavedataUpdate();

// msg dialog
int sceUtilityMsgDialogInitStart(uint32_t paramPtr);
int sceUtilityMsgDialogGetStatus();
int sceUtilityMsgDialogAbort();
int sceUtilityMsgDialogShutdownStart();
int sceUtilityMsgDialogUpdate(int n);

// system parameter
int sceUtilityGetSystemParamInt(int id, uint32_t valuePtr);
int sceUtilityGetSystemParamString(int id, uint32_t bufPtr, int len);

// av module
int sceUtilityLoadAvModule(int _module);
int sceUtilityUnloadAvModule(int _module);
}