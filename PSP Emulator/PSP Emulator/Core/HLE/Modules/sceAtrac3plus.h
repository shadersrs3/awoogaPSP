#pragma once

#include <cstdint>

#include <Core/PSP/Types.h>

namespace Core::HLE {
int sceAtracAddStreamData(int atracID, uint32_t bytesToAdd);
int sceAtracSetDataAndGetID(uint32_t bufPtr, SceSize bufsize);
int sceAtracDecodeData(int atracID, uint32_t outSamplesPtr, uint32_t outNPtr, uint32_t outEndPtr, uint32_t outRemainFramePtr);
int sceAtracGetNextSample(int atracID, uint32_t outNPtr);
int sceAtracGetStreamDataInfo(int atracID, uint32_t writePointerPtr, uint32_t availableBytesPtr, uint32_t readOffsetPtr);
int sceAtracSetData(int atracID, uint32_t bufferPtr, uint32_t bufferByte);
int sceAtracGetRemainFrame(int atracID, uint32_t outRemainFramePtr);
}