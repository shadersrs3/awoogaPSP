#if defined (_MSC_VER) || defined(_WIN32) || defined (_WIN64)
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <memory>
#include <iostream>
#include <fstream>
#include <algorithm>

#include <Core/GPU/GPU.h>
#include <Core/GPU/Renderer.h>
#include <Core/GPU/GEConstants.h>
#include <Core/GPU/VertexDecoder.h>
#include <Core/GPU/TextureDecoder.h>

#include <Core/Memory/MemoryAccess.h>

#include <GL/glew.h>
#include <GL/GL.h>

#include <Core/Logger.h>

namespace Core::GPU {
static const char *logType = "Renderer";
struct RenderDeviceNone : public RenderDevice {
private:

public:
    RenderDeviceNone();
    ~RenderDeviceNone();
    void displayListBegin() override {}
    void displayListEnd() override {}
    void prepareDraw(GPUState *state, int type, int count) override;
    void drawPrimitive(GPUState *state, int type, int count) override;
    void endDraw(GPUState *state) override;

    const char *getDeviceName() override { return "None"; }
    virtual RendererType getDeviceType() override { return RENDERER_TYPE_NONE; }
};

RenderDeviceNone::RenderDeviceNone() {

}

RenderDeviceNone::~RenderDeviceNone() {

}

void RenderDeviceNone::prepareDraw(GPUState *state, int type, int count) {

}

void RenderDeviceNone::drawPrimitive(GPUState *state, int primitiveType, int count) {

}

void RenderDeviceNone::endDraw(GPUState *state) {

}

static std::shared_ptr<uint8_t> loadShader(const char *file) {
    std::ifstream inputFile(file, std::ios::binary);
    int fileSize;

    std::shared_ptr<uint8_t> buffer;

    if (inputFile.is_open()) {
        inputFile.seekg(0, std::ios::end);
        fileSize = (int)inputFile.tellg();
        inputFile.seekg(0, std::ios::beg);

        buffer = std::shared_ptr<uint8_t>(new uint8_t[fileSize + 2], [](uint8_t *ptr) { if (ptr) { delete[] ptr; } });

        buffer.get()[fileSize] = 0;
        inputFile.read((char *)buffer.get(), fileSize);
        inputFile.close();
    } else {
        LOG_ERROR(logType, "error opening shader file %s", file);
        return nullptr;
    }

    return buffer;
}

static bool createShader(uint32_t& program, const char *vertexShaderFile, const char *fragmentShaderFile) {
    uint32_t vertexShader, fragmentShader;
    bool status = false;

    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);

    std::shared_ptr<uint8_t> buf = loadShader(vertexShaderFile);

    if (!buf)
        return false;

    const char *source = (const char *)buf.get();

    glShaderSource(vertexShader, 1, &source, nullptr);
    glCompileShader(vertexShader);
    {
        int  success;
        char infoLog[512];
        glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
            LOG_ERROR(logType, "Vertex Shader failed: %s\n", infoLog);
            status = false;
            goto done;
        }
    }
    buf = loadShader(fragmentShaderFile);

    source = (const char *)buf.get();

    glShaderSource(fragmentShader, 1, &source, nullptr);
    glCompileShader(fragmentShader);
    {
        int  success;
        char infoLog[512];
        glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

        if (!success)
        {
            glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
            LOG_ERROR(logType, "Fragment Shader Failed: %s", infoLog);
            status = false;
            goto done;
        }
    }

    program = glCreateProgram();

    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    {
        int  success;
        char infoLog[512];

        glGetProgramiv(program, GL_LINK_STATUS, &success);
        if (!success) {
            glGetProgramInfoLog(program, 512, NULL, infoLog);
            LOG_ERROR(logType, "Link Failed: %s", infoLog);
            goto done;
        }
    }

    status = true;

done:
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
    return status;
}

struct RenderDeviceOpenGL : public RenderDevice {
public:
    std::vector<VertexData> *_vertexData;
    std::vector<int> indices;
    TextureData *_textureData;

