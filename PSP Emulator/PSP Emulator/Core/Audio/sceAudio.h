#pragma once

#include <cstdint>

namespace Core::Audio {
int sceAudioOutputBlocking(int channel, int volume, uint32_t bufferAddress);
int sceAudioOutputPannedBlocking(int channel, int leftVolume, int rightVolume, uint32_t bufferAddress);
int sceAudioChReserve(int channel, int sampleCount, int frequency);
int sceAudioChRelease(int channel);
int sceAudioSetChannelDataLen(int channel, int samplecount);
int sceAudioOutput2Reserve(int samplecount);
int sceAudioOutput2OutputBlocking(int vol, uint32_t bufferAddress);
int sceAudioChangeChannelVolume(int channel, int leftVolume, int rightVolume);
int sceAudioOutput2ChangeLength(int samplecount);
}