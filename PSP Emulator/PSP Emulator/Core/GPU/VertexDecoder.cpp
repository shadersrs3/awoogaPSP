#include <Core/GPU/GPU.h>
#include <Core/GPU/VertexDecoder.h>
#include <Core/GPU/GEConstants.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

#include <unordered_map>

namespace Core::GPU {
static const char *logType = "VertexDecoder";

struct VertexDataCache {
    int type;
    int count;
    uint32_t indexListAddress;
    std::vector<VertexData> data;
    GPUState::VertexInfo vertexInfo;

    bool isClean(const GPUState *state, int type, int count) {
        return this->type == type && this->count == count && state->indexListAddress == indexListAddress;
    }
};

static std::unordered_map<uint64_t, VertexDataCache> vertexCache;

static uint64_t getVertexKey(const GPUState *state, int type, int count) {
    const GPUState::VertexInfo& info = state->vertexInfo;
    uint64_t key = 0;
    uint64_t *ptr = nullptr, *ptrEnd = nullptr;

    if (state->vertexListAddress != 0) {
        ptr = (uint64_t *) Core::Memory::getPointerUnchecked(state->vertexListAddress);
        ptrEnd = ptr + ((count * info.vertex_size) / 8);
    } else {
        LOG_ERROR(logType, "can't get vertex key if vertex list is NULL");
        return ~0uLL;
    }

    int vertexCount = (count * info.vertex_size);
    for (; ptr < ptrEnd; ptr++) {
        key += *ptr;
    }

    key += type + count;
    return key;
}

std::vector<VertexData> *__getListFromVertexCache(const GPUState *state, int type, int count) {
    uint64_t key = 0;// getVertexKey(state, type, count);
    VertexDataCache cache;

    if (auto it = vertexCache.find(key); it != vertexCache.end()) {
        //return &it->second.data;
    }

    cache.data = __decodeVertexList(state, type, count);
    cache.type = type;
    cache.count = count;
    cache.indexListAddress = state->indexListAddress;
    cache.vertexInfo = state->vertexInfo;
    vertexCache[key] = cache;
    // LOG_DEBUG(logType, "saved 0x%016llx.vertcache", key);
    return &vertexCache[key].data;
}

static std::vector<VertexData> __triangulateRectangle(const std::vector<VertexData>& invertex) {
    std::vector<VertexData> outvertex;

    for (size_t i2 = 0; i2 < invertex.size(); i2 += 2) {
        size_t i1 = i2 + 1;
        size_t ix;
        size_t iy;

        const VertexData& v1 = invertex[i1];
        const VertexData& v2 = invertex[i2];

        if ((v2.position[0] - v1.position[0]) * (v2.position[1] - v1.position[1]) < 0) {
            ix = i2;
            iy = i1;
        } else {
            ix = i1;
            iy = i2;
        }

        const VertexData& vx = invertex[ix];
        const VertexData& vy = invertex[iy];
        VertexData v;

        v.uv[0] = vx.uv[0];
        v.uv[1] = vy.uv[1];
        v.position[0] = vx.position[0];
        v.position[1] = vy.position[1];
        v.position[2] = v2.position[2];
        v.color[0] = invertex[i1].color[0];
        v.color[1] = invertex[i1].color[1];
        v.color[2] = invertex[i1].color[2];
        v.color[3] = invertex[i1].color[3];

        outvertex.push_back(v);

        v.uv[0] = v1.uv[0];
        v.uv[1] = v1.uv[1];
        v.position[0] = v1.position[0];
        v.position[1] = v1.position[1];
        v.position[2] = v2.position[2];
        v.color[0] = invertex[i1].color[0];
        v.color[1] = invertex[i1].color[1];
        v.color[2] = invertex[i1].color[2];
        v.color[3] = invertex[i1].color[3];

        outvertex.push_back(v);

        v.uv[0] = v2.uv[0];
        v.uv[1] = v2.uv[1];
        v.position[0] = v2.position[0];
        v.position[1] = v2.position[1];
        v.position[2] = v2.position[2];
        v.color[0] = invertex[i1].color[0];
        v.color[1] = invertex[i1].color[1];
        v.color[2] = invertex[i1].color[2];
        v.color[3] = invertex[i1].color[3];

        outvertex.push_back(v);

        v.uv[0] = vy.uv[0];
        v.uv[1] = vx.uv[1];
        v.position[0] = vy.position[0];
        v.position[1] = vx.position[1];
        v.position[2] = v2.position[2];
        v.color[0] = invertex[i1].color[0];
        v.color[1] = invertex[i1].color[1];
        v.color[2] = invertex[i1].color[2];
        v.color[3] = invertex[i1].color[3];

        outvertex.push_back(v);
    }
    return outvertex;
}

std::vector<VertexData> __decodeVertexList(const GPUState *state, int type, int count) {
    std::vector<VertexData> _data;
    const GPUState::VertexInfo& vertexInfo = state->vertexInfo;

    float morphingWeights[8];
    std::memcpy(morphingWeights, state->morphingWeights, sizeof state->morphingWeights);

    if (vertexInfo.mc == 1 && !vertexInfo.wt) {
        morphingWeights[0] = 1.0f;
    }

    for (int i = 0; i < 8; i++) {
        if (morphingWeights[i] == 0.f)
            morphingWeights[i] = 1.f;
    }

    typedef int32_t s32;
    typedef uint32_t u32;
    typedef uint16_t u16;
    typedef int16_t s16;
    typedef uint8_t u8;
    typedef int8_t s8;
    typedef float f32;

    char *inVertices = (char *) Core::Memory::getPointerUnchecked(state->vertexListAddress);
    if (!inVertices) {
        LOG_ERROR(logType, "can't get vertex pointer from list address 0x%08x!", state->vertexListAddress);
        return _data;
    }

    void *inIndices = nullptr;
    if (state->indexListAddress != 0) {
        inIndices = Core::Memory::getPointerUnchecked(state->indexListAddress);
    }

    for (int i = 0; i < count; i++) {
        VertexData data {};
        int current_address = i;
        if (inIndices != nullptr) {
            switch (vertexInfo.it) {
            case 1:
                current_address = ((u8 *)inIndices)[i];
                break;
            case 2:
                current_address = ((u16 *)inIndices)[i];
                break;
            case 3:
                current_address = ((u32*)inIndices)[i];
                break;
            }
        }

        char *vertex_address = ((char *)inVertices) + (vertexInfo.vertex_size * current_address);

        // weights
        if (!vertexInfo.tm && vertexInfo.wt)
        {
            switch (vertexInfo.wt)
            {
            case 1:
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                u8 *weightData = (u8 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size));
                for (u32 skinCounter = 0; skinCounter < vertexInfo.wc; skinCounter++)
                {
                    data.w[skinCounter] += (f32(weightData[skinCounter]) / 128.0f) * morphingWeights[morphcounter];
                }
            }
            break;
            case 2:
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                u16 *weightData = (u16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size));
                for (u32 skinCounter = 0; skinCounter < vertexInfo.wc; skinCounter++)
                {
                    data.w[skinCounter] += (f32(weightData[skinCounter]) / 32768.0f) * morphingWeights[morphcounter];
                }
            }
            break;
            case 3:
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                f32 *weightData = (f32 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size));
                for (u32 skinCounter = 0; skinCounter < vertexInfo.wc; skinCounter++)
                {
                    data.w[skinCounter] += weightData[skinCounter] * morphingWeights[morphcounter];
                }
            }
            break;
            }
        }

        // texture
        if (vertexInfo.tt) {
            if (vertexInfo.tm) {
                switch (vertexInfo.tt) {
                case 1:
                {
                    u8 *uvdata = (u8 *)(vertex_address + vertexInfo.texture_offset);
                    data.uv[0] = f32(uvdata[0]);
                    data.uv[1] = f32(uvdata[1]);
                    break;
                }
                case 2:
                {
                    u16 *uvdata = (u16 *)(vertex_address + vertexInfo.texture_offset);
                    data.uv[0] = f32(uvdata[0]);
                    data.uv[1] = f32(uvdata[1]);
                    break;
                }
                case 3:
                {
                    f32 *uvdata = (f32 *)(vertex_address + vertexInfo.texture_offset);
                    data.uv[0] = uvdata[0];
                    data.uv[1] = uvdata[1];
                    break;
                }
                }
            } else {
                f32 morphUV[2] {};
                switch (vertexInfo.tt)
                {
                case 1:
                    for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++) {
                        u8 *uvdata = (u8 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.texture_offset);
                        morphUV[0] += (f32(uvdata[0]) / 128.0f) * morphingWeights[morphcounter];
                        morphUV[1] += (f32(uvdata[1]) / 128.0f) * morphingWeights[morphcounter];
                    }
                    break;
                case 2:
                    for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++) {
                        u16 *uvdata = (u16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.texture_offset);
                        morphUV[0] += (f32(uvdata[0]) / 32768.0f) * morphingWeights[morphcounter];
                        morphUV[1] += (f32(uvdata[1]) / 32768.0f) * morphingWeights[morphcounter];
                    }
                    break;
                case 3:
                    for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++) {
                        f32 *uvdata = (f32 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.texture_offset);
                        morphUV[0] += uvdata[0] * morphingWeights[morphcounter];
                        morphUV[1] += uvdata[1] * morphingWeights[morphcounter];
                    }
                    break;
                }
                data.uv[0] = morphUV[0];
                data.uv[1] = morphUV[1];
            }
        }

        // color
        if (vertexInfo.ct) {
            switch(vertexInfo.ct) {
            case 1:
            case 2:
            case 3:
            break;
            case 4: // GU_COLOR_5650
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                u16 current_color = *(u16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.color_offset);
                f32 r,g,b;

                r = f32(current_color>>( 0) & 0x1f) / 31.0f;
                g = f32(current_color>>( 5) & 0x3f) / 63.0f;
                b = f32(current_color>>(11) & 0x1f) / 31.0f;

                data.color[0] += r * morphingWeights[morphcounter];
                data.color[1] += g * morphingWeights[morphcounter];
                data.color[2] += b * morphingWeights[morphcounter];
                data.color[3] += morphingWeights[morphcounter];
            }
            case 5: // GU_COLOR_5551
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                u16 current_color = *(u16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.color_offset);
                f32 r,g,b,a;

                r = f32(current_color>>(0*5) & 0x1f) / 31.0f;
                g = f32(current_color>>(1*5) & 0x1f) / 31.0f;
                b = f32(current_color>>(2*5) & 0x1f) / 31.0f;
                a = f32(current_color>>(3*5));

                data.color[0] += r * morphingWeights[morphcounter];
                data.color[1] += g * morphingWeights[morphcounter];
                data.color[2] += b * morphingWeights[morphcounter];
                data.color[3] += a * morphingWeights[morphcounter];
            }
            case 6: // GU_COLOR_4444
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                u16 current_color = *(u16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.color_offset);
                float r,g,b,a;

                r = f32(current_color>>(0*4) & 0xF) / 15.0f;
                g = f32(current_color>>(1*4) & 0xF) / 15.0f;
                b = f32(current_color>>(2*4) & 0xF) / 15.0f;
                a = f32(current_color>>(3*4) & 0xF) / 15.0f;

                data.color[0] += r * morphingWeights[morphcounter];
                data.color[1] += g * morphingWeights[morphcounter];
                data.color[2] += b * morphingWeights[morphcounter];
                data.color[3] += a * morphingWeights[morphcounter];
            }
            break;
            case 7: // GU_COLOR_8888
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                u32 current_color = *(u32 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.color_offset);
                f32 r,g,b,a;

                r = f32(current_color>>(0*8) & 0xff) / 255.0f;
                g = f32(current_color>>(1*8) & 0xff) / 255.0f;
                b = f32(current_color>>(2*8) & 0xff) / 255.0f;
                a = f32(current_color>>(3*8) & 0xff) / 255.0f;

                data.color[0] += r * morphingWeights[morphcounter];
                data.color[1] += g * morphingWeights[morphcounter];
                data.color[2] += b * morphingWeights[morphcounter];
                data.color[3] += a * morphingWeights[morphcounter];
            }
            break;
            }
        }


        // normals
        if (!vertexInfo.tm && vertexInfo.nt)
        {
            float morphNormals[3] = { 0.0f, 0.0f, 0.0f};
            switch (vertexInfo.nt)
            {
            case 1:
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                s8 *float_normals = (s8 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.normal_offset);

                morphNormals[0] += f32(float_normals[0]) / 127.0f * morphingWeights[morphcounter];
                morphNormals[1] += f32(float_normals[1]) / 127.0f * morphingWeights[morphcounter];
                morphNormals[2] += f32(float_normals[2]) / 127.0f * morphingWeights[morphcounter];
            }
            break;
            case 2:
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                s16 *float_normals = (s16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.normal_offset);

                morphNormals[0] += f32(float_normals[0]) / 32767.0f * morphingWeights[morphcounter];
                morphNormals[1] += f32(float_normals[1]) / 32767.0f * morphingWeights[morphcounter];
                morphNormals[2] += f32(float_normals[2]) / 32767.0f * morphingWeights[morphcounter];
            }
            break;
            case 3:
            for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
            {
                f32 *float_normals = (f32 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.normal_offset);

                morphNormals[0] += f32(float_normals[0]) * morphingWeights[morphcounter];
                morphNormals[1] += f32(float_normals[1]) * morphingWeights[morphcounter];
                morphNormals[2] += f32(float_normals[2]) * morphingWeights[morphcounter];
            }
            break;
            }
            data.normal[0] = morphNormals[0];
            data.normal[1] = morphNormals[1];
            data.normal[2] = morphNormals[2];
        }

        // position
        if (vertexInfo.vt)
        {
            if (vertexInfo.tm)
            {
                switch (vertexInfo.vt)
                {
                case 1:
                {
                    u8 *current_vertex = (u8 *)(vertex_address + vertexInfo.position_offset);
                    data.position[0] = f32(s32(s8(current_vertex[0])));
                    data.position[1] = f32(s32(s8(current_vertex[1])));
                    data.position[2] = f32(u32(u8(current_vertex[2])));
                }
                break;
                case 2:
                {
                    u16 *current_vertex = (u16 *)(vertex_address + vertexInfo.position_offset);
                    data.position[0] = f32(s32(s16(current_vertex[0])));
                    data.position[1] = f32(s32(s16(current_vertex[1])));
                    data.position[2] = f32(u32(u16(current_vertex[2])));
                }
                break;
                case 3:
                {
                    f32 *current_vertex = (f32 *)(vertex_address + vertexInfo.position_offset);
                    data.position[0] = current_vertex[0];
                    data.position[1] = current_vertex[1];
                    data.position[2] = current_vertex[2];
                }
                break;
                }
            }
            else
            {
                float morphVerts[3] = { 0.0f, 0.0f, 0.0f };
                switch (vertexInfo.vt)
                {
                case 1:
                for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
                {
                    s8 *current_vertex = (s8 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.position_offset);

                    morphVerts[0] += f32(current_vertex[0]) / 127.0f * morphingWeights[morphcounter];
                    morphVerts[1] += f32(current_vertex[1]) / 127.0f * morphingWeights[morphcounter];
                    morphVerts[2] += f32(current_vertex[2]) / 127.0f * morphingWeights[morphcounter];
                }
                break;
                case 2:
                for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
                {
                    s16 *current_vertex = (s16 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.position_offset);
                    {
                        morphVerts[0] += f32(current_vertex[0]) / 32767.0f * morphingWeights[morphcounter];
                        morphVerts[1] += f32(current_vertex[1]) / 32767.0f * morphingWeights[morphcounter];
                        morphVerts[2] += f32(current_vertex[2]) / 32767.0f * morphingWeights[morphcounter];
                    }
                }
                break;
                case 3:
                for (u32 morphcounter = 0; morphcounter < vertexInfo.mc; morphcounter++)
                {
                    f32 *current_vertex = (f32 *)(vertex_address + (morphcounter * vertexInfo.one_vertex_size) + vertexInfo.position_offset);

                    morphVerts[0] += current_vertex[0] * morphingWeights[morphcounter];
                    morphVerts[1] += current_vertex[1] * morphingWeights[morphcounter];
                    morphVerts[2] += current_vertex[2] * morphingWeights[morphcounter];
                }
                break;
                }
                data.position[0] = morphVerts[0];
                data.position[1] = morphVerts[1];
                data.position[2] = morphVerts[2];
            }
        }

        _data.push_back(data);
    }

    if (type == GE_PRIM_RECTANGLES)
        _data = __triangulateRectangle(_data);

    return _data;
}
}