    GLuint m_VBO, m_VAO, m_EBO;
    GLuint m_FramebufferVAO, m_FramebufferVBO;
    GLuint m_FramebufferObject, m_RenderBufferObject, m_FramebufferTexture;

    GLuint m_StreamingTextureHandle;
    // uint32_t vertexOperationFlags, pixelOperationFlags;
    bool streamingTexture;
    GLint useTextureIndex;
    GLint uProjectionIndex, uViewIndex, uWorldIndex, uBoneIndex;
    GLint uWeightCountIndex;
    GLint materialAmbientEnableIndex, materialDiffuseEnableIndex, materialSpecularEnableIndex;
    GLint materialAmbientIndex, materialDiffuseIndex, materialSpecularIndex;
    GLint textureScaleUIndex, textureScaleVIndex;
    GLint textureOffsetUIndex, textureOffsetVIndex;
    GLint skinningEnable;
    uint32_t throughModeVertexProcessor, normalModeVertexProcessor;
    uint32_t throughModeFragmentProcessor, normalModeFragmentProcessor;
    uint32_t m_Program, m_FramebufferProgram;
    bool validOpenGLState;
    bool __prepareDraw;
public:
    RenderDeviceOpenGL();
    ~RenderDeviceOpenGL();
    void displayListBegin() override;
    void displayListEnd() override;
    void prepareDraw(GPUState *state, int type, int count) override;
    void drawPrimitive(GPUState *state, int type, int count) override;
    void endDraw(GPUState *state) override;

    const char *getDeviceName() override { return "OpenGL"; }
    virtual RendererType getDeviceType() override { return RENDERER_TYPE_OPENGL; }
};

void __openglDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* userParam) {
    LOG_ERROR(logType, "an error has occurred with OpenGL: %d %s", source, message);
}

