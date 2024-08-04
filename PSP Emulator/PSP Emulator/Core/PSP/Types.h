#pragma once

#include <cstdint>


/* SCE types. */
typedef unsigned char SceUChar8;
typedef uint16_t SceUShort16;
typedef uint32_t SceUInt32;
typedef uint64_t SceUInt64;
typedef uint64_t SceULong64;

typedef char SceChar8;
typedef int16_t SceShort16;
typedef int32_t SceInt32;
typedef int64_t SceInt64;
typedef int64_t SceLong64;

typedef float SceFloat;
typedef float SceFloat32;

typedef short unsigned int SceWChar16;
typedef unsigned int SceWChar32;

typedef int SceBool;

typedef void SceVoid;
typedef void *ScePVoid;

typedef int SceUID;

typedef unsigned int SceSize;
typedef int SceSSize;

typedef unsigned char SceUChar;
typedef unsigned int SceUInt;

typedef int SceMode;
typedef SceInt64 SceOff;
typedef SceInt64 SceIores;

#pragma pack(push, 1)
struct ScePspDateTime {
    int16_t year;
    int16_t month;
    int16_t day;
    int16_t hour;
    int16_t minute;
    int16_t second;
    uint32_t microsecond;
};
#pragma pack(pop)