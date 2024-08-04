#include <Core/HLE/Modules/sceUtility.h>
#include <Core/HLE/Savedata.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

namespace Core::HLE {
static const char *logType = "sceUtility";

// module
int sceUtilityLoadModule(int _module) {
    LOG_WARN(logType, "unimplemented sceUtilityLoadModule(%d)", _module);
    return 0;
}

int sceUtilityUnloadModule(int _module) {
    LOG_WARN(logType, "unimplemented sceUtilityUnloadModule(%d)", _module);
    return 0;
}

// net module
int sceUtilityLoadNetModule(int _module) {
    LOG_WARN(logType, "unimplemented sceUtilityLoadNetModule(%d)", _module);
    return 0;
}

int sceUtilityUnloadNetModule(int _module) {
    LOG_WARN(logType, "unimplemented sceUtilityUnloadNetModule(%d)", _module);
}

// msg dialog
int sceUtilityMsgDialogInitStart(uint32_t paramPtr) {
    LOG_WARN(logType, "unimplemented sceUtilityMsgDialogInitStart(0x%08x)", paramPtr);
    return 0;
}

int sceUtilityMsgDialogGetStatus() {
    LOG_WARN(logType, "unimplemented sceUtilityMsgDialogGetStatus()");
    return 0;
}

int sceUtilityMsgDialogAbort();
int sceUtilityMsgDialogShutdownStart();

int sceUtilityMsgDialogUpdate(int n) {
    LOG_WARN(logType, "unimplemented sceUtilityMsgDialogUpdate(%d)", n);
    return 0;
}

// savedata

static Savedata _savedata;

int sceUtilitySavedataInitStart(uint32_t paramPtr) {
    auto savedata = (SceUtilitySavedataParam *) Core::Memory::getPointerUnchecked(paramPtr);

    if (!savedata) {
        LOG_ERROR(logType, "can't initialize savedata if parameter pointer is invalid!");
        return -1;
    }

    LOG_DEBUG(logType,
        "\n"
        "---- savedata info ----\n"
        "file name: %s\n"
        "game name: %s\n"
        "mode: %d\n"
        "overwrite: %d\n"
        "structure base size: 0x%04x\n"
        "data buffer pointer: 0x%08x\n"
        "data buffer (actual size): 0x%08x, buffer size: 0x%08x\n"
        "---- savedata end info ----",
        savedata->fileName,
        savedata->gameName,
        savedata->mode,
        savedata->overwrite,
        savedata->base.size,
        savedata->dataBuf,
        savedata->dataSize,
        savedata->dataBufSize);

    savedata->base.result = 0x80110307;
    LOG_SYSCALL(logType, "sceUtilitySavedataInitStart(0x%08x)", paramPtr);
    _savedata.param = savedata;
    _savedata.status = 1;
    return 0;
}

int sceUtilitySavedataGetStatus() {
    int oldStatus;
    if (!_savedata.param) {
        LOG_WARN(logType, "can't get save data status if parameter is invalid!");
        return 0;
    }

    oldStatus = _savedata.status;
    switch (_savedata.status) {
    case 1:
        _savedata.status = 2;
        break;
    case 2:
        _savedata.status = 3;
        break;
    case 4:
        _savedata.status = 0;
        break;
    }

    LOG_WARN(logType, "sceUtilitySavedataGetStatus() = %d", oldStatus);
    return oldStatus;
}

int sceUtilitySavedataShutdownStart() {
    if (_savedata.param)
        _savedata.status = 4;
    LOG_SYSCALL(logType, "sceUtilitySavedataShutdownStart()");
    return 0;
}

int sceUtilitySavedataUpdate() {
    LOG_SYSCALL(logType, "sceUtilitySavedataUpdate()");
    return 0;
}

// system parameter
/**
* IDs for use inSystemParam functions
* PSP_SYSTEMPARAM_ID_INT are for use with SystemParamInt funcs
* PSP_SYSTEMPARAM_ID_STRING are for use with SystemParamString funcs
*/
#define PSP_SYSTEMPARAM_ID_STRING_NICKNAME	1
#define PSP_SYSTEMPARAM_ID_INT_ADHOC_CHANNEL	2
#define PSP_SYSTEMPARAM_ID_INT_WLAN_POWERSAVE	3
#define PSP_SYSTEMPARAM_ID_INT_DATE_FORMAT	4
#define PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT	5
//Timezone offset from UTC in minutes, (EST = -300 = -5 * 60)
#define PSP_SYSTEMPARAM_ID_INT_TIMEZONE		6
#define PSP_SYSTEMPARAM_ID_INT_DAYLIGHTSAVINGS	7
#define PSP_SYSTEMPARAM_ID_INT_LANGUAGE		8
#define PSP_SYSTEMPARAM_ID_INT_BUTTON_PREFERENCE        9
#define PSP_SYSTEMPARAM_ID_INT_LOCK_PARENTAL_LEVEL      10

/**
* Return values for the SystemParam functions
*/
#define PSP_SYSTEMPARAM_RETVAL_OK	0
#define PSP_SYSTEMPARAM_RETVAL_FAIL	0x80110103

/**
* Valid values for PSP_SYSTEMPARAM_ID_INT_ADHOC_CHANNEL
*/
#define PSP_SYSTEMPARAM_ADHOC_CHANNEL_AUTOMATIC 0
#define PSP_SYSTEMPARAM_ADHOC_CHANNEL_1		1
#define PSP_SYSTEMPARAM_ADHOC_CHANNEL_6		6
#define PSP_SYSTEMPARAM_ADHOC_CHANNEL_11	11

/**
* Valid values for PSP_SYSTEMPARAM_ID_INT_WLAN_POWERSAVE
*/
#define PSP_SYSTEMPARAM_WLAN_POWERSAVE_OFF	0
#define PSP_SYSTEMPARAM_WLAN_POWERSAVE_ON	1

/**
* Valid values for PSP_SYSTEMPARAM_ID_INT_DATE_FORMAT
*/
#define PSP_SYSTEMPARAM_DATE_FORMAT_YYYYMMDD	0
#define PSP_SYSTEMPARAM_DATE_FORMAT_MMDDYYYY	1
#define PSP_SYSTEMPARAM_DATE_FORMAT_DDMMYYYY	2

/**
* Valid values for PSP_SYSTEMPARAM_ID_INT_TIME_FORMAT
*/
#define PSP_SYSTEMPARAM_TIME_FORMAT_24HR	0
#define PSP_SYSTEMPARAM_TIME_FORMAT_12HR	1

/**
* Valid values for PSP_SYSTEMPARAM_ID_INT_DAYLIGHTSAVINGS
*/
#define PSP_SYSTEMPARAM_DAYLIGHTSAVINGS_STD	0
#define PSP_SYSTEMPARAM_DAYLIGHTSAVINGS_SAVING	1

/**
* Valid values for PSP_SYSTEMPARAM_ID_INT_LANGUAGE
*/
#define PSP_SYSTEMPARAM_LANGUAGE_JAPANESE		0
#define PSP_SYSTEMPARAM_LANGUAGE_ENGLISH		1
#define PSP_SYSTEMPARAM_LANGUAGE_FRENCH			2
#define PSP_SYSTEMPARAM_LANGUAGE_SPANISH		3
#define PSP_SYSTEMPARAM_LANGUAGE_GERMAN			4
#define PSP_SYSTEMPARAM_LANGUAGE_ITALIAN		5
#define PSP_SYSTEMPARAM_LANGUAGE_DUTCH			6
#define PSP_SYSTEMPARAM_LANGUAGE_PORTUGUESE		7
#define PSP_SYSTEMPARAM_LANGUAGE_RUSSIAN		8
#define PSP_SYSTEMPARAM_LANGUAGE_KOREAN			9
#define PSP_SYSTEMPARAM_LANGUAGE_CHINESE_TRADITIONAL	10
#define PSP_SYSTEMPARAM_LANGUAGE_CHINESE_SIMPLIFIED	11

int sceUtilityGetSystemParamInt(int id, uint32_t valuePtr) {
    uint32_t paramInt, *ptr;

    switch (id) {
    case PSP_SYSTEMPARAM_ID_INT_LANGUAGE:
        paramInt = PSP_SYSTEMPARAM_LANGUAGE_ENGLISH; // TODO
        break;
    case PSP_SYSTEMPARAM_ID_INT_BUTTON_PREFERENCE:
        paramInt = 1; // North America
        break;
    default:
        LOG_ERROR(logType, "invalid sceUtilityGetSystemParamInt(%d, 0x%08x)", id, valuePtr);
        return PSP_SYSTEMPARAM_RETVAL_FAIL;
    }

    ptr = (uint32_t *) Core::Memory::getPointerUnchecked(valuePtr);

    if (ptr) {
        *ptr = paramInt;
        LOG_SYSCALL(logType, "sceUtilityGetSystemParamInt(%d, 0x%08x)", id, valuePtr);
    } else {
        LOG_ERROR(logType, "sceUtilityGetSystemParamInt(%d, 0x%08x) invalid pointer", id, valuePtr);
    }
    return 0;
}

int sceUtilityGetSystemParamString(int id, uint32_t bufPtr, int len) {
    static const char *nickname = "Shaders\x00";
    switch (id) {
    case PSP_SYSTEMPARAM_ID_STRING_NICKNAME:
        Core::Memory::Utility::copyMemoryHostToGuest(bufPtr, nickname, 8);
        break;
    default:
        LOG_WARN(logType, "unimplemented sceUtilityGetSystemParamString id %d", id);
        return PSP_SYSTEMPARAM_RETVAL_FAIL;
    }
    
    LOG_SYSCALL(logType, "sceUtilityGetSystemParamString id %d", id);
    return 0;
}

#define PSP_AV_MODULE_AVCODEC 0
#define PSP_AV_MODULE_SASCORE 1
#define PSP_AV_MODULE_ATRAC3PLUS 2
#define PSP_AV_MODULE_MPEGBASE 3
#define PSP_AV_MODULE_MP3 4
#define PSP_AV_MODULE_VAUDIO 5
#define PSP_AV_MODULE_AAC 6
#define PSP_AV_MODULE_G729 7

// av module
int sceUtilityLoadAvModule(int _module) {
    LOG_WARN(logType, "unimplemented sceUtilityLoadAvModule(%d)", _module);
    return 0;
}

int sceUtilityUnloadAvModule(int _module) {
    LOG_WARN(logType, "unimplemented sceUtilityUnloadAvModule(%d)", _module);
    return 0;
}
}