RenderDeviceOpenGL::RenderDeviceOpenGL() {
    _vertexData = nullptr;
    _textureData = nullptr;

    m_Program = 0;
    m_FramebufferProgram = 0;
    m_VBO = 0;
    m_VAO = 0;
    m_EBO = 0;
    m_FramebufferVAO = 0;
    m_FramebufferVBO = 0;
    m_FramebufferObject = 0;
    m_RenderBufferObject = 0;
    m_FramebufferTexture = 0;
    throughModeVertexProcessor = 0;
    normalModeVertexProcessor = 0;
    throughModeFragmentProcessor = 0;
    normalModeFragmentProcessor = 0;
    streamingTexture = false;
    m_StreamingTextureHandle = 0;
    __prepareDraw = false;

    useTextureIndex = 0;
    uProjectionIndex = uViewIndex = uWorldIndex = uBoneIndex = uWeightCountIndex = 0;
    materialAmbientEnableIndex = materialDiffuseEnableIndex = materialSpecularEnableIndex = 0;
    materialAmbientIndex = materialDiffuseIndex = materialSpecularIndex = 0;
    textureScaleUIndex = 0;
    textureScaleVIndex = 0;
    textureOffsetUIndex = 0;
    textureOffsetVIndex = 0;
    skinningEnable = 0;

    if (glGetString(GL_VENDOR) == nullptr) {
        validOpenGLState = false;
        return;
    }
    validOpenGLState = true;
    if (createShader(m_Program, "Shaders/NTMVertexShader.glsl", "Shaders/NTMFragmentShader.glsl") != false)
        LOG_SUCCESS(logType, "successfully created normal/through mode shader");
    else
        LOG_ERROR(logType, "an error has occured while processing normal/through mode shader");

    if (createShader(m_FramebufferProgram, "Shaders/FramebufferVertexShader.glsl", "Shaders/FramebufferFragmentShader.glsl") != false)
        LOG_SUCCESS(logType, "successfully created framebuffer shader");
    else
        LOG_ERROR(logType, "an error has occured while processing framebuffer shader");

    throughModeVertexProcessor = glGetSubroutineIndex(m_Program, GL_VERTEX_SHADER, "throughModeVertexProcessor");
    throughModeFragmentProcessor = glGetSubroutineIndex(m_Program, GL_FRAGMENT_SHADER, "throughModeFragmentProcessor");

    normalModeVertexProcessor = glGetSubroutineIndex(m_Program, GL_VERTEX_SHADER, "normalModeVertexProcessor");
    normalModeFragmentProcessor = glGetSubroutineIndex(m_Program, GL_FRAGMENT_SHADER, "normalModeFragmentProcessor");

    useTextureIndex = glGetUniformLocation(m_Program, "_useTexture");
    uProjectionIndex = glGetUniformLocation(m_Program, "uProjection");
    uViewIndex = glGetUniformLocation(m_Program, "uView");
    uWorldIndex = glGetUniformLocation(m_Program, "uWorld");
    uBoneIndex = glGetUniformLocation(m_Program, "uBone");
    uWeightCountIndex = glGetUniformLocation(m_Program, "uWeightCount");
    materialAmbientEnableIndex = glGetUniformLocation(m_Program, "materialAmbientEnable");
    materialDiffuseEnableIndex = glGetUniformLocation(m_Program, "materialDiffuseEnable");
    materialSpecularEnableIndex = glGetUniformLocation(m_Program, "materialSpecularEnable");
    materialAmbientIndex = glGetUniformLocation(m_Program, "materialAmbient");
    materialDiffuseIndex = glGetUniformLocation(m_Program, "materialDiffuse");
    materialSpecularIndex = glGetUniformLocation(m_Program, "materialSpecular");
    textureScaleUIndex = glGetUniformLocation(m_Program, "textureScaleU");
    textureScaleVIndex = glGetUniformLocation(m_Program, "textureScaleV");
    textureOffsetUIndex = glGetUniformLocation(m_Program, "textureOffsetU");
    textureOffsetVIndex = glGetUniformLocation(m_Program, "textureOffsetV");
    skinningEnable = glGetUniformLocation(m_Program, "skinningEnable");

    LOG_DEBUG(logType, "(vertex, fragment) Through Mode subroutine %d %d index Normal Mode subroutine index %d %d",
                throughModeVertexProcessor, throughModeFragmentProcessor, normalModeVertexProcessor, normalModeFragmentProcessor);

    glGenTextures(1, &m_StreamingTextureHandle);

    glGenVertexArrays(1, &m_VAO);
    glGenBuffers(1, &m_VBO);
    glGenBuffers(1, &m_EBO);

    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offsetof(VertexData, position));

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offsetof(VertexData, uv));
    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offsetof(VertexData, color));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offsetof(VertexData, normal));
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)offsetof(VertexData, w));
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(VertexData), (const void *)(offsetof(VertexData, w) + 4 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);
    glEnableVertexAttribArray(3);
    glEnableVertexAttribArray(4);
    glEnableVertexAttribArray(5);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenVertexArrays(1, &m_FramebufferVAO);
    glGenBuffers(1, &m_FramebufferVBO);
    glBindVertexArray(m_FramebufferVAO);

    struct FramebufferVertex {
        glm::vec3 position;
        glm::vec2 uv;
    };

    static FramebufferVertex vertices[] = {
        // top left
        { { -1.0f,  1.0f, 0.0f, }, { 0.f, 1.f } },
        { { 1.0f, 1.0f, 0.0f, }, { 1.f, 1.f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.f, 0.f } },
        // bottom right
        { { 1.0f,  1.0f, 0.0f, }, { 1.f, 1.f } },
        { { 1.0f, -1.0f, 0.0f, }, { 1.f, 0.f } },
        { { -1.0f, -1.0f, 0.0f }, { 0.f, 0.f } },
    };

    glBindBuffer(GL_ARRAY_BUFFER, m_FramebufferVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(FramebufferVertex), (const void *) offsetof(FramebufferVertex, position));
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FramebufferVertex), (const void *) offsetof(FramebufferVertex, uv));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glGenFramebuffers(1, &m_FramebufferObject);
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferObject);
    glGenTextures(1, &m_FramebufferTexture);
    glBindTexture(GL_TEXTURE_2D, m_FramebufferTexture);

    static std::vector<uint8_t> data;
    data.resize((480 * 3) * (272 * 3) * 4, 0x00);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 480*2, 272*2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &data[0]);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_FramebufferTexture, 0);

    glGenRenderbuffers(1, &m_RenderBufferObject);
    glBindRenderbuffer(GL_RENDERBUFFER, m_RenderBufferObject);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, 480*2, 272*2);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_RenderBufferObject);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

