#pragma once

#include <cstdint>
#include <Core/PSP/Types.h>

#define DEBUG
#define ENABLE_OPENGL
// #define ENABLE_AUDIO

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

#define ARRAY_SIZEOF(structure) (sizeof(structure) / sizeof(*structure))
#define CAST(cast, function) reinterpret_cast<cast>(function) 