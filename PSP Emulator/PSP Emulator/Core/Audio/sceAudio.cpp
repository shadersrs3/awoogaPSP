#include <Core/Audio/sceAudio.h>

#include <Core/Kernel/sceKernelTypes.h>
#include <Core/Kernel/sceKernelThread.h>

#include <Core/Audio/sceAudio.h>

#include <Core/Logger.h>

#include <Core/Timing.h>

using namespace Core::Kernel;

namespace Core::Audio {
static const char *logType = "sceAudio";

static void __audioOutput(int channel, int lVol, int rVol, uint32_t bufferAddress, bool blocking) {
    current->waitData[0] = Core::Timing::getBaseCycles() + Core::Timing::usToCycles((uint64_t) 1000000);
    threadAddToWaitingList(current, 0, WAITTYPE_AUDIOCHANNEL);
}

int sceAudioOutputBlocking(int channel, int volume, uint32_t bufferAddress) {
    __audioOutput(channel, volume, volume, bufferAddress, true);
    // LOG_WARN(logType, "unimplemented sceAudioOutputBlocking");
    return 0;
}

int sceAudioOutputPannedBlocking(int channel, int leftVolume, int rightVolume, uint32_t bufferAddress) {
    __audioOutput(channel, leftVolume, rightVolume, bufferAddress, true);
    // LOG_WARN(logType, "unimplemented sceAudioOutputPannedBlocking");
    return 0x100;
}

int sceAudioChReserve(int channel, int sampleCount, int frequency) {
    LOG_WARN(logType, "unimplemented sceAudioChReserve");
    return 0;
}

int sceAudioChRelease(int channel) {
    LOG_WARN(logType, "unimplemented sceAudioChRelease");
    return 0;
}

int sceAudioSetChannelDataLen(int channel, int samplecount) {
    LOG_WARN(logType, "unimplemented sceAudioSetChannelDataLen");
    return 0;
}

int sceAudioOutput2Reserve(int samplecount) {
    LOG_WARN(logType, "unimplemented sceAudioOutput2Reserve");
    return 0;
}

int sceAudioOutput2OutputBlocking(int vol, uint32_t bufferAddress) {
    __audioOutput(6, vol, vol, bufferAddress, true);
    LOG_WARN(logType, "unimplemented sceAudioOutput2OutputBlocking");
    return 0;
}

int sceAudioChangeChannelVolume(int channel, int leftVolume, int rightVolume) {
    LOG_WARN(logType, "unimplemented sceAudioChangeChannelVolume(%d, %d, %d)", channel, leftVolume, rightVolume);
    return 0;
}

int sceAudioOutput2ChangeLength(int samplecount) {
    LOG_WARN(logType, "unimplemented sceAudioOutput2ChangeLength(%d)", samplecount);
    return 0;
}
}