#pragma once

#include <cstdint>

#include <Core/PSP/Types.h>
#include <Core/HLE/Dialog.h>

namespace Core::HLE {

/** Initial focus position for list selection types */
enum PspUtilitySavedataFocus : int {
    PSP_UTILITY_SAVEDATA_FOCUS_UNKNOWN = 0,
    PSP_UTILITY_SAVEDATA_FOCUS_FIRSTLIST,	/* First in list */
    PSP_UTILITY_SAVEDATA_FOCUS_LASTLIST,	/* Last in list */
    PSP_UTILITY_SAVEDATA_FOCUS_LATEST,	/* Most recent date */
    PSP_UTILITY_SAVEDATA_FOCUS_OLDEST,	/* Oldest date */
    PSP_UTILITY_SAVEDATA_FOCUS_UNKNOWN2,
    PSP_UTILITY_SAVEDATA_FOCUS_UNKNOWN3,
    PSP_UTILITY_SAVEDATA_FOCUS_FIRSTEMPTY, /* First empty slot */
    PSP_UTILITY_SAVEDATA_FOCUS_LASTEMPTY,	/*Last empty slot */
};

enum PspUtilitySavedataMode : int {
    PSP_UTILITY_SAVEDATA_AUTOLOAD = 0,
    PSP_UTILITY_SAVEDATA_AUTOSAVE,
    PSP_UTILITY_SAVEDATA_LOAD,
    PSP_UTILITY_SAVEDATA_SAVE,
    PSP_UTILITY_SAVEDATA_LISTLOAD,
    PSP_UTILITY_SAVEDATA_LISTSAVE,
    PSP_UTILITY_SAVEDATA_LISTDELETE,
    PSP_UTILITY_SAVEDATA_LISTALLDELETE,
    SCE_UTILITY_SAVEDATA_SIZES,
    SCE_UTILITY_SAVEDATA_AUTODELETE,
    SCE_UTILITY_SAVEDATA_DELETE,
    SCE_UTILITY_SAVEDATA_LIST,
    SCE_UTILITY_SAVEDATA_FILES,
    SCE_UTILITY_SAVEDATA_MAKEDATASECURE,
    SCE_UTILITY_SAVEDATA_MAKEDATA,
    SCE_UTILITY_SAVEDATA_READDATASECURE,
    SCE_UTILITY_SAVEDATA_READDATA,
    SCE_UTILITY_SAVEDATA_WRITEDATASECURE,
    SCE_UTILITY_SAVEDATA_WRITEDATA,
    SCE_UTILITY_SAVEDATA_ERASESECURE,
    SCE_UTILITY_SAVEDATA_ERASE,
    SCE_UTILITY_SAVEDATA_DELETEDATA,
    SCE_UTILITY_SAVEDATA_GETSIZE,
};

#pragma pack(push, 1)
struct PspUtilitySavedataSFOParam {
    char title[0x80];
    char savedataTitle[0x80];
    char detail[0x400];
    uint8_t parentalLevel;
    uint8_t unknown[3];
};

struct PspUtilitySavedataFileData {
    uint32_t bufPtr;
    SceSize bufSize;
    SceSize size;
    int unknown;
};

struct PspUtilitySavedataListSaveNewData {
    PspUtilitySavedataFileData icon0;
    uint32_t title;
};

struct SceUtilitySavedataMsFreeInfo {
    int clusterSize;
    int freeClusters;
    int freeSpaceKB;
    char freeSpaceStr[8];
};

struct SceUtilitySavedataUsedDataInfo {
    int usedClusters;
    int usedSpaceKB;
    char usedSpaceStr[8];
    int usedSpace32KB;
    char usedSpace32Str[8];
};

struct SceUtilitySavedataParam {
    SceDialogCommon base;
    int mode;
    int bind;
    int overwrite;
    char gameName[13];
    char reserved[3];
    char saveName[20];
    uint32_t saveNameList;
    char fileName[13];
    char reserved1[3];
    uint32_t dataBuf;
    SceSize dataBufSize;
    SceSize dataSize;
    PspUtilitySavedataSFOParam sfoParam;
    PspUtilitySavedataFileData icon0FileData;
    PspUtilitySavedataFileData icon1FileData;
    PspUtilitySavedataFileData pic1FileData;
    PspUtilitySavedataFileData snd0FileData;
    uint32_t newDataPointer;
    PspUtilitySavedataFocus focus;
    int abortStatus;
    uint32_t msFreePtr;
    uint32_t msDataPtr;
    uint32_t utilityDataPtr;
    uint8_t key[0x10];
    uint32_t secureVersion;
    uint32_t multiStatus;
    uint32_t idList;
    uint32_t fileList;
    uint32_t sizeInfo;
};
#pragma pack(pop)

struct Savedata {
    int status;
    SceUtilitySavedataParam *param;
};
}