#if 0
    glEnable(GL_DEBUG_OUTPUT);
    glDebugMessageCallback(__openglDebugCallback, nullptr);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif
}

RenderDeviceOpenGL::~RenderDeviceOpenGL() {
    if (glGetString(GL_VENDOR) == nullptr)
        return;

    for (auto& i : getTextureDataList()) {
        if (i->handle)
            glDeleteTextures(1, (GLuint *)&i->handle);
    }

    glDeleteProgram(m_Program);
    glDeleteProgram(m_FramebufferProgram);

    glDeleteVertexArrays(1, &m_VAO);
    glDeleteBuffers(1, &m_VBO);

    glDeleteVertexArrays(1, &m_FramebufferVAO);
    glDeleteBuffers(1, &m_FramebufferVBO);
    glDeleteRenderbuffers(1, &m_RenderBufferObject);
    glDeleteFramebuffers(1, &m_FramebufferObject);
    glDeleteTextures(1, &m_FramebufferTexture);
}

void RenderDeviceOpenGL::displayListBegin() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferObject);
    glUseProgram(m_Program);
    glBindVertexArray(m_VAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_VBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_EBO);
}

void RenderDeviceOpenGL::displayListEnd() {
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glUseProgram(0);
}

// v and vecOut must point to different memory.
inline void Vec3ByMatrix43(float vecOut[3], const float v[3], const float m[12]) {
    vecOut[0] = v[0] * m[0] + v[1] * m[3] + v[2] * m[6] + m[9];
    vecOut[1] = v[0] * m[1] + v[1] * m[4] + v[2] * m[7] + m[10];
    vecOut[2] = v[0] * m[2] + v[1] * m[5] + v[2] * m[8] + m[11];
}

static void __calculateSkinningPosition(GPUState *state, glm::vec3& _pos, float *w) {
    float x = 0.f, y = 0.f, z = 0.f;
    int wc = ((state->vertexInfo.param >> 14) & 7) + 1;

    GE_Matrix4x4 skinMatrix[8];
    for (int i = 0; i < wc; i++)
    {
        float weight = w[i];
        if (weight != 0.f) {
            GE_Matrix4x4& matrix = state->boneMatrix[i];

            x += (	_pos.x * matrix.mData[0]
                + 	_pos.y * matrix.mData[4]
                + 	_pos.z * matrix.mData[8]
                +           matrix.mData[12]) * weight;

            y += (	_pos.x * matrix.mData[1]
                + 	_pos.y * matrix.mData[5]
                + 	_pos.z * matrix.mData[9]
                +           matrix.mData[13]) * weight;

            z += (	_pos.x * matrix.mData[2]
                + 	_pos.y * matrix.mData[6]
                + 	_pos.z * matrix.mData[10]
                +           matrix.mData[14]) * weight;
        }
    }

    _pos.x = x;
    _pos.y = y;
    _pos.z = z;
}

