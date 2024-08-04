#pragma once

#include <cstdint>

/* Index for the two analog directions */
#define CTRL_ANALOG_X   0
#define CTRL_ANALOG_Y   1

/* Button bit masks */
#define SCE_CTRL_SELECT        0x00000001
#define SCE_CTRL_START         0x00000008
#define SCE_CTRL_UP            0x00000010
#define SCE_CTRL_RIGHT         0x00000020
#define SCE_CTRL_DOWN          0x00000040
#define SCE_CTRL_LEFT          0x00000080
#define SCE_CTRL_L             0x00000100
#define SCE_CTRL_R             0x00000200
#define SCE_CTRL_TRIANGLE      0x00001000
#define SCE_CTRL_CIRCLE        0x00002000
#define SCE_CTRL_CROSS         0x00004000
#define SCE_CTRL_SQUARE        0x00008000
#define SCE_CTRL_HOME          0x00010000
#define SCE_CTRL_HOLD          0x00020000
#define SCE_CTRL_SCREEN        0x00040000
#define SCE_CTRL_NOTE          0x00080000
#define SCE_CTRL_VOLUP         0x00100000
#define SCE_CTRL_VOLDOWN       0x00200000
#define SCE_CTRL_WLAN_UP       0x00400000
#define SCE_CTRL_REMOTE        0x00800000
#define SCE_CTRL_DISC          0x01000000
#define SCE_CTRL_MS            0x02000000

namespace Core::HLE {
int sceCtrlSetSamplingCycle(int cycle);
int sceCtrlSetSamplingMode(int cycle);
int sceCtrlReadBufferPositive(uint32_t ctrlDataPtr, int count);
int sceCtrlPeekBufferPositive(uint32_t ctrlDataPtr, int count);
}