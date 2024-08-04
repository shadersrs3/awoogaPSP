#include <Core/GPU/GPU.h>

#include <Core/Logger.h>

namespace Core::GPU {
static const char *logType = "gpu";

inline float getFloat24(uint32_t data){ data<<=8; return *(float*)(&data);}

void GPUState::setIndexListAddress(uint32_t param) {
    indexListAddress = (baseAddress | param) + offsetAddress;
}

void GPUState::setVertexListAddress(uint32_t param) {
    vertexListAddress = (baseAddress | param) + offsetAddress;
}

void GPUState::draw(const DrawType& type, uint32_t param) {

}

void GPUState::setBBOX(uint32_t bboxParam) {

}

void GPUState::jump(DisplayList& dl, uint32_t param) {
    uint32_t jumpAddress = (baseAddress | param) & 0x0FFFFFFC;
    dl.currentAddress = jumpAddress + offsetAddress;
}

void GPUState::jumpBBOX(DisplayList& dl, uint32_t param) {

}

bool GPUState::call(DisplayList& dl, uint32_t param) {
    uint32_t callAddress = ((baseAddress | param) & 0x0FFFFFFF) + offsetAddress;

    int index = dl.callStackCurrentIndex;
    if (index >= 8) {
        LOG_ERROR(logType, "call stack to 0x%08x exceeded limit", callAddress);
        return false;
    }

    dl.callStack[dl.callStackCurrentIndex++] = dl.currentAddress + 4;
    dl.currentAddress = callAddress;
    return true;
}

bool GPUState::ret(DisplayList& dl, uint32_t param) {
    int index = dl.callStackCurrentIndex;
    if (index <= 0) {
        LOG_ERROR(logType, "can't return display list if index is zero!");
        return false;
    }

    dl.currentAddress = dl.callStack[--dl.callStackCurrentIndex];
    return true;
}

void GPUState::end(uint32_t param) {

}

void GPUState::signal(uint32_t signalParam) {

}

void GPUState::finish(uint32_t param) {

}

void GPUState::setBaseAddress(uint32_t addressBase) {
    baseAddress = (addressBase << 8) & 0xFF000000;
}

__forceinline void PaddingMax(int &x, int y) { if (x < y) x = y; }

void GPUState::setVertexType(uint32_t param) {
    static const uint32_t size_mapping[4] = { 0, 1, 2, 4 };
    static const uint32_t size_padding[4] = { 0, 0, 1, 3 };
    static const uint32_t color_size_mapping[8] = { 0, 1, 1, 1, 2, 2, 2, 4 };
    static const uint32_t color_size_padding[8] = { 0, 0, 0, 0, 1, 1, 1, 3 };

    // param to parse
    vertexInfo.param = param;

    // vertex elements
    vertexInfo.tt = (vertexInfo.param >>   0) & 0x3;
    vertexInfo.ct = (vertexInfo.param >>   2) & 0x7;
    vertexInfo.nt = (vertexInfo.param >>   5) & 0x3;
    vertexInfo.vt = (vertexInfo.param >>   7) & 0x3;
    vertexInfo.wt = (vertexInfo.param >>   9) & 0x3;
    vertexInfo.it = (vertexInfo.param >>  11) & 0x3;

    vertexInfo.wc = ((vertexInfo.param >> 14) & 0x7) + 1;
    vertexInfo.mc = ((vertexInfo.param >> 18) & 0x7) + 1;
    vertexInfo.tm = ((vertexInfo.param >> 23) & 0x1) != 0;

    // offsets and alignment size
    uint32_t offset  = 0;
    int padding = 0;
    uint32_t type;

    if (!vertexInfo.tm && vertexInfo.wt) {
        type = vertexInfo.wt;
        offset += size_mapping[type] * vertexInfo.wc;
        PaddingMax(padding, size_padding[type]);
    }

    if (vertexInfo.tt) {
        type = vertexInfo.tt; 
        offset = (offset + size_padding[type]) & ~size_padding[type];
        vertexInfo.texture_offset = offset;
        offset += size_mapping[type] * 2;
        PaddingMax(padding, size_padding[type]);
    }

    if (vertexInfo.ct) {
        type = vertexInfo.ct;
        offset = (offset + color_size_padding[type]) & ~color_size_padding[type];
        vertexInfo.color_offset = offset;
        offset += color_size_mapping[type];
        PaddingMax(padding, color_size_padding[type]); 
    }

    if (!vertexInfo.tm && vertexInfo.nt) {
        type = vertexInfo.nt;
        offset = (offset + size_padding[type]) & ~size_padding[type];
        vertexInfo.normal_offset = offset;
        offset += size_mapping[type] * 3;
        PaddingMax(padding, size_padding[type]); 
    }

    type = vertexInfo.vt; 
    offset = (offset + size_padding[type]) & ~size_padding[type];
    vertexInfo.position_offset = offset;    
    offset += size_mapping[type] * 3;
    PaddingMax(padding, size_padding[type]);

    vertexInfo.one_vertex_size = (offset + padding) & ~padding;
    vertexInfo.vertex_size     = vertexInfo.one_vertex_size*vertexInfo.mc;
}

void GPUState::setOffsetAddress(uint32_t param) {
    offsetAddress = param << 8;
}

void GPUState::setOriginAddress(DisplayList& dl, uint32_t param) {
    offsetAddress = dl.currentAddress;
    // LOG_WARN(logType, "called origin address!");
}

void GPUState::setDrawingRegion1(uint32_t param) {

}

void GPUState::setDrawingRegion2(uint32_t param) {

}

void GPUState::setGlobalLightingEnable(uint32_t param) {
    globalLightingEnable = bool(param & 1);
}

void GPUState::setLightingEnable(int lightNr, uint32_t param) {
    lightingEnable[lightNr] = bool(param & 1);
}

void GPUState::setClippingEnable(uint32_t param) {
    clippingEnable = bool(param & 1);
}

void GPUState::setCullingEnable(uint32_t param) {
    cullingEnable = bool(param & 1);
}

void GPUState::setTextureEnable(uint32_t param) {
    textureEnable = bool(param & 1);
}

void GPUState::setFogEnable(uint32_t param) {
    fogEnable = bool(param & 1);
}

void GPUState::setDitherEnable(uint32_t param) {
    ditherEnable = bool(param & 1);
}

void GPUState::setAlphaBlendingEnable(uint32_t param) {
    alphaBlendingEnable = bool(param & 1);
}

void GPUState::setAlphaTestEnable(uint32_t param) {
    alphaTestEnable = bool(param & 1);
}

void GPUState::setDepthTestEnable(uint32_t param) {
    depthTestEnable = bool(param & 1);
}

void GPUState::setStencilTestEnable(uint32_t param) {
    stencilTestEnable = bool(param & 1);
}

void GPUState::setAntialiasingEnable(uint32_t param) {
    antialiasingEnable = bool(param & 1);
}

void GPUState::setPatchCullingEnable(uint32_t param) {
    patchCullingEnable = bool(param & 1);
}

void GPUState::setColorTestEnable(uint32_t param) {
    colorTestEnable = bool(param & 1);
}

void GPUState::setLogicalOperationEnable(uint32_t param) {
    logicalOperationEnable = bool(param & 1);
}

void GPUState::setBoneMatrixNumber(uint32_t param) {
    boneMatrixNumber = param & 0x7F;
}

void GPUState::setBoneMatrixData(uint32_t param) {
    int _boneMatrixNumber = boneMatrixNumber / 12;
    auto& _boneMatrix = rawBoneMatrix[_boneMatrixNumber];
    int currentBone = (boneMatrixNumber++) % 12;
    _boneMatrix.mData[currentBone] = getFloat24(param);

    if (currentBone == 11) {
        boneMatrix[_boneMatrixNumber] = GE_Matrix4x4(_boneMatrix);
    }
}

void GPUState::setVertexWeight(int weightNr, uint32_t param) {
    morphingWeights[weightNr] = getFloat24(param);
}

void GPUState::setPatchDivisionCount(uint32_t count) {

}

void GPUState::setPatchPrimitive(uint32_t param) {

}

void GPUState::setPatchFace(uint32_t param) {

}

void GPUState::setWorldMatrixNumber(uint32_t param) {
    worldMatrixNumber = param & 0xF;
}

void GPUState::setWorldMatrixData(uint32_t param) {
    float data = getFloat24(param);
    int number = worldMatrixNumber % 12;

    rawWorldMatrix.mData[number] = data;
    if (++worldMatrixNumber == 12)
        worldMatrix = GE_Matrix4x4(rawWorldMatrix);

    matrixUpdated = true;
}

void GPUState::setViewMatrixNumber(uint32_t param) {
    viewMatrixNumber = param & 0xF;
}

void GPUState::setViewMatrixData(uint32_t param) {
    float data = getFloat24(param);
    int number = viewMatrixNumber % 12;

    rawViewMatrix.mData[number] = data;
    if (++viewMatrixNumber == 12)
        viewMatrix = GE_Matrix4x4(rawViewMatrix);

    matrixUpdated = true;
}

void GPUState::setProjectionMatrixNumber(uint32_t param) {
    projectionMatrixNumber = param & 0xF;
}

void GPUState::setProjectionMatrixData(uint32_t param) {
    float data = getFloat24(param);
    int number = projectionMatrixNumber++ % 16;
    projectionMatrix.mData[number] = data;
    matrixUpdated = true;
}

void GPUState::setTexGenMatrixNumber(uint32_t param) {
    texGenMatrixNumber = param & 0xF;
}

void GPUState::setTexGenMatrixData(uint32_t param) {
    float data = getFloat24(param);
    int number = texGenMatrixNumber % 12;

    rawTexGenMatrix.mData[number] = data;
    if (++texGenMatrixNumber == 12)
        texGenMatrix = GE_Matrix4x4(rawTexGenMatrix);
    matrixUpdated = true;
}

void GPUState::setSX(uint32_t param) {
    viewportXScale = getFloat24(param);
}

void GPUState::setSY(uint32_t param) {
    viewportYScale = getFloat24(param);
}

void GPUState::setSZ(uint32_t param) {
    viewportZScale = getFloat24(param);
}

void GPUState::setTX(uint32_t param) {
    viewportXCenter = getFloat24(param);
}

void GPUState::setTY(uint32_t param) {
    viewportYCenter = getFloat24(param);
}

void GPUState::setTZ(uint32_t param) {
    viewportZCenter = getFloat24(param);
}

void GPUState::setSU(uint32_t param) {
    textureScaleU = getFloat24(param);
}

void GPUState::setSV(uint32_t param) {
    textureScaleV = getFloat24(param);
}

void GPUState::setTU(uint32_t param) {
    textureOffsetX = getFloat24(param);
}

void GPUState::setTV(uint32_t param) {
    textureOffsetY = getFloat24(param);
}

void GPUState::setScreenOffsetX(uint32_t param) {
    screenOffsetX = float(param & 0xFFFF) / 16.f;
}

void GPUState::setScreenOffsetY(uint32_t param) {
    screenOffsetY = float(param & 0xFFFF) / 16.f;
}

void GPUState::setShadingMode(uint32_t param) {

}

void GPUState::setNormalReverse(uint32_t param) {
    reverseNormals = bool(param & 1);
}

void GPUState::setMaterial(uint32_t param) {
    materialAmbientEnable = param & 1;
    materialDiffuseEnable = (param >> 1) & 1;
    materialSpecularEnable = (param >> 2) & 1;
}

void GPUState::setMEC(uint32_t param) {
    materialEmission[0] = (param & 255) / 255.f;
    materialEmission[1] = ((param >> 8) & 255) / 255.f;
    materialEmission[2] = ((param >> 16) & 255) / 255.f;
    materialEmission[3] = 1.f;
}

void GPUState::setMAC(uint32_t param) {
    materialAmbient[0] = (param & 255) / 255.f;
    materialAmbient[1] = ((param >> 8) & 255) / 255.f;
    materialAmbient[2] = ((param >> 16) & 255) / 255.f;
}

void GPUState::setMDC(uint32_t param) {
    materialDiffuse[0] = (param & 255) / 255.f;
    materialDiffuse[1] = ((param >> 8) & 255) / 255.f;
    materialDiffuse[2] = ((param >> 16) & 255) / 255.f;
    materialDiffuse[3] = 1.f;
}

void GPUState::setMSC(uint32_t param) {
    materialSpecular[0] = (param & 255) / 255.f;
    materialSpecular[1] = ((param >> 8) & 255) / 255.f;
    materialSpecular[2] = ((param >> 16) & 255) / 255.f;
    materialSpecular[3] = 1.f;
}

void GPUState::setModelColorAlpha(uint32_t param) {
    materialAmbient[3] = (param & 255) / 255.f;
}

void GPUState::setMK(uint32_t param) {

}

void GPUState::setAmbientLightColor(uint32_t param) {

}

void GPUState::setAmbientLightColorAlpha(uint32_t param) {

}

void GPUState::setLightMode(uint32_t param) {

}

void GPUState::setLightType(int lightNr, uint32_t param) {

}

void GPUState::setLightVectorPosition(int lightNr, int xyz, uint32_t param) {

}

void GPUState::setLightVectorDirection(int lightNr, int xyz, uint32_t param) {

}

void GPUState::setLightDistanceAttenuation(int lightNr, int abc, uint32_t param) {

}

void GPUState::setLightConvergenceFactor(int lightNr, uint32_t param) {

}

void GPUState::setLightCutoffDotProductCoefficient(int lightNr, uint32_t param) {

}

void GPUState::setLightColorAmbient(int lightNr, uint32_t param) {

}

void GPUState::setLightColorDiffuse(int lightNr, uint32_t param) {

}

void GPUState::setLightColorSpecular(int lightNr, uint32_t param) {

}

void GPUState::setCullingSurface(uint32_t param) {
    cullingFaceDirection = param != 0;
}

void GPUState::setFramebufferBasePointer(uint32_t param) {

}

void GPUState::setFramebuuferBaseWidth(uint32_t param) {

}

void GPUState::setDepthbufferBasePointer(uint32_t param) {

}

void GPUState::setDepthbufferBaseWidth(uint32_t param) {

}

void GPUState::setTextureBufferBasePointer(int level, uint32_t param) {
    textureInfo.textureBasePointer[level] = (textureInfo.textureBasePointer[level] & 0xFF000000) | param & 0xFFFFFF;
}

void GPUState::setTextureBufferWidth(int level, uint32_t param) {
    textureInfo.textureBufferWidth[level] = param & 0x7FF;
    textureInfo.textureBasePointer[level] = (textureInfo.textureBasePointer[level] & 0xFFFFFF) | ((param << 8) & 0xFF000000);
}

void GPUState::setCLUTBasePointer(uint32_t param) {
    uint32_t& clutAddress = textureInfo.clutAddress;
    clutAddress = (clutAddress & 0xFF000000) | param;
    textureInfo.clutIsDirty = true;
}

void GPUState::setUpperCLUTBasePointer(uint32_t param) {
    uint32_t& clutAddress = textureInfo.clutAddress;
    clutAddress = (clutAddress & 0x00FFFFFF) | ((param << 8) & 0x0F000000);
}

void GPUState::setTextureSize(int level, uint32_t param) {
    int textureWidth = 1 << (param & 0xF);
    int textureHeight = 1 << ((param >> 8) & 0xF);
    textureInfo.textureWidth[level] = textureWidth;
    textureInfo.textureHeight[level] = textureHeight;
}

void GPUState::setTextureMappingMode(uint32_t param) {
    textureMappingMode = param & 3;
    textureProjectionMapping = (param >> 8) & 3;
}

void GPUState::setTextureShadeMapping(uint32_t param) {
    textureShadeU = param & 3;
    textureShadeV = param & 3;
}

void GPUState::setTextureMode(uint32_t param) {
    textureInfo.textureNumMipMaps = ((param >> 16) & 7) + 1;
    textureInfo.clutShared = ((param >> 8) & 0xFF) != 0;
    textureInfo.textureSwizzle = (param & 0xFF) != 0;
}

void GPUState::setTexturePixelFormat(uint32_t param) {
    textureInfo.textureStorage = param & 0xF;
}

void GPUState::setCLUTLoad(uint32_t load) {
    textureInfo.clutNp = load & 0x3F;
    textureInfo.clutIsDirty = true;
}

void GPUState::setCLUT(uint32_t param) {
    textureInfo.clutMode = param & 3;
    textureInfo.clutSft = (param >> 2) & 0x1F;
    textureInfo.clutMsk = (param >> 8) & 0xFF;
    textureInfo.clutCsa = (param >> 16) & 0x1F;
}

void GPUState::setTextureFilter(uint32_t param) {
    textureMagFilter = (param >> 8) & 0xFF;
    textureMinFilter = param & 0x7;
}

void GPUState::setTextureWrapMode(uint32_t wrap) {
    textureWrapModeS = wrap & 0xFF;
    textureWrapModeT = (wrap >> 8) & 0xFF;
}

void GPUState::setTextureLevelMode(uint32_t param) {
    textureMipMapMode = param & 3;
    textureMipmapBias = float(int32_t(int8_t(param >> 16))) / 16.f;
}

void GPUState::setTextureFunction(uint32_t param) {
    textureFunction.txf = param & 7;
    textureFunction.tcc = (param >> 8) & 1;
    textureFunction.cd = (param >> 16) & 1;
}

void GPUState::setTextureEnvironmentColor(uint32_t param) {

}

void GPUState::textureFlush(uint32_t param) {

}

void GPUState::textureSync(uint32_t param) {

}

void GPUState::setFogParameter(int nr, uint32_t param) {

}

void GPUState::setFogColor(uint32_t param) {

}

void GPUState::setTextureSlope(uint32_t param) {
    textureMipMapSlope = getFloat24(param);
}

void GPUState::setFramePixelFormat(uint32_t param) {

}

void GPUState::setClearMode(uint32_t param) {
    clearModeEnable = (param & 0x1) != 0;
    clearColorMasked = (param & 0x100) != 0;
    clearAlphaMasked = (param & 0x200) != 0;
    clearDepthMasked = (param & 0x400) != 0;
}

void GPUState::setScissoringAreaUpperLeft(uint32_t param) {
    scissor.scissorUpperLeftX = (uint16_t)(param & 0x3FF);
    scissor.scissorUpperLeftY = (uint16_t)((param >> 10) & 0x3FF);
}

void GPUState::setScissoringAreaLowerRight(uint32_t param) {
    scissor.scissorLowerRightX = (uint16_t)(param & 0x3FF);
    scissor.scissorLowerRightY = (uint16_t)((param >> 10) & 0x3FF);
}

void GPUState::setMinDepthRange(uint32_t param) {
    minZ = (param & 0xFFFF) / 65535.f;
}

void GPUState::setMaxDepthRange(uint32_t param) {
    maxZ = (param & 0xFFFF) / 65535.f;
}

void GPUState::setColorTestFunction(uint32_t param) {

}

void GPUState::setColorReference(uint32_t param) {

}

void GPUState::setColorMask(uint32_t param) {

}

void GPUState::setAlphaTest(uint32_t param) {
    alphaTestFunction = (param & 7);
    alphaTestColorReference = param >> 8;
    alphaTestColorMask = param >> 16;
}

void GPUState::setStencilTest(uint32_t param) {

}

void GPUState::setStencilOperation(uint32_t param) {

}

void GPUState::setDepthTestFunction(uint32_t param) {
}

void GPUState::setBlend(uint32_t param) {

}

void GPUState::setFixA(uint32_t param) {

}

void GPUState::setFixB(uint32_t param) {
}

GPUState *gpu;

GPUState *getGPUState() {
    return gpu;
}

void initialize() {
    gpu = new GPUState;
    if (!gpu)
        return;

    std::memset(gpu, 0, sizeof *gpu);
    LOG_SUCCESS(logType, "initialized gpu");
}

void reset() {
    if (gpu) {
        delete gpu;
        gpu = nullptr;
    }

    gpu = new GPUState;
    std::memset(gpu, 0, sizeof *gpu);
    LOG_SUCCESS(logType, "restarted gpu");
}

void destroy() {
    if (gpu) {
        delete gpu;
        gpu = nullptr;
    }

    LOG_SUCCESS(logType, "destroyed gpu");
}
}