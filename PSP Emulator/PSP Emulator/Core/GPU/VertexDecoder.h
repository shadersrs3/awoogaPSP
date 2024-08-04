#pragma once

#include <cstdint>
#include <vector>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Core::GPU {
struct GPUState;

struct VertexData {
    glm::vec3 position;
    glm::vec2 uv;
    glm::vec4 color;
    glm::vec3 normal;
    float w[8];
};

std::vector<VertexData> *__getListFromVertexCache(const GPUState *state, int type, int count);
std::vector<VertexData> __decodeVertexList(const GPUState *state, int type, int count);
}