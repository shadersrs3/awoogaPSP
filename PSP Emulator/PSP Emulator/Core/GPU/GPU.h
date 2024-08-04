#pragma once

#include <cstdint>
#include <cstring>

#include <Core/GPU/DisplayList.h>

namespace Core::GPU {
class GE_Matrix4x3 {
public:
    GE_Matrix4x3(float m11 = 1.0f, float m12 = 0.0f, float m13 = 0.0f,
        float m21 = 0.0f, float m22 = 1.0f, float m23 = 0.0f,
        float m31 = 0.0f, float m32 = 0.0f, float m33 = 1.0f,
        float m41 = 0.0f, float m42 = 0.0f, float m43 = 0.0f)
    {
        mData[0] = m11;
        mData[1] = m12;
        mData[2] = m13;

        mData[3] = m21;
        mData[4] = m22;
        mData[5] = m23;

        mData[6] = m31;
        mData[7] = m32;
        mData[8] = m33;

        mData[9] = m41;
        mData[10] = m42;
        mData[11] = m43;
    }

    void DeepCopyFrom(GE_Matrix4x3 &inMatrix)
    {
        memcpy(mData, inMatrix.mData, sizeof(float) * 12);
    }

public:
    float mData[12];
};

class GE_Matrix4x4 {
public:
    GE_Matrix4x4(float m11 = 1.0f, float m12 = 0.0f, float m13 = 0.0f, float m14 = 0.0f,
        float m21 = 0.0f, float m22 = 1.0f, float m23 = 0.0f, float m24 = 0.0f,
        float m31 = 0.0f, float m32 = 0.0f, float m33 = 1.0f, float m34 = 0.0f,
        float m41 = 0.0f, float m42 = 0.0f, float m43 = 0.0f, float m44 = 1.0f)
    {
        mData[0] = m11;
        mData[1] = m12;
        mData[2] = m13;
        mData[3] = m14;

        mData[4] = m21;
        mData[5] = m22;
        mData[6] = m23;
        mData[7] = m24;

        mData[8] = m31;
        mData[9] = m32;
        mData[10] = m33;
        mData[11] = m34;

        mData[12] = m41;
        mData[13] = m42;
        mData[14] = m43;
        mData[15] = m44;
    }

    GE_Matrix4x4(const GE_Matrix4x3& inMatrix)
    {
        mData[0] = inMatrix.mData[0];
        mData[1] = inMatrix.mData[1];
        mData[2] = inMatrix.mData[2];
        mData[3] = 0.0f;

        mData[4] = inMatrix.mData[3];
        mData[5] = inMatrix.mData[4];
        mData[6] = inMatrix.mData[5];
        mData[7] = 0.0f;

        mData[8] = inMatrix.mData[6];
        mData[9] = inMatrix.mData[7];
        mData[10] = inMatrix.mData[8];
        mData[11] = 0.0f;

        mData[12] = inMatrix.mData[9];
        mData[13] = inMatrix.mData[10];
        mData[14] = inMatrix.mData[11];
        mData[15] = 1.0f;
    }

    void DeepCopyFrom(GE_Matrix4x4 &inMatrix)
    {
        memcpy(mData, inMatrix.mData, sizeof(float) * 16);
    }

    inline void SetScale(float inX, float inY, float inZ)
    {
        mData[0] = inX;
        mData[5] = inY;
        mData[10] = inZ;
    }

public:
    float mData[16];
};

struct GPUState {
public:
    uint32_t vertexListAddress;
    uint32_t indexListAddress;
    uint32_t baseAddress;
    uint32_t offsetAddress;

    struct VertexInfo {
        uint32_t param;
        uint8_t tt, ct, nt, vt, wt, it, wc, mc, tm;
        uint32_t texture_offset;
        uint32_t color_offset;
        uint32_t normal_offset;
        uint32_t position_offset;
        uint32_t one_vertex_size, vertex_size;
    } vertexInfo;

    struct TextureInfo {
        bool clutIsDirty;
        int textureNumMipMaps;
        uint32_t textureBasePointer[8];
        uint32_t textureBufferWidth[8];
        uint32_t textureWidth[8];
        uint32_t textureHeight[8];
        uint32_t textureStorage;
        bool textureSwizzle;
        bool clutShared;
        uint32_t clutAddress;
        uint32_t clutMode;
        uint8_t clutCsa;
        uint8_t clutSft;
        uint8_t clutMsk;
        uint8_t clutNp;
    } textureInfo;

    struct TextureFunction {
        uint8_t txf;
        bool tcc, cd;
        float r, g, b, a;
    } textureFunction;

