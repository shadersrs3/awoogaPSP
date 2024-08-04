#include <cstdio>
#include <cstdlib>
#include <vector>

#include <Core/HLE/Modules/sceAtrac3plus.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

namespace Core::HLE {
static const char *logType = "Atrac3";

#define WAVE_CHUNK_ID_DATA 0x61746164 /* "data" */
#define WAVE_CHUNK_ID_SMPL 0x6C706D73 /* "smpl" */
#define WAVE_CHUNK_ID_FACT 0x74636166 /* "fact" */
#define WAVE_CHUNK_ID_FMT  0x20746D66 /* "fmt " */
#define RIFF_MAGIC 0x46464952 /* "RIFF" */
#define WAVE_MAGIC 0x45564157 /* "WAVE" */

#define ATRAC3_SAMPLES_PER_FRAME (1024)
#define ATRAC3PLUS_FRAME_SAMPLES (2048)

enum class AudioCodec : uint16_t {
    AT3 = 0x0270,
    AT3_PLUS = 0xFFFE
};

static const int atracDecodeDelayUs = 2700;

struct AtracInfo {
    AudioCodec codec;
    int channels;
    int sampleRate;
    int bitRate;
    int bytesPerFrame;
    int endSample;
    int sampleOffset;
    uint32_t baseDataAddress;
    uint32_t dataAddressEnd;
    int dataSize;
    uint32_t currentDataAddress;
    int startSkippedSamples;
    int maxSamples;
};

struct AtracID {
    uint32_t baseAddress;
    int initialBufferSize;
    AtracInfo info;
    uint8_t *data;
};

static int atracIDCount = 0;
std::vector<AtracID> atracList { AtracID {} }; // There's only ever 6

static void debugAtracID(const AtracID *id) {
    const AtracInfo *info = &id->info;
    printf("Base address: 0x%08x, initial buffer size: 0x%08x\n", id->baseAddress, id->initialBufferSize);
    printf("Codec: 0x%04x, channels: %d, sample rate: %d, bit rate: %d, bytes per frame %d, data address: 0x%08x, end data address: 0x%08x, endSample: %d, sampleOffset: %d\n", info->codec, info->channels, info->sampleRate, info->bitRate, info->bytesPerFrame, info->baseDataAddress, info->dataAddressEnd, info->endSample, info->sampleOffset);
}

static bool analyzeRIFF(AtracID *id) {
    if (*(uint32_t *) id->data != RIFF_MAGIC) {
        LOG_ERROR(logType, "can't analyze a non RIFF header");
        return false;
    }

    if (*(uint32_t *)(id->data + 0x8) != WAVE_MAGIC) {
        LOG_ERROR(logType, "can't analyze a non WAVE header");
        return false;
    }

    if (id->initialBufferSize < 12) {
        LOG_WARN(logType, "RIFF file is invalid");
        return false;
    }

    uint8_t *chunkStart = (id->data + 0xC);
    uint8_t *chunkEndLimit = id->data + id->initialBufferSize;

    while (chunkStart < chunkEndLimit) {
        uint8_t *identifier = chunkStart;
        uint32_t length = *(uint32_t *) (chunkStart + 0x4);
        uint8_t *data = chunkStart + 0x8;
        uint32_t offset = (uint32_t)(id->initialBufferSize - (chunkEndLimit - chunkStart) + 0x8);

        if (length == 0) {
            LOG_ERROR(logType, "length is zero for an Atrac3 context");
            return false;
        }

        uint32_t magic = *(uint32_t *) identifier;

        switch (magic) {
        case WAVE_CHUNK_ID_FMT:
            if (length >= 16) {
                AudioCodec codec = *(AudioCodec *) data;
                uint16_t channels = *(uint16_t *)(data + 2);
                int sampleRate = *(uint32_t *)(data + 4);
                int bitRate = *(uint32_t *)(data + 8);
                int bytesPerFrame = *(uint16_t *)(data + 12);
                int hiBytesPerSample = *(uint16_t *)(data + 14);


                switch (codec) {
                case AudioCodec::AT3:
                    id->info.maxSamples = ATRAC3_SAMPLES_PER_FRAME;
                    id->info.startSkippedSamples = 69;
                    break;
                case AudioCodec::AT3_PLUS:
                    id->info.maxSamples = ATRAC3PLUS_FRAME_SAMPLES;
                    id->info.startSkippedSamples = 368;
                    break;
                default:
                    id->info.maxSamples = 0;
                    id->info.startSkippedSamples = 0;
                    break;
                }

                if (!(codec == AudioCodec::AT3 || codec == AudioCodec::AT3_PLUS)) {
                    LOG_ERROR(logType, "unknown codec it's either at3 or at3plus");
                    return false;
                } else {
                }

                id->info.baseDataAddress = id->baseAddress + offset;
                id->info.codec = codec;
                id->info.channels = channels;
                id->info.sampleRate = sampleRate;
                id->info.bitRate = bitRate;
                id->info.bytesPerFrame = bytesPerFrame;
            } else {
                LOG_ERROR(logType, "unimplemented fmt chunk length < 16");
                return false;
            }
            break;
        case WAVE_CHUNK_ID_FACT:
            if (length >= 8) {
                int endSample = *(uint32_t *) data;
                int sampleOffset;

                if (endSample > 0)
                    endSample -= 1;

                if (length >= 12) {
                    sampleOffset = *(uint32_t *) (data + 8);
                } else {
                    sampleOffset = *(uint32_t *) (data + 4);
                }

                id->info.endSample = endSample;
                id->info.sampleOffset = sampleOffset;
            } else {
                LOG_ERROR(logType, "unknown fact chunk size");
                return false;
            }
            break;
        case WAVE_CHUNK_ID_SMPL:
            LOG_WARN(logType, "unimplemented smpl chunk!");
            return false;
        case WAVE_CHUNK_ID_DATA:
            id->info.baseDataAddress = id->baseAddress + offset;
            id->info.dataSize = length;
            id->info.dataAddressEnd = id->info.baseDataAddress + length;
            id->info.currentDataAddress = id->info.baseDataAddress;
            break;
        default:
            LOG_ERROR(logType, "Unknown chunk magic %c%c%c%c' 0x%08x\n", identifier[0], identifier[1], identifier[2], identifier[3], length);
            return false;
        }

        chunkStart += length + 0x8;
    }
    return true;
}

int sceAtracAddStreamData(int atracID, uint32_t bytesToAdd) {
    printf("UNIMPLEMENTED %s", __FUNCTION__);
    std::exit(0);
    return 0;
}

int sceAtracSetHalfwayBufferAndGetID(uint8_t bufferPtr, uint32_t readByte, uint32_t bufferByte) {
    return 0;
}

int sceAtracSetDataAndGetID(uint32_t bufPtr, SceSize bufSize) {
    uint8_t *data = (uint8_t *) Memory::getPointer(bufPtr);
    if (!data) {
        LOG_ERROR(logType, "can't create a new Atrac3 context");
        return -1;
    }

    AtracID id {};
    id.data = data;
    id.baseAddress = bufPtr;
    id.initialBufferSize = bufSize;
    if (analyzeRIFF(&id) == false) {
        LOG_ERROR(logType, "can't create a new Atrac3 context");
        return 1;
    }

    atracList.push_back(id);
    LOG_WARN(logType, "sceAtracSetDataAndGetID(bufPtr: 0x%08x, bufSize: 0x%08x)", bufPtr, bufSize);
    return ++atracIDCount;
}

int sceAtracDecodeData(int atracID, uint32_t outSamplesPtr, uint32_t outNPtr, uint32_t outEndPtr, uint32_t outRemainFramePtr) {
    printf("UNIMPLEMENTED %s\n", __FUNCTION__);
    return -1;
}

int sceAtracGetNextSample(int atracID, uint32_t outNPtr) {
    const auto *id = &atracList[atracID];

    int nextSample;

    uint32_t *ptr = (uint32_t *) Memory::getPointer(outNPtr);

    if (id->info.currentDataAddress <= id->info.dataAddressEnd)
        nextSample = id->info.maxSamples;
    else
        nextSample = 0;

    if (ptr)
        *ptr = (uint32_t)nextSample;
    LOG_WARN(logType, "sceAtracGetNextSample()");
    return -1;
}

int sceAtracGetStreamDataInfo(int atracID, uint32_t writePointerPtr, uint32_t availableBytesPtr, uint32_t readOffsetPtr) {
    printf("UNIMPLEMENTED %s", __FUNCTION__);
    std::exit(0);
    return 0;
}

int sceAtracSetData(int atracID, uint32_t bufferPtr, uint32_t bufferByte) {
    printf("UNIMPLEMENTED %s", __FUNCTION__);
    std::exit(0);
    return 0;
}

int sceAtracGetRemainFrame(int atracID, uint32_t outRemainFramePtr) {
    if (!(atracID > 0 && atracID < atracList.size())) {
        LOG_WARN(logType, "invalid atrac ID");
        return -1;
    }

    const auto *id = &atracList[atracID];
    int remainingFrames = (id->info.dataAddressEnd - id->info.currentDataAddress) / id->info.bytesPerFrame;
 
    uint32_t *ptr = (uint32_t *) Memory::getPointer(outRemainFramePtr);
    if (ptr)
        *ptr = (uint32_t)remainingFrames;
    LOG_WARN(logType, "sceAtracGetRemainFrame()");
    return -1;
}
}