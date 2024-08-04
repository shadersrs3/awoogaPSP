#include <deque>

#include <Core/GPU/GPU.h>
#include <Core/GPU/DisplayList.h>
#include <Core/GPU/Renderer.h>

#include <Core/GPU/GEConstants.h>

#include <Core/Memory/MemoryAccess.h>
#include <Core/Kernel/sceKernelInterrupt.h>

#include <Core/Logger.h>

#include <Core/Allegrex/Allegrex.h>

namespace Core::GPU {
static const char *logType = "displayList";
static std::deque<DisplayList> dlQueue;

union GPUOpcode {
    struct {
        uint32_t parameter : 24;
        uint32_t opcode : 8;
    };

    uint32_t instruction;
};

static constexpr const int DL_QUEUE_MAX_SIZE = 64;
static constexpr const int DL_MAX_INSTRUCTIONS = 50000000;

#define X \
    DO(CMD_NOP, 0x00) \
    DO(CMD_VADDR, 0x01) \
    DO(CMD_IADDR, 0x02) \
    DO(CMD_PRIM, 0x04) \
    DO(CMD_BEZIER, 0x05) \
    DO(CMD_SPLINE, 0x06) \
    DO(CMD_BBOX, 0x07) \
    DO(CMD_JUMP, 0x08) \
    DO(CMD_BJUMP, 0x09) \
    DO(CMD_CALL, 0x0A) \
    DO(CMD_RET, 0x0B) \
    DO(CMD_END, 0x0C) \
    DO(CMD_SIGNAL, 0x0E) \
    DO(CMD_FINISH, 0x0F) \
    DO(CMD_BASE, 0x10) \
    DO(CMD_VTYPE, 0x12) \
    DO(CMD_OFFSET, 0x13) \
    DO(CMD_ORIGIN, 0x14) \
    DO(CMD_REGION1, 0x15) \
    DO(CMD_REGION2, 0x16) \
    DO(CMD_LTE, 0x17) \
    DO(CMD_LE0, 0x18) \
    DO(CMD_LE1, 0x19) \
    DO(CMD_LE2, 0x1A) \
    DO(CMD_LE3, 0x1B) \
    DO(CMD_CLE, 0x1C) \
    DO(CMD_BCE, 0x1D) \
    DO(CMD_TME, 0x1E) \
    DO(CMD_FGE, 0x1F) \
    DO(CMD_DTE, 0x20) \
    DO(CMD_ABE, 0x21) \
    DO(CMD_ATE, 0x22) \
    DO(CMD_ZTE, 0x23) \
    DO(CMD_STE, 0x24) \
    DO(CMD_AAE, 0x25) \
    DO(CMD_PCE, 0x26) \
    DO(CMD_CTE, 0x27) \
    DO(CMD_LOE, 0x28) \
    DO(CMD_BONEN, 0x2A) \
    DO(CMD_BONED, 0x2B) \
    DO(CMD_WEIGHT0, 0x2C) \
    DO(CMD_WEIGHT1, 0x2D) \
    DO(CMD_WEIGHT2, 0x2E) \
    DO(CMD_WEIGHT3, 0x2F) \
    DO(CMD_WEIGHT4, 0x30) \
    DO(CMD_WEIGHT5, 0x31) \
    DO(CMD_WEIGHT6, 0x32) \
    DO(CMD_WEIGHT7, 0x33) \
    DO(CMD_DIVIDE, 0x36) \
    DO(CMD_PPM, 0x37) \
    DO(CMD_PFACE, 0x38) \
    DO(CMD_WORLDN, 0x3A) \
    DO(CMD_WORLDD, 0x3B) \
    DO(CMD_VIEWN, 0x3C) \
    DO(CMD_VIEWD, 0x3D) \
    DO(CMD_PROJN, 0x3E) \
    DO(CMD_PROJD, 0x3F) \
    DO(CMD_TGENN, 0x40) \
    DO(CMD_TGEND, 0x41) \
    DO(CMD_SX, 0x42) \
    DO(CMD_SY, 0x43) \
    DO(CMD_SZ, 0x44) \
    DO(CMD_TX, 0x45) \
    DO(CMD_TY, 0x46) \
    DO(CMD_TZ, 0x47) \
    DO(CMD_SU, 0x48) \
    DO(CMD_SV, 0x49) \
    DO(CMD_TU, 0x4A) \
    DO(CMD_TV, 0x4B) \
    DO(CMD_OFFSETX, 0x4C) \
    DO(CMD_OFFSETY, 0x4D) \
    DO(CMD_SHADE, 0x50) \
    DO(CMD_NREV, 0x51) \
    DO(CMD_MATERIAL, 0x53) \
    DO(CMD_MEC, 0x54) \
    DO(CMD_MAC, 0x55) \
    DO(CMD_MDC, 0x56) \
    DO(CMD_MSC, 0x57) \
    DO(CMD_MAA, 0x58) \
    DO(CMD_MK, 0x5B) \
    DO(CMD_AC, 0x5C) \
    DO(CMD_AA, 0x5D) \
    DO(CMD_LMODE, 0x5E) \
    DO(CMD_LTYPE0, 0x5F) \
    DO(CMD_LTYPE1, 0x60) \
    DO(CMD_LTYPE2, 0x61) \
    DO(CMD_LTYPE3, 0x62) \
    DO(CMD_LX0, 0x63) \
    DO(CMD_LY0, 0x64) \
    DO(CMD_LZ0, 0x65) \
    DO(CMD_LX1, 0x66) \
    DO(CMD_LY1, 0x67) \
    DO(CMD_LZ1, 0x68) \
    DO(CMD_LX2, 0x69) \
    DO(CMD_LY2, 0x6A) \
    DO(CMD_LZ2, 0x6B) \
    DO(CMD_LX3, 0x6C) \
    DO(CMD_LY3, 0x6D) \
    DO(CMD_LZ3, 0x6E) \
    DO(CMD_LDX0, 0x6F) \
    DO(CMD_LDY0, 0x70) \
    DO(CMD_LDZ0, 0x71) \
    DO(CMD_LDX1, 0x72) \
    DO(CMD_LDY1, 0x73) \
    DO(CMD_LDZ1, 0x74) \
    DO(CMD_LDX2, 0x75) \
    DO(CMD_LDY2, 0x76) \
    DO(CMD_LDZ2, 0x77) \
    DO(CMD_LDX3, 0x78) \
    DO(CMD_LDY3, 0x79) \
    DO(CMD_LDZ3, 0x7A) \
    DO(CMD_LKA0, 0x7B) \
    DO(CMD_LKB0, 0x7C) \
    DO(CMD_LKC0, 0x7D) \
    DO(CMD_LKA1, 0x7E) \
    DO(CMD_LKB1, 0x7F) \
    DO(CMD_LKC1, 0x80) \
    DO(CMD_LKA2, 0x81) \
    DO(CMD_LKB2, 0x82) \
    DO(CMD_LKC2, 0x83) \
    DO(CMD_LKA3, 0x84) \
    DO(CMD_LKB3, 0x85) \
    DO(CMD_LKC3, 0x86) \
    DO(CMD_LKS0, 0x87) \
    DO(CMD_LKS1, 0x88) \
    DO(CMD_LKS2, 0x89) \
    DO(CMD_LKS3, 0x8A) \
    DO(CMD_LKO0, 0x8B) \
    DO(CMD_LKO1, 0x8C) \
    DO(CMD_LKO2, 0x8D) \
    DO(CMD_LKO3, 0x8E) \
    DO(CMD_LAC0, 0x8F) \
    DO(CMD_LDC0, 0x90) \
    DO(CMD_LSC0, 0x91) \
    DO(CMD_LAC1, 0x92) \
    DO(CMD_LDC1, 0x93) \
    DO(CMD_LSC1, 0x94) \
    DO(CMD_LAC2, 0x95) \
    DO(CMD_LDC2, 0x96) \
    DO(CMD_LSC2, 0x97) \
    DO(CMD_LAC3, 0x98) \
    DO(CMD_LDC3, 0x99) \
    DO(CMD_LSC3, 0x9A) \
    DO(CMD_CULL, 0x9B) \
    DO(CMD_FBP, 0x9C) \
    DO(CMD_FBW, 0x9D) \
    DO(CMD_ZBP, 0x9E) \
    DO(CMD_ZBW, 0x9F) \
    DO(CMD_TBP0, 0xA0) \
    DO(CMD_TBP1, 0xA1) \
    DO(CMD_TBP2, 0xA2) \
    DO(CMD_TBP3, 0xA3) \
    DO(CMD_TBP4, 0xA4) \
    DO(CMD_TBP5, 0xA5) \
    DO(CMD_TBP6, 0xA6) \
    DO(CMD_TBP7, 0xA7) \
    DO(CMD_TBW0, 0xA8) \
    DO(CMD_TBW1, 0xA9) \
    DO(CMD_TBW2, 0xAA) \
    DO(CMD_TBW3, 0xAB) \
    DO(CMD_TBW4, 0xAC) \
    DO(CMD_TBW5, 0xAD) \
    DO(CMD_TBW6, 0xAE) \
    DO(CMD_TBW7, 0xAF) \
    DO(CMD_CBP, 0xB0) \
    DO(CMD_CBW, 0xB1) \
    DO(CMD_XBP1, 0xB2) \
    DO(CMD_XBPW1, 0xB3) \
    DO(CMD_XBP2, 0xB4) \
    DO(CMD_XBPW2, 0xB5) \
    DO(CMD_TSIZE0, 0xB8) \
    DO(CMD_TSIZE1, 0xB9) \
    DO(CMD_TSIZE2, 0xBA) \
    DO(CMD_TSIZE3, 0xBB) \
    DO(CMD_TSIZE4, 0xBC) \
    DO(CMD_TSIZE5, 0xBD) \
    DO(CMD_TSIZE6, 0xBE) \
    DO(CMD_TSIZE7, 0xBF) \
    DO(CMD_TMAP, 0xC0) \
    DO(CMD_TSHADE, 0xC1) \
    DO(CMD_TMODE, 0xC2) \
    DO(CMD_TPF, 0xC3) \
    DO(CMD_CLOAD, 0xC4) \
    DO(CMD_CLUT, 0xC5) \
    DO(CMD_TFILTER, 0xC6) \
    DO(CMD_TWRAP, 0xC7) \
    DO(CMD_TLEVEL, 0xC8) \
    DO(CMD_TFUNC, 0xC9) \
    DO(CMD_TEC, 0xCA) \
    DO(CMD_TFLUSH, 0xCB) \
    DO(CMD_TSYNC, 0xCC) \
    DO(CMD_FOG1, 0xCD) \
    DO(CMD_FOG2, 0xCE) \
    DO(CMD_FC, 0xCF) \
    DO(CMD_TSLOPE, 0xD0) \
    DO(CMD_FPF, 0xD2) \
    DO(CMD_CMODE, 0xD3) \
    DO(CMD_SCISSOR1, 0xD4) \
    DO(CMD_SCISSOR2, 0xD5) \
    DO(CMD_MINZ, 0xD6) \
    DO(CMD_MAXZ, 0xD7) \
    DO(CMD_CTEST, 0xD8) \
    DO(CMD_CREF, 0xD9) \
    DO(CMD_CMSK, 0xDA) \
    DO(CMD_ATEST, 0xDB) \
    DO(CMD_STEST, 0xDC) \
    DO(CMD_SOP, 0xDD) \
    DO(CMD_ZTEST, 0xDE) \
    DO(CMD_BLEND, 0xDF) \
    DO(CMD_FIXA, 0xE0) \
    DO(CMD_FIXB, 0xE1) \
    DO(CMD_DITH1, 0xE2) \
    DO(CMD_DITH2, 0xE3) \
    DO(CMD_DITH3, 0xE4) \
    DO(CMD_DITH4, 0xE5) \
    DO(CMD_LOP, 0xE6) \
    DO(CMD_ZMSK, 0xE7) \
    DO(CMD_PMSK1, 0xE8) \
    DO(CMD_PMSK2, 0xE9) \
    DO(CMD_XSTART, 0xEA) \
    DO(CMD_XPOS1, 0xEB) \
    DO(CMD_XPOS2, 0xEC) \
    DO(CMD_XSIZE, 0xEE) \
    DO(CMD_X2, 0xF0) \
    DO(CMD_Y2, 0xF1) \
    DO(CMD_Z2, 0xF2) \
    DO(CMD_S2, 0xF3) \
    DO(CMD_T2, 0xF4) \
    DO(CMD_Q2, 0xF5) \
    DO(CMD_RGB2, 0xF6) \
    DO(CMD_AP2, 0xF7) \
    DO(CMD_F2, 0xF8) \
    DO(CMD_I2, 0xF9)

#define DO(name, value) name = value,

enum GPUCommand : int {
    X
};

#undef DO

#define DO(name, value) case value: return #name;

const char *getCommandName(uint8_t cmd) {
    switch (cmd) {
        X
    }
    return "(unknown GPU cmd name)";
}

#undef DO

DisplayList *getDisplayListFromQueue(int qid) {
    for (auto& i : dlQueue) {
        if (qid = i.id)
            return &i;
    }
    return nullptr;
}

bool addDisplayListToQueue(const DisplayList& list) {
    if (dlQueue.size() >= DL_QUEUE_MAX_SIZE)
        return false;

    dlQueue.push_back(list);
    return true;
}

void displayListEnd() {
    if (dlQueue.size() == 0)
        return;

    dlQueue.pop_front();
}

DisplayList *getDisplayListFromQueue() {
    if (dlQueue.size() == 0)
        return nullptr;

    return &dlQueue.front();
}

static int drawCount;
static int primitiveDrawCount;

bool displayListInStallAddress(const DisplayList *dl) {
    return dl->currentAddress == dl->stallAddress;
}

void displayListRun(DisplayList *dl, int steps) {
    if (!dl)
        return;

    GPUState *state = getGPUState();

    int instructionsElapsed = 0;

    GPUOpcode *opcode;

    opcode = (GPUOpcode *) Memory::getPointerUnchecked(dl->currentAddress);
    if (dl->state == 1) {
        __RenderDeviceDisplayListBegin();
        dl->state = 2;
    }

    uint32_t oldAddress = 0;
    
    state->textureScaleU = 1.f;
    state->textureScaleV = 1.f;
    state->textureOffsetX = 0.f;
    state->textureOffsetY = 0.f;
    state->vertexInfo = {};
    for (int i = 0; i < steps; i++) {
        if (displayListInStallAddress(dl))
            break;

        if (!opcode) {
            LOG_ERROR(logType, "invalid display list address 0x%08x", dl->currentAddress);
            return;
        }

        switch (opcode->opcode) {
        case CMD_NOP: break;
        case CMD_VADDR:
            state->setVertexListAddress(opcode->parameter);
            break;
        case CMD_IADDR: state->setIndexListAddress(opcode->parameter); break;
        case CMD_PRIM:
        {
            int count = opcode->parameter & 0xFFFF;
            int type = (opcode->parameter >> 16) & 7;
            
            __PrepareDraw(state, type, count);
            __DrawPrimitive(state, type, count);
            __EndDraw(state);

            switch (state->vertexInfo.it) {
            case 0:
                state->vertexListAddress += state->vertexInfo.vertex_size * count;
                break;
            case 1:
                state->indexListAddress += count;
                break;
            case 2:
                state->indexListAddress += 2 * count;
                break;
            }

            primitiveDrawCount += count;
            ++drawCount;

            break;
        }
        case CMD_BEZIER:
            break;
        case CMD_JUMP:
        {
            state->jump(*dl, opcode->parameter);
            opcode = (GPUOpcode *) Memory::getPointerUnchecked(dl->currentAddress);
            continue;
        }
#if 0
        case CMD_BBOX:
            break;
        case CMD_BJUMP:
        {
            state->jump(*dl, opcode->parameter);
            opcode = ((GPUOpcode *) Memory::getPointerUnchecked(dl->currentAddress)) + 1;
            dl->currentAddress += 4;
            continue;
        }
        case CMD_SIGNAL:
            break;
#endif
        case CMD_CALL:
            state->call(*dl, opcode->parameter);
            opcode = (GPUOpcode *) Memory::getPointerUnchecked(dl->currentAddress);
            continue;
        case CMD_RET:
            state->ret(*dl, opcode->parameter);
            opcode = (GPUOpcode *) Memory::getPointerUnchecked(dl->currentAddress);
            continue;
        case CMD_END:
            // printf("%d\n", drawCount);
            drawCount = 0;
            primitiveDrawCount = 0;
            displayListEnd();
            __RenderDeviceDisplayListEnd();
            return;
        case CMD_FINISH:
        {
            Core::Kernel::hleTriggerInterrupt(25, 1, opcode->parameter & 0xFFFF, true);
            break;
        }
        case CMD_BASE: state->setBaseAddress(opcode->parameter); break;
        case CMD_VTYPE: state->setVertexType(opcode->parameter); break;
        case CMD_OFFSET: state->setOffsetAddress(opcode->parameter); break;
        case CMD_ORIGIN: state->setOriginAddress(*dl, opcode->parameter); break;
        case CMD_REGION1: state->setDrawingRegion1(opcode->parameter); break;
        case CMD_REGION2: state->setDrawingRegion2(opcode->parameter); break;
        case CMD_LTE: state->setGlobalLightingEnable(opcode->parameter); break;
        case CMD_LE0:
        case CMD_LE1:
        case CMD_LE2:
        case CMD_LE3: state->setLightingEnable(opcode->opcode - CMD_LE0, opcode->parameter); break;
        case CMD_CLE: state->setClippingEnable(opcode->parameter); break;
        case CMD_BCE: state->setCullingEnable(opcode->parameter); break;
        case CMD_TME: state->setTextureEnable(opcode->parameter); break;
        case CMD_FGE: state->setFogEnable(opcode->parameter); break;
        case CMD_DTE: state->setDitherEnable(opcode->parameter); break;
        case CMD_ABE: state->setAlphaBlendingEnable(opcode->parameter); break;
        case CMD_ATE: state->setAlphaTestEnable(opcode->parameter); break;
        case CMD_ZTE: state->setDepthTestEnable(opcode->parameter); break;
        case CMD_STE: state->setStencilTestEnable(opcode->parameter); break;
        case CMD_AAE: state->setAntialiasingEnable(opcode->parameter); break;
        case CMD_PCE: state->setPatchCullingEnable(opcode->parameter); break;
        case CMD_CTE: state->setColorTestEnable(opcode->parameter); break;
        case CMD_LOE: state->setLogicalOperationEnable(opcode->parameter); break;
        case CMD_BONEN: state->setBoneMatrixNumber(opcode->parameter); break;
        case CMD_BONED: state->setBoneMatrixData(opcode->parameter); break;
        case CMD_WEIGHT0:
        case CMD_WEIGHT1:
        case CMD_WEIGHT2:
        case CMD_WEIGHT3:
        case CMD_WEIGHT4:
        case CMD_WEIGHT5:
        case CMD_WEIGHT6:
        case CMD_WEIGHT7: state->setVertexWeight(opcode->opcode - CMD_WEIGHT0, opcode->parameter); break;
        case CMD_DIVIDE: state->setPatchDivisionCount(opcode->parameter); break;
        case CMD_PPM: state->setPatchPrimitive(opcode->parameter); break;
        case CMD_PFACE: state->setPatchFace(opcode->parameter); break;
        case CMD_WORLDN: state->setWorldMatrixNumber(opcode->parameter); break;
        case CMD_WORLDD: state->setWorldMatrixData(opcode->parameter); break;
        case CMD_VIEWN: state->setViewMatrixNumber(opcode->parameter); break;
        case CMD_VIEWD: state->setViewMatrixData(opcode->parameter); break;
        case CMD_PROJN: state->setProjectionMatrixNumber(opcode->parameter); break;
        case CMD_PROJD: state->setProjectionMatrixData(opcode->parameter); break;
        case CMD_TGENN: state->setTexGenMatrixNumber(opcode->parameter); break;
        case CMD_TGEND: state->setTexGenMatrixData(opcode->parameter); break;
        case CMD_SX: state->setSX(opcode->parameter); break;
        case CMD_SY: state->setSY(opcode->parameter); break;
        case CMD_SZ: state->setSZ(opcode->parameter); break;
        case CMD_TX: state->setTX(opcode->parameter); break;
        case CMD_TY: state->setTY(opcode->parameter); break;
        case CMD_SU: state->setSU(opcode->parameter); break;
        case CMD_SV: state->setSV(opcode->parameter); break;
        case CMD_TZ: state->setTZ(opcode->parameter); break;
        case CMD_TU: state->setTU(opcode->parameter); break;
        case CMD_TV: state->setTV(opcode->parameter); break;
        case CMD_OFFSETX: state->setScreenOffsetX(opcode->parameter); break;
        case CMD_OFFSETY: state->setScreenOffsetY(opcode->parameter); break;
        case CMD_SHADE: state->setShadingMode(opcode->parameter); break;
        case CMD_NREV: state->setNormalReverse(opcode->parameter); break;
        case CMD_MATERIAL: state->setMaterial(opcode->parameter); break;
        case CMD_MEC: state->setMEC(opcode->parameter); break;
        case CMD_MAC: state->setMAC(opcode->parameter); break;
        case CMD_MSC: state->setMSC(opcode->parameter); break;
        case CMD_MAA: state->setModelColorAlpha(opcode->parameter); break;
        case CMD_MK: state->setMK(opcode->parameter); break;
        case CMD_CULL: state->setCullingSurface(opcode->parameter); break;
        case CMD_FBP: break;
        case CMD_FBW: break;
        case CMD_ZBP: break;
        case CMD_ZBW: break;
        case CMD_TBP0:
        case CMD_TBP1:
        case CMD_TBP2:
        case CMD_TBP3:
        case CMD_TBP4:
        case CMD_TBP5:
        case CMD_TBP6:
        case CMD_TBP7: state->setTextureBufferBasePointer(opcode->opcode - CMD_TBP0, opcode->parameter); break;
        case CMD_TBW0:
        case CMD_TBW1:
        case CMD_TBW2:
        case CMD_TBW3:
        case CMD_TBW4:
        case CMD_TBW5:
        case CMD_TBW6:
        case CMD_TBW7: state->setTextureBufferWidth(opcode->opcode - CMD_TBW0, opcode->parameter); break;
        case CMD_CBP: state->setCLUTBasePointer(opcode->parameter); break;
        case CMD_CBW: state->setUpperCLUTBasePointer(opcode->parameter); break;
        case CMD_XBP1: break;
        case CMD_XBPW1: break;
        case CMD_XBP2: break;
        case CMD_XBPW2: break;
        case CMD_TSIZE0:
        case CMD_TSIZE1:
        case CMD_TSIZE2:
        case CMD_TSIZE3:
        case CMD_TSIZE4:
        case CMD_TSIZE5:
        case CMD_TSIZE6:
        case CMD_TSIZE7: state->setTextureSize(opcode->opcode - CMD_TSIZE0, opcode->parameter); break;
        case CMD_TMAP: state->setTextureMappingMode(opcode->parameter); break;
        case CMD_TSHADE: state->setTextureShadeMapping(opcode->parameter); break;
        case CMD_TMODE: state->setTextureMode(opcode->parameter); break;
        case CMD_TPF: state->setTexturePixelFormat(opcode->parameter); break;
        case CMD_CLOAD: state->setCLUTLoad(opcode->parameter); break;
        case CMD_CLUT: state->setCLUT(opcode->parameter); break;
        case CMD_TFILTER: state->setTextureFilter(opcode->parameter); break;
        case CMD_TWRAP: state->setTextureWrapMode(opcode->parameter); break;
        case CMD_TLEVEL: state->setTextureLevelMode(opcode->parameter); break;
        case CMD_TFUNC: state->setTextureFunction(opcode->parameter); break;
        case CMD_TEC: state->setTextureEnvironmentColor(opcode->parameter); break;
        case CMD_TFLUSH: break;
        case CMD_TSYNC: break;
        case CMD_FOG1: state->setFogParameter(0, opcode->parameter); break;
        case CMD_FOG2: state->setFogParameter(1, opcode->parameter); break;
        case CMD_FC: state->setFogColor(opcode->parameter); break;
        case CMD_TSLOPE: state->setTextureSlope(opcode->parameter); break;
        case CMD_FPF: state->setFramePixelFormat(opcode->parameter); break;
        case CMD_CMODE: state->setClearMode(opcode->parameter); break;
        case CMD_SCISSOR1: state->setScissoringAreaLowerRight(opcode->parameter); break;
        case CMD_SCISSOR2: state->setScissoringAreaUpperLeft(opcode->parameter); break;
        case CMD_MINZ: state->setMinDepthRange(opcode->parameter); break;
        case CMD_MAXZ: state->setMaxDepthRange(opcode->parameter); break;
        case CMD_CTEST: state->setColorTestFunction(opcode->parameter); break;
        case CMD_CREF: state->setColorReference(opcode->parameter); break;
        case CMD_CMSK: state->setColorMask(opcode->parameter); break;
        case CMD_ATEST: state->setAlphaTest(opcode->parameter); break;
        case CMD_STEST: state->setStencilTest(opcode->parameter); break;
        case CMD_SOP: state->setStencilOperation(opcode->parameter); break;
        case CMD_ZTEST: state->setDepthTestFunction(opcode->parameter); break;
        case CMD_BLEND: state->setBlend(opcode->parameter); break;
        case CMD_FIXA: state->setFixA(opcode->parameter); break;
        case CMD_FIXB: state->setFixB(opcode->parameter); break;
        default:
            if (opcode->opcode < 0x17) {
                LOG_ERROR(logType, "unimplemented GPU opcode 0x%02X (%s) instruction: 0x%08X list PC: 0x%08X", opcode->opcode, getCommandName(opcode->opcode),
                                                            opcode->instruction, dl->currentAddress);
                Core::Allegrex::setProcessorFailed(true);
                return;
            }
            break;
        }

        dl->currentAddress += 4;
        opcode++;
    }
}
}