    bool globalLightingEnable;
    bool lightingEnable[4];
    bool clippingEnable;
    bool cullingEnable;
    bool textureEnable;
    bool fogEnable;
    bool ditherEnable;
    bool alphaBlendingEnable;
    bool alphaTestEnable;
    bool depthTestEnable;
    bool stencilTestEnable;
    bool antialiasingEnable;
    bool patchCullingEnable;
    bool colorTestEnable;
    bool logicalOperationEnable;
    bool reverseNormals;
    float morphingWeights[8];

    GE_Matrix4x3 rawBoneMatrix[8];
    GE_Matrix4x4 boneMatrix[8];
    int boneMatrixNumber;
    GE_Matrix4x4 projectionMatrix;
    int projectionMatrixNumber;
    GE_Matrix4x3 rawViewMatrix;
    GE_Matrix4x4 viewMatrix;
    int viewMatrixNumber;
    GE_Matrix4x3 rawWorldMatrix;
    GE_Matrix4x4 worldMatrix;
    int worldMatrixNumber;
    GE_Matrix4x3 rawTexGenMatrix;
    GE_Matrix4x4 texGenMatrix;
    int texGenMatrixNumber;
    bool matrixUpdated;

    bool cullingFaceDirection;

    float viewportXScale, viewportYScale, viewportZScale, viewportXCenter, viewportYCenter, viewportZCenter;
    float textureScaleU, textureScaleV;
    float textureOffsetX, textureOffsetY;

    float screenOffsetX, screenOffsetY;

    bool materialAmbientEnable, materialDiffuseEnable, materialSpecularEnable;
    float materialAmbient[4], materialDiffuse[4], materialSpecular[4], materialEmission[4];

    uint8_t textureMappingMode;
    uint8_t textureProjectionMapping;
    uint8_t textureShadeU;
    uint8_t textureShadeV;
    uint8_t textureMagFilter;
    uint8_t textureMinFilter;
    uint8_t textureWrapModeS;
    uint8_t textureWrapModeT;
    uint8_t textureMipMapMode;
    float textureMipmapBias;
    float textureMipMapSlope;

    bool clearModeEnable;
    bool clearColorMasked;
    bool clearAlphaMasked;
    bool clearDepthMasked;

    struct Scissor {
        uint16_t scissorUpperLeftX, scissorUpperLeftY;
        uint16_t scissorLowerRightX, scissorLowerRightY;
    } scissor;

    float minZ, maxZ;
    uint8_t alphaTestFunction;
    uint8_t alphaTestColorReference;
    uint8_t alphaTestColorMask;
    uint8_t stencilTestFunction;
    uint8_t stencilTestReference;
    uint8_t stencilTestMask;
    uint8_t stencilSFail;
    uint8_t stencilZFail;
    uint8_t stencilZPass;
    uint8_t depthTestFunction;
    uint8_t alphaBlendingSFactor;
    uint8_t alphaBlendingDFactor;
    uint8_t alphaBlendingEquation;
    float alphaBlendingFixA[3];
    float alphaBlendingFixB[3];
    bool debugModeEnable;
public:
    enum DrawType { PRIMITIVE, SPLINE, BEZIER };