void RenderDeviceOpenGL::prepareDraw(GPUState *state, int type, int count) {
    if (!validOpenGLState)
        return;

    __prepareDraw = true;
    bool throughMode = false;

    float viewportWidth =  2.0f * state->viewportXScale;
    float viewportHeight = -2.0f * state->viewportYScale;
    float viewportX = state->viewportXCenter - state->screenOffsetX - viewportWidth / 2.f;
    float viewportY = 272.0f - (state->viewportYCenter - state->viewportYCenter) + state->viewportYScale * 2;
    // glViewport((int)viewportX, (int)viewportY, (GLsizei)viewportWidth * 2, (GLsizei)viewportHeight * 2);

    throughMode = state->vertexInfo.tm != 0;

    uint32_t currentVertexShaderSubroutine;
    uint32_t currentFragmentShaderSubroutine;

    if (throughMode) {
        currentVertexShaderSubroutine = throughModeVertexProcessor;
        currentFragmentShaderSubroutine = throughModeFragmentProcessor;
    } else {
        currentVertexShaderSubroutine = normalModeVertexProcessor;
        currentFragmentShaderSubroutine = normalModeFragmentProcessor;
    }

    glUniformSubroutinesuiv(GL_VERTEX_SHADER, 1, &currentVertexShaderSubroutine);
    glUniformSubroutinesuiv(GL_FRAGMENT_SHADER, 1, &currentFragmentShaderSubroutine);

    if (_vertexData) {
        auto& vertexData = *_vertexData;

        static VertexData vertices[] = {
            // bottom right
            { glm::vec3(480.f, 272.f, 0.f), glm::vec2(1, 1), glm::vec4(1, 0, 0, 1) },
            { glm::vec3(480.f, .0f, 0.f), glm::vec2(1, 0), glm::vec4(0, 1, 0, 1) },
            { glm::vec3(0.f, 272.f, 0.f), glm::vec2(0, 1), glm::vec4(0, 0, 1, 1) },
            // top left
            { glm::vec3(0.f, .0f, 0.f), glm::vec2(0, 0), glm::vec4(1, 0, 0, 1) },
            { glm::vec3(480.f, .0f, 0.f), glm::vec2(1, 0), glm::vec4(0, 1, 0, 1) },
            { glm::vec3(0.f, 272.f, 0.f), glm::vec2(0, 1), glm::vec4(0, 0, 1, 1) },
        };

        if (throughMode) {
            if (type == GE_PRIM_RECTANGLES) {
                indices.clear();

                for (int i = 0; i < vertexData.size(); i += 4) {
                    indices.push_back(i + 0);
                    indices.push_back(i + 1);
                    indices.push_back(i + 2);
                    indices.push_back(i + 2);
                    indices.push_back(i + 1);
                    indices.push_back(i + 3);
                }

                glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(uint32_t), &indices[0], GL_STATIC_DRAW);
            }

            glUniform1i(glGetUniformLocation(m_Program, "throughMode"), 1);
            glUniform1f(glGetUniformLocation(m_Program, "textureScaleX"), (float) state->textureInfo.textureWidth[0]);
            glUniform1f(glGetUniformLocation(m_Program, "textureScaleY"), (float) state->textureInfo.textureHeight[0]);
        } else {
            glUniform1i(glGetUniformLocation(m_Program, "throughMode"), 0);
        }

        if ((state->vertexInfo.param & GE_VTYPE_WEIGHT_MASK) != 0) {
            for (int i = 0; i < count; i++)
                __calculateSkinningPosition(state, vertexData[i].position, vertexData[i].w);
        }

        glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(VertexData), &vertexData[0], GL_STATIC_DRAW);

        // glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), &vertices[0], GL_STATIC_DRAW);

        if (state->matrixUpdated) {
            glUniformMatrix4fv(uProjectionIndex, 1, GL_FALSE, &state->projectionMatrix.mData[0]);
            glUniformMatrix4fv(uViewIndex, 1, GL_FALSE, &state->viewMatrix.mData[0]);
            glUniformMatrix4fv(uWorldIndex, 1, GL_FALSE, &state->worldMatrix.mData[0]);
            glUniformMatrix4fv(uBoneIndex, 1, GL_FALSE, &state->boneMatrix->mData[0]);
            state->matrixUpdated = false;
        }
    }

    if (_textureData) {
        static uint32_t minFilter[] = { GL_NEAREST, GL_LINEAR, 0, 0, GL_NEAREST_MIPMAP_NEAREST, GL_LINEAR_MIPMAP_NEAREST, GL_NEAREST_MIPMAP_LINEAR, GL_LINEAR_MIPMAP_LINEAR };
        auto applyTextureFilters = [&]() {
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, state->textureWrapModeS ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, state->textureWrapModeT ? GL_CLAMP_TO_EDGE : GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, state->textureMagFilter ? GL_LINEAR : GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, minFilter[state->textureMinFilter]);
            // TODO
        };

        auto textureHandler = [&]() {
            if (_textureData->forceUpdate) {
                glDeleteTextures(1, (GLuint *)&_textureData->handle);        
                _textureData->handle = 0;
                _textureData->forceUpdate = false;
            }

            if (!_textureData->handle) { // create a new texture
                glGenTextures(1, (GLuint *)&_textureData->handle);
                glBindTexture(GL_TEXTURE_2D, (GLuint) _textureData->handle);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

                glPixelStorei(GL_UNPACK_ALIGNMENT, _textureData->textureByteAlignment);

                for (int i = 0; i < _textureData->textureInfo.textureNumMipMaps; i++) {
                    glPixelStorei(GL_UNPACK_ROW_LENGTH, _textureData->textureInfo.textureBufferWidth[i]);

                    glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, _textureData->textureInfo.textureWidth[i],
                        _textureData->textureInfo.textureHeight[i], 0, GL_RGBA, _textureData->getTexturePixelType(), getCurrentDecodedTexture(i));
                    glGenerateMipmap(GL_TEXTURE_2D);
                }

                glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
                glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
            } else {
                glBindTexture(GL_TEXTURE_2D, (GLuint) _textureData->handle);
            }
        };

        auto streamingTextureHandler = [&]() {
            glBindTexture(GL_TEXTURE_2D, m_StreamingTextureHandle);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            glPixelStorei(GL_UNPACK_ALIGNMENT, _textureData->textureByteAlignment);
            for (int i = 0; i < _textureData->textureInfo.textureNumMipMaps; i++) {
                glPixelStorei(GL_UNPACK_ROW_LENGTH, _textureData->textureInfo.textureBufferWidth[i]);
                glTexImage2D(GL_TEXTURE_2D, i, GL_RGBA, _textureData->textureInfo.textureBufferWidth[i],
                    _textureData->textureInfo.textureHeight[i], 0, GL_RGBA, _textureData->getTexturePixelType(), getCurrentDecodedTexture(i));
                glGenerateMipmap(GL_TEXTURE_2D);
            }

            glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
            glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
        };

        if (streamingTexture) {
            streamingTextureHandler();
        } else {
            textureHandler();
        }

        applyTextureFilters();

        if (_textureData->key <= 0x10000000) {
            __prepareDraw = false;
        }

        glUniform1f(textureScaleUIndex, state->textureScaleU);
        glUniform1f(textureScaleVIndex, state->textureScaleV);
        glUniform1f(textureOffsetUIndex, state->textureOffsetX);
        glUniform1f(textureOffsetVIndex, state->textureOffsetY);
    } else {
        glBindTexture(GL_TEXTURE_2D, 0);
    }

    glUniform1i(glGetUniformLocation(m_Program, "hasVertexColor"), state->vertexInfo.ct != 0);

    bool hasTexture = _textureData != nullptr;
    glUniform1i(useTextureIndex, hasTexture);
    glUniform1i(materialAmbientEnableIndex, state->materialAmbientEnable);
    glUniform1i(materialDiffuseEnableIndex, state->materialDiffuseEnable);
    glUniform1i(materialSpecularEnableIndex, state->materialSpecularEnable);
    if (state->materialAmbientEnable) {
        glUniform4f(materialAmbientIndex, state->materialAmbient[0], state->materialAmbient[1], state->materialAmbient[2], state->materialAmbient[3]);
    }

    if (state->materialDiffuseEnable) {
        glUniform4f(materialDiffuseIndex, state->materialDiffuse[0], state->materialDiffuse[1], state->materialDiffuse[2], state->materialDiffuse[3]);
    }

    if (state->materialSpecularEnable) {
        glUniform4f(materialSpecularIndex, state->materialSpecular[0], state->materialSpecular[1], state->materialSpecular[2], state->materialSpecular[3]);
    }

    auto applyAlphaBlending = [&]() {
        bool alphaBlendingEnabled = state->clearModeEnable || state->alphaBlendingEnable;
        if (alphaBlendingEnabled) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }
    };

    auto applyDepthTest = [&]() {
        bool depthTestEnabled = state->clearModeEnable || state->depthTestEnable;
        if (depthTestEnabled) {
            glEnable(GL_DEPTH_TEST);
        } else {
            glDisable(GL_DEPTH_TEST);
        }
    };

    auto applyAlphaTest = [&]() {
        bool alphaTestEnabled = state->clearModeEnable || state->alphaTestEnable;

        if (alphaTestEnabled) {
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        } else {
            glDisable(GL_BLEND);
        }
    };

    auto applyClearMode = [&]() {
        glUniform1i(glGetUniformLocation(m_Program, "clearModeEnable"), state->clearModeEnable);

        if (state->clearModeEnable) {
            glEnable(GL_DEPTH_TEST);
            glClear(GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);
        }
    };

    auto applyFaceCulling = [&]() {
        if (state->clearModeEnable || type == GE_PRIM_RECTANGLES || !state->cullingEnable) {
            glDisable(GL_CULL_FACE);
        } else {
            glEnable(GL_CULL_FACE);
            glFrontFace(state->cullingFaceDirection != 0 ? GL_CW : GL_CCW);
        }
    };

    glDepthRange(state->minZ, state->maxZ);

    applyDepthTest();
    applyAlphaBlending();
    applyAlphaTest();
    applyClearMode();
    applyFaceCulling();
}

