#pragma once

#include <vector>

#include <Core/GPU/GPU.h>

namespace Core::GPU {
struct TextureData {
    uint64_t timestamp;
    GPUState::TextureInfo textureInfo;
    int textureByteAlignment;
    uint64_t handle; // For OpenGL
    uint64_t key;
    bool isDirty;
    bool forceUpdate;
    uint32_t getTexturePixelType();
};

struct TextureCacheData {
    uint64_t handle;
    uint64_t key;
};

std::vector<TextureCacheData> __updateTextureCache(); // returns handles for host renderer destroying

std::vector<TextureData *> getTextureDataList();
void *getCurrentDecodedTexture(int level);

TextureData *__getTextureByKey(const uint64_t& key);
TextureData *__getTextureFromCache(const GPUState *state);
TextureData __decodeTexture(const GPUState *state, uint64_t key = 0);
}