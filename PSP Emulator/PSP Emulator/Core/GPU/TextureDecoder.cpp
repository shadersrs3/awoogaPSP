#include <Core/GPU/TextureDecoder.h>
#include <Core/GPU/GEConstants.h>

#include <Core/Logger.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Timing.h>

#include <GL/glew.h>

#include <vector>
#include <unordered_map>

namespace Core::GPU {
static const char *logType = "TextureDecoder";

std::unordered_map<uint64_t, TextureData> textureDataCache;

static uint8_t buffer[8][512 * 512 * 4];
static uint8_t swizzledBuffer[8][512 * 512 * 4];

static constexpr int textureUpdateTimeConstant() {
    return 30000;
}

std::vector<TextureCacheData> __updateTextureCache() {
    std::vector<TextureCacheData> data;
    
    for (auto it = textureDataCache.begin(); it != textureDataCache.end(); ) {
        TextureData *_data = &it->second;

        if (Core::Timing::getSystemTimeMilliseconds() >= _data->timestamp + textureUpdateTimeConstant()) {
            data.push_back(TextureCacheData { .handle = _data->handle, .key = _data->key });
            it = textureDataCache.erase(it);
            continue;
        }
        it++;
    }
    return data;
}

std::vector<TextureData *> getTextureDataList() {
    std::vector<TextureData *> _t;
    for (auto& i : textureDataCache)
        _t.push_back(&i.second);
    return _t;
}

static uint64_t getTextureHashedKey(const GPUState::TextureInfo *data) {
    uint64_t hash = 0;
    for (int i = 0; i < data->textureNumMipMaps; ++i) {
        uint32_t n = std::max(data->textureBufferWidth[i], data->textureWidth[i]) * data->textureHeight[i];
        switch (data->textureStorage) {
        case TPSM_PIXEL_STORAGE_MODE_16BIT_BGR5650:
        case TPSM_PIXEL_STORAGE_MODE_16BIT_ABGR5551:
        case TPSM_PIXEL_STORAGE_MODE_16BIT_ABGR4444:
        case TPSM_PIXEL_STORAGE_MODE_16BIT_INDEXED:
            n /= 2;
            break;
        case TPSM_PIXEL_STORAGE_MODE_32BIT_ABGR8888:
        case TPSM_PIXEL_STORAGE_MODE_32BIT_INDEXED:
        break;
        case TPSM_PIXEL_STORAGE_MODE_DXT1:
            n = ((data->textureWidth[i] + 3) / 4)*((data->textureWidth[i] + 3) / 4)*2;
            break;
        case TPSM_PIXEL_STORAGE_MODE_DXT3:
        case TPSM_PIXEL_STORAGE_MODE_DXT5:
            n = ((data->textureWidth[i] + 3) / 4)*((data->textureWidth[i] + 3) / 4)*4;
            break;
        case TPSM_PIXEL_STORAGE_MODE_4BIT_INDEXED:
            n /= 8;
            break;
        case TPSM_PIXEL_STORAGE_MODE_8BIT_INDEXED:
            n /= 4;
            break;
        }

        uint64_t *p = (uint64_t *) Memory::getPointerUnchecked(data->textureBasePointer[i]);
        if (!p) {
            LOG_ERROR(logType, "invalid hash key texture base pointer level %d addr 0x%08x", i, data->textureBasePointer[i]);
            return ~0uLL;
        }

        for (uint64_t *e = p + (n/2); p < e; p++) {
            hash += *p;
        }

        hash += data->textureStorage + data->clutMode + data->clutAddress + data->textureHeight[i] * data->textureWidth[i] + data->textureBasePointer[i];
    }
    return hash;
}

uint32_t TextureData::getTexturePixelType() {
    static uint32_t storage[4] = {
        GL_UNSIGNED_SHORT_5_6_5,
        GL_UNSIGNED_SHORT_1_5_5_5_REV,
        GL_UNSIGNED_SHORT_4_4_4_4_REV,
        GL_UNSIGNED_BYTE,
    };

    if (textureInfo.textureStorage >= 4 && textureInfo.textureStorage <= 7) {
        return storage[textureInfo.clutMode];
    } else if (textureInfo.textureStorage < 4) {
        return storage[textureInfo.textureStorage];
    } else
        LOG_ERROR(logType, "can't get texture pixel type %d", textureInfo.textureStorage);
    return GL_UNSIGNED_BYTE;
}

TextureData *__getTextureByKey(const uint64_t& key) {
    auto it = textureDataCache.find(key);
    return it != textureDataCache.end() ? &it->second : nullptr;
}

static bool requiredToUpdate(const TextureData *data) {
    return Core::Timing::getSystemTimeMilliseconds() >= data->timestamp + textureUpdateTimeConstant();
}

TextureData *__getTextureFromCache(const GPUState *state) {
    const GPUState::TextureInfo& info = state->textureInfo;

    if (!state->textureEnable || state->clearModeEnable)
        return nullptr;

    uint64_t key = getTextureHashedKey(&state->textureInfo);
    
    if (key == -1)
        return nullptr;

    TextureData data;

    auto it = textureDataCache.find(key);
    if (it != textureDataCache.end()) {
        it->second.forceUpdate = requiredToUpdate(&it->second);
        if (!it->second.isDirty && !it->second.forceUpdate) {
            it->second.timestamp += textureUpdateTimeConstant();
            if (!memcmp(&it->second.textureInfo, &info, sizeof info)) { // lazy 
                return &it->second;
            }
        }

        uint64_t oldHandle = it->second.handle;
        it->second.isDirty = true;
        textureDataCache.erase(it);
        // LOG_DEBUG(logType, "dirty 0x%016llx.texcache remaking...", key);

        data = __decodeTexture(state, key);
        if (data.isDirty == true) {
            LOG_ERROR(logType, "can't save texture again, key 0x%016llx invalid decoding", key);
            return nullptr;
        }
        
        data.handle = oldHandle;
        data.forceUpdate = true;
        textureDataCache[key] = data;
        return &textureDataCache[key];
    }

    data = __decodeTexture(state, key);
    if (data.isDirty == true) {
        LOG_ERROR(logType, "can't save texture, key 0x%016llx invalid decoding", key);
        return nullptr;
    }

    textureDataCache[key] = data;
    LOG_DEBUG(logType, "saved 0x%016llx.texcache", key);
    return &textureDataCache[key];
}

static inline uint32_t getCLUTIndex(uint32_t index, uint32_t csa, int sft, uint32_t msk) {
    return ((index >> sft) & msk) | (csa << 4);
}

static inline void __fastUnswizzle(uint8_t *out, const uint8_t *in, int32_t width, int32_t height) {
    int32_t blockx, blocky;
    int32_t j;

    int32_t width_blocks = (width / 16);
    int32_t height_blocks = (height / 8);

    int32_t dst_pitch = (width-16)/4;
    int32_t dst_row = width * 8;

    const uint32_t *src = (const uint32_t *)in;
    uint8_t *ydst = out;
    for (blocky = 0; blocky < height_blocks; ++blocky)
    {
        uint8_t *xdst = ydst;
        for (blockx = 0; blockx < width_blocks; ++blockx)
        {
            uint32_t *dst = (uint32_t *)xdst;
            for (j = 0; j < 8; ++j)
            {
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                *(dst++) = *(src++);
                dst += dst_pitch;
            }
            xdst += 16;
        }
        ydst += dst_row;
    }
}

template<int _Bpp>
static void __unswizzleTexture(const GPUState::TextureInfo& info, void *outBuffer, const void *inBuffer, int level) {
    switch (_Bpp) {
    case  4: __fastUnswizzle((uint8_t *)outBuffer, (const uint8_t *)inBuffer, info.textureBufferWidth[level]/2, info.textureHeight[level]); break;
    case  8: __fastUnswizzle((uint8_t *)outBuffer, (const uint8_t *)inBuffer, info.textureBufferWidth[level]  , info.textureHeight[level]); break;
    case 16: __fastUnswizzle((uint8_t *)outBuffer, (const uint8_t *)inBuffer, info.textureBufferWidth[level]*2, info.textureHeight[level]); break;
    case 32: __fastUnswizzle((uint8_t *)outBuffer, (const uint8_t *)inBuffer, info.textureBufferWidth[level]*4, info.textureHeight[level]); break;
    }
}

static bool decodeCLUT4(TextureData& data, const GPUState::TextureInfo& info) {
    for (int level = 0; level < info.textureNumMipMaps; level++) {
        if (info.clutAddress == 0) {
            return false;
        }

        void *inputPointer = Core::Memory::getPointerUnchecked(info.textureBasePointer[level]);

        if (!inputPointer) {
            LOG_ERROR(logType, "can't decode CLUT-4 texture base pointer!");
            return false;
        }

        void *outputPointer = buffer[level];

        switch (info.clutMode) {
        case CMODE_FORMAT_16BIT_BGR5650:
        case CMODE_FORMAT_16BIT_ABGR5551:
        case CMODE_FORMAT_16BIT_ABGR4444:
        {
            data.textureByteAlignment = 2;
            uint8_t *ip;
            uint16_t *cp;
            uint16_t *op = (uint16_t *) outputPointer;

            cp = (uint16_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't decode CLUT-4 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<4>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint8_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i += 2) {
                uint8_t index = *ip++;
                *op++ = cp[getCLUTIndex((index >> 0) & 0xF, pal, info.clutSft, info.clutMsk)];
                *op++ = cp[getCLUTIndex((index >> 4) & 0xF, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        case CMODE_FORMAT_32BIT_ABGR8888:
        {
            data.textureByteAlignment = 4;
            uint8_t *ip;
            uint32_t *cp;
            uint32_t *op = (uint32_t *) outputPointer;

            cp = (uint32_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-4 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<4>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint8_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i += 2) {
                uint8_t index = *ip++;
                *op++ = cp[getCLUTIndex((index >> 0) & 0xF, pal, info.clutSft, info.clutMsk)];
                *op++ = cp[getCLUTIndex((index >> 4) & 0xF, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        }
    }
    return true;
}

static bool decodeCLUT8(TextureData& data, const GPUState::TextureInfo& info) {
    for (int level = 0; level < info.textureNumMipMaps; level++) {
        if (info.clutAddress == 0) {
            return false;
        }

        void *inputPointer = Core::Memory::getPointerUnchecked(info.textureBasePointer[level]);

        if (!inputPointer) {
            LOG_ERROR(logType, "can't get CLUT-8 texture base pointer!");
            return false;
        }

        void *outputPointer = buffer[level];

        switch (info.clutMode) {
        case CMODE_FORMAT_16BIT_BGR5650:
        case CMODE_FORMAT_16BIT_ABGR5551:
        case CMODE_FORMAT_16BIT_ABGR4444:
        {
            data.textureByteAlignment = 2;
            uint8_t *ip;
            uint16_t *cp;
            uint16_t *op = (uint16_t *) outputPointer;

            cp = (uint16_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-8 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<8>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint8_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i++) {
                uint8_t index = *ip++;
                *op++ = cp[getCLUTIndex(index, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        case CMODE_FORMAT_32BIT_ABGR8888:
        {
            data.textureByteAlignment = 4;
            uint8_t *ip;
            uint32_t *cp;
            uint32_t *op = (uint32_t *) outputPointer;

            cp = (uint32_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-8 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<8>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint8_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i++) {
                uint8_t index = *ip++;
                *op++ = cp[getCLUTIndex(index, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        }
    }
    return true;
}

static bool decodeCLUT16(TextureData& data, const GPUState::TextureInfo& info) {
    for (int level = 0; level < info.textureNumMipMaps; level++) {
        if (info.clutAddress == 0) {
            return false;
        }

        void *inputPointer = Core::Memory::getPointerUnchecked(info.textureBasePointer[level]);

        if (!inputPointer) {
            LOG_ERROR(logType, "can't get CLUT-16 texture base pointer!");
            return false;
        }

        void *outputPointer = buffer[level];

        switch (info.clutMode) {
        case CMODE_FORMAT_16BIT_BGR5650:
        case CMODE_FORMAT_16BIT_ABGR5551:
        case CMODE_FORMAT_16BIT_ABGR4444:
        {
            data.textureByteAlignment = 2;
            uint16_t *ip;
            uint16_t *cp;
            uint16_t *op = (uint16_t *) outputPointer;

            cp = (uint16_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-16 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<16>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = (uint16_t *) swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint16_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i++) {
                uint16_t index = *ip++;
                *op++ = cp[getCLUTIndex(index, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        case CMODE_FORMAT_32BIT_ABGR8888:
        {
            data.textureByteAlignment = 4;
            uint16_t *ip;
            uint32_t *cp;
            uint32_t *op = (uint32_t *) outputPointer;

            cp = (uint32_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-16 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<16>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = (uint16_t *) swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint16_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i++) {
                uint16_t index = *ip++;
                *op++ = cp[getCLUTIndex(index, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        }
    }
    return true;
}

static bool decodeCLUT32(TextureData& data, const GPUState::TextureInfo& info) {
    for (int level = 0; level < info.textureNumMipMaps; level++) {
        if (info.clutAddress == 0) {
            return false;
        }

        void *inputPointer = Core::Memory::getPointerUnchecked(info.textureBasePointer[level]);

        if (!inputPointer) {
            LOG_ERROR(logType, "can't get CLUT-32 texture base pointer!");
            return false;
        }

        void *outputPointer = buffer[level];

        switch (info.clutMode) {
        case CMODE_FORMAT_16BIT_BGR5650:
        case CMODE_FORMAT_16BIT_ABGR5551:
        case CMODE_FORMAT_16BIT_ABGR4444:
        {
            data.textureByteAlignment = 2;
            uint32_t *ip;
            uint16_t *cp;
            uint16_t *op = (uint16_t *) outputPointer;

            cp = (uint16_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-32 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<32>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = (uint32_t *) swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint32_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i++) {
                uint32_t index = *ip++;
                *op++ = cp[getCLUTIndex(index, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        case CMODE_FORMAT_32BIT_ABGR8888:
        {
            data.textureByteAlignment = 4;
            uint32_t *ip;
            uint32_t *cp;
            uint32_t *op = (uint32_t *) outputPointer;

            cp = (uint32_t *) Core::Memory::getPointerUnchecked(info.clutAddress);
            if (!cp) {
                LOG_ERROR(logType, "can't get CLUT-32 clut palette!");
                return false;
            }

            if (info.textureSwizzle) {
                __unswizzleTexture<32>(info, (void *) swizzledBuffer[level], (void *) inputPointer, level);
                ip = (uint32_t *) swizzledBuffer[level];
                inputPointer = swizzledBuffer[level];
            } else {
                ip = (uint32_t *) inputPointer;
            }

            int length = info.textureBufferWidth[level] * info.textureHeight[level];

            uint32_t pal = info.clutCsa + (info.clutShared ? 0 : level);

            for (int i = 0; i < length; i++) {
                uint32_t index = *ip++;
                *op++ = cp[getCLUTIndex(index, pal, info.clutSft, info.clutMsk)];
            }
            break;
        }
        }
    }
    return true;
}

TextureData __decodeTexture(const GPUState *state, uint64_t key) {
    TextureData data{};

    if (!state->textureEnable || state->clearModeEnable) {
        data.isDirty = true;
        return data;
    }

    const GPUState::TextureInfo& info = state->textureInfo;
    switch (info.textureStorage) {
    case GE_TFMT_5650:
    case GE_TFMT_5551:
    case GE_TFMT_4444:
        data.textureByteAlignment = 2;
        for (int level = 0; level < info.textureNumMipMaps; level++) {
            void *inputPointer = Core::Memory::getPointerUnchecked(info.textureBasePointer[level]);
            if (!inputPointer) {
                LOG_ERROR(logType, "can't get GE_TMFT_4bpp texture base pointer!");
                data.isDirty = true;
                return data;
            }

            void *outputPointer = buffer[level];
            if (info.textureSwizzle) {
                __unswizzleTexture<32>(info, (void *) outputPointer, (void *) inputPointer, level);
            } else {
                int length = (info.textureBufferWidth[level] * info.textureHeight[level]) * sizeof(uint32_t);
                std::memcpy(outputPointer, inputPointer, length);
            }
        }
        break;
    case GE_TFMT_8888:
        data.textureByteAlignment = 4;
        for (int level = 0; level < info.textureNumMipMaps; level++) {
            void *inputPointer = Core::Memory::getPointerUnchecked(info.textureBasePointer[level]);
            if (!inputPointer) {
                LOG_ERROR(logType, "can't get GE_TMFT_8bpp texture base pointer!");
                data.isDirty = true;
                return data;
            }

            void *outputPointer = buffer[level];
            if (info.textureSwizzle) {
                __unswizzleTexture<32>(info, (void *) outputPointer, (void *) inputPointer, level);
            } else {
                int length = (info.textureBufferWidth[level] * info.textureHeight[level]) * sizeof(uint32_t);
                std::memcpy(outputPointer, inputPointer, length);
            }
        }
        break;
    case GE_TFMT_CLUT4:
        if (!decodeCLUT4(data, info)) {
            LOG_ERROR(logType, "can't decode CLUT4");
            data.isDirty = true;
            return data;
        }
        break;
    case GE_TFMT_CLUT8:
        if (!decodeCLUT8(data, info)) {
            LOG_ERROR(logType, "can't decode CLUT8");
            data.isDirty = true;
            return data;
        }
        break;
    case GE_TFMT_CLUT16:
        if (!decodeCLUT16(data, info)) {
            LOG_ERROR(logType, "can't decode CLUT16");
            data.isDirty = true;
            return data;
        }
        break;
    case GE_TFMT_CLUT32:
        if (!decodeCLUT32(data, info)) {
            LOG_ERROR(logType, "can't decode CLUT32");
            data.isDirty = true;
            return data;
        }
        break;

    case GE_TFMT_DXT1:
    case GE_TFMT_DXT3:
    case GE_TFMT_DXT5:
    default:
        data.isDirty = true;
        LOG_ERROR(logType, "Unimplemented texture storage 0x%02x!", info.textureStorage);
        data.textureByteAlignment = 0;
        return data;
    }

    data.key = key;
    data.timestamp = Core::Timing::getSystemTimeMilliseconds();
    data.handle = 0;
    data.textureInfo = state->textureInfo;
    data.handle = 0;
    data.isDirty = false;
    data.forceUpdate = true;

    return data;
}

void *getCurrentDecodedTexture(int level) {
    return buffer[level];
}
}