void RenderDeviceOpenGL::drawPrimitive(GPUState *state, int type, int count) {
    if (!validOpenGLState)
        return;

    if (!__prepareDraw || !_vertexData)
        return;

    switch (type) {
    case GE_PRIM_POINTS:
        glDrawArrays(GL_POINTS, 0, (GLsizei)_vertexData->size());
        break;
    case GE_PRIM_TRIANGLES:
        glDrawArrays(GL_TRIANGLES, 0, (GLsizei)_vertexData->size());
        break;
    case GE_PRIM_TRIANGLE_FAN:
        glDrawArrays(GL_TRIANGLE_FAN, 0, (GLsizei)_vertexData->size());
        break;
    case GE_PRIM_TRIANGLE_STRIP:
        glDrawArrays(GL_TRIANGLE_STRIP, 0, (GLsizei)_vertexData->size());
        break;
    case GE_PRIM_RECTANGLES:
        glDrawElements(GL_TRIANGLES, (GLsizei)indices.size(), GL_UNSIGNED_INT, 0);
        break;
    default:
        LOG_ERROR(logType, "Unimplemented draw primitive %d count %d", type, count);
    }
}

void RenderDeviceOpenGL::endDraw(GPUState *state) {
    if (!validOpenGLState)
        return;

    __prepareDraw = false;
}