    // operations
    void setIndexListAddress(uint32_t param);
    void setVertexListAddress(uint32_t param);
    void draw(const DrawType& type, uint32_t param);
    void setBBOX(uint32_t bboxParam);
    void jump(DisplayList& dl, uint32_t param);
    void jumpBBOX(DisplayList& dl, uint32_t param);
    bool call(DisplayList& dl, uint32_t param);
    bool ret(DisplayList& dl, uint32_t param);
    void end(uint32_t param);
    void signal(uint32_t signalParam);
    void finish(uint32_t param);
    void setBaseAddress(uint32_t addressBase);
    void setVertexType(uint32_t vertexType);
    void setOffsetAddress(uint32_t param);
    void setOriginAddress(DisplayList& dl, uint32_t param);
    void setDrawingRegion1(uint32_t param);
    void setDrawingRegion2(uint32_t param);
    void setGlobalLightingEnable(uint32_t param);
    void setLightingEnable(int lightNr, uint32_t param);
    void setClippingEnable(uint32_t param);
    void setCullingEnable(uint32_t param);
    void setTextureEnable(uint32_t param);
    void setFogEnable(uint32_t param);
    void setDitherEnable(uint32_t param);
    void setAlphaBlendingEnable(uint32_t param);
    void setAlphaTestEnable(uint32_t param);
    void setDepthTestEnable(uint32_t param);
    void setStencilTestEnable(uint32_t param);
    void setAntialiasingEnable(uint32_t param);
    void setPatchCullingEnable(uint32_t param);
    void setColorTestEnable(uint32_t param);
    void setLogicalOperationEnable(uint32_t param);
    void setBoneMatrixNumber(uint32_t param);
    void setBoneMatrixData(uint32_t param);
    void setVertexWeight(int weightNr, uint32_t param);
    void setPatchDivisionCount(uint32_t count);
    void setPatchPrimitive(uint32_t param);
    void setPatchFace(uint32_t param);
    void setWorldMatrixNumber(uint32_t param);
    void setWorldMatrixData(uint32_t param);
    void setViewMatrixNumber(uint32_t param);
    void setViewMatrixData(uint32_t param);
    void setProjectionMatrixNumber(uint32_t param);
    void setProjectionMatrixData(uint32_t param);
    void setTexGenMatrixNumber(uint32_t param);
    void setTexGenMatrixData(uint32_t param);
    void setSX(uint32_t param);
    void setSY(uint32_t param);
    void setSZ(uint32_t param);
    void setTX(uint32_t param);
    void setTY(uint32_t param);
    void setTZ(uint32_t param);
    void setSU(uint32_t param);
    void setSV(uint32_t param);
    void setTU(uint32_t param);
    void setTV(uint32_t param);
    void setScreenOffsetX(uint32_t param);
    void setScreenOffsetY(uint32_t param);
    void setShadingMode(uint32_t param);
    void setNormalReverse(uint32_t param);
    void setMaterial(uint32_t param);
    void setMEC(uint32_t param);
    void setMAC(uint32_t param);
    void setMDC(uint32_t param);
    void setMSC(uint32_t param);
    void setModelColorAlpha(uint32_t param);
    void setMK(uint32_t param);
    void setAmbientLightColor(uint32_t param);
    void setAmbientLightColorAlpha(uint32_t param);
    void setLightMode(uint32_t param);
    void setLightType(int lightNr, uint32_t param);
    void setLightVectorPosition(int lightNr, int xyz, uint32_t param);
    void setLightVectorDirection(int lightNr, int xyz, uint32_t param);
    void setLightDistanceAttenuation(int lightNr, int abc, uint32_t param);
    void setLightConvergenceFactor(int lightNr, uint32_t param);
    void setLightCutoffDotProductCoefficient(int lightNr, uint32_t param);
    void setLightColorAmbient(int lightNr, uint32_t param);
    void setLightColorDiffuse(int lightNr, uint32_t param);
    void setLightColorSpecular(int lightNr, uint32_t param);
    void setCullingSurface(uint32_t param);
    void setFramebufferBasePointer(uint32_t param);
    void setFramebuuferBaseWidth(uint32_t param);
    void setDepthbufferBasePointer(uint32_t param);
    void setDepthbufferBaseWidth(uint32_t param);
    void setTextureBufferBasePointer(int level, uint32_t param);
    void setTextureBufferWidth(int level, uint32_t param);
    void setCLUTBasePointer(uint32_t param);
    void setUpperCLUTBasePointer(uint32_t param);
    void setTextureSize(int level, uint32_t param);
    void setTextureMappingMode(uint32_t param);
    void setTextureShadeMapping(uint32_t param);
    void setTextureMode(uint32_t param);
    void setTexturePixelFormat(uint32_t param);
    void setCLUTLoad(uint32_t load);
    void setCLUT(uint32_t param);
    void setTextureFilter(uint32_t param);
    void setTextureWrapMode(uint32_t wrap);
    void setTextureLevelMode(uint32_t param);
    void setTextureFunction(uint32_t param);
    void setTextureEnvironmentColor(uint32_t param);
    void textureFlush(uint32_t param);
    void textureSync(uint32_t param);
    void setFogParameter(int nr, uint32_t param);
    void setFogColor(uint32_t param);
    void setTextureSlope(uint32_t param);
    void setFramePixelFormat(uint32_t param);
    void setClearMode(uint32_t clearMode);
    void setScissoringAreaUpperLeft(uint32_t param);
    void setScissoringAreaLowerRight(uint32_t param);
    void setMinDepthRange(uint32_t param);
    void setMaxDepthRange(uint32_t param);
    void setColorTestFunction(uint32_t param);
    void setColorReference(uint32_t param);
    void setColorMask(uint32_t param);
    void setAlphaTest(uint32_t param);
    void setStencilTest(uint32_t param);
    void setStencilOperation(uint32_t param);
    void setDepthTestFunction(uint32_t param);
    void setBlend(uint32_t param);
    void setFixA(uint32_t param);
    void setFixB(uint32_t param);
    // TODO
};

GPUState *getGPUState();

void initialize();
void reset();
void destroy();
}