void __RenderDeviceDisplayListBegin() {
    RenderDevice *dev = getRenderDevice();
    if (!dev)
        return;

    dev->displayListBegin();
}

void __RenderDeviceDisplayListEnd() {
    RenderDevice *dev = getRenderDevice();
    if (!dev)
        return;

    dev->displayListEnd();
}

void __DrawDebugPrimitive(GPUState *state, int type, int count) {

}

void __PrepareDraw(GPUState *state, int type, int count) {
    RenderDevice *dev = getRenderDevice();
    if (!dev)
        return;


    std::vector<VertexData> *vertexData = __getListFromVertexCache(state, type, count);
    TextureData *textureData, streamingTexture;
    
    switch (dev->getDeviceType()) {
    case RENDERER_TYPE_OPENGL:
    {
        auto oglDevice = reinterpret_cast<RenderDeviceOpenGL *>(dev);
    
        auto _handle = __updateTextureCache();
        if (_handle.size() != 0) {
            for (auto& i : _handle) {            
                if (i.handle)
                    glDeleteTextures(1, (GLuint *)&i.handle);
                // LOG_DEBUG(logType, "deleted key 0x%016llx.texcache (unused for awhile)", i.key);
            }
        }

        if (!oglDevice->streamingTexture) {
            textureData = __getTextureFromCache(state);
        } else {
            streamingTexture = __decodeTexture(state);
            textureData = streamingTexture.isDirty ? nullptr : &streamingTexture;
        }

        oglDevice->_textureData = textureData;
        oglDevice->_vertexData = vertexData;
        dev->prepareDraw(state, type, count);
        break;
    }
    }
}

void __DrawPrimitive(GPUState *state, int type, int count) {
    RenderDevice *dev = getRenderDevice();
    if (!dev)
        return;

    dev->drawPrimitive(state, type, count);
}

void __EndDraw(GPUState *state) {
    RenderDevice *dev = getRenderDevice();
    if (!dev)
        return;

    dev->endDraw(state);
}

void __openglUpdateFramebuffer(RenderDevice *device, int x, int y, int w, int h) {

}

void __openglSetStreamingTextureDevice(RenderDevice *device, bool state) {
    if (device && device->getDeviceType() == RENDERER_TYPE_OPENGL) {
        reinterpret_cast<RenderDeviceOpenGL *>(device)->streamingTexture = state;
    }
}

void __openglRenderScreen(RenderDevice *device) {
    RenderDeviceOpenGL *dev = (RenderDeviceOpenGL *) device;

    if (!dev || dev->getDeviceType() != RENDERER_TYPE_OPENGL)
        return;

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // glViewport(0, 0, 480*2, 272*2);

    glBindTexture(GL_TEXTURE_2D, dev->m_FramebufferTexture);
    glBindVertexArray(dev->m_FramebufferVAO);

    glDisable(GL_DEPTH_TEST);

    glUseProgram(dev->m_FramebufferProgram);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    device->displayListBegin();

    // glUseProgram(0);
    // glBindTexture(GL_TEXTURE_2D, 0);
    // glBindVertexArray(0);
}

void __openglDebugTexture(const char *texcache) {
    auto dev = (RenderDeviceOpenGL *) getRenderDevice();

    if (dev->getDeviceType() != RENDERER_TYPE_OPENGL) {
        LOG_ERROR(logType, "device is not OpenGL");
        return;
    }

    FILE *f = fopen(texcache, "rb");
    if (!f) {
        LOG_ERROR(logType, "can't find texture cache file path %s", texcache);
        return;
    }

    fseek(f, 0, SEEK_END);
    auto size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *buf = new uint8_t[size];
    fread(buf, size, 1, f);
    fclose(f);

    uint32_t fbo, rbo, tex;
    int w = 64, h = 64;

    glGenFramebuffers(1, &fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_SHORT_1_5_5_5_REV, buf);
    delete[] buf;

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex, 0);

    glGenRenderbuffers(1, &rbo);
    glBindRenderbuffer(GL_RENDERBUFFER, rbo);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, rbo);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glBindTexture(GL_TEXTURE_2D, 0);

    glDeleteFramebuffers(1, &dev->m_FramebufferObject);
    glDeleteRenderbuffers(1, &dev->m_RenderBufferObject);
    glDeleteTextures(1, &dev->m_FramebufferTexture);

    dev->m_FramebufferObject = fbo;
    dev->m_RenderBufferObject = rbo;
    dev->m_FramebufferTexture = tex;
}

RenderDevice *createRenderDevice(RendererType type) {
    RenderDevice *dev;
    switch (type) {
    case RENDERER_TYPE_NONE:
        dev = new RenderDeviceNone;
        break;
    case RENDERER_TYPE_OPENGL:
        dev = new RenderDeviceOpenGL;
        break;
    default:
        dev = nullptr;
    }
    return dev;
}

void destroyRenderDevice(RenderDevice *device) {
    if (device) {
        delete device;
    }
}

static RenderDevice *curDevice;

RenderDevice *getRenderDevice() {
    return curDevice;
}
void setRenderDevice(RenderDevice *device) {
    curDevice = device;
}
}