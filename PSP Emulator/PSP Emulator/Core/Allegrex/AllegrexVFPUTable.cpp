#include <Core/Allegrex/AllegrexVFPUTable.h>
#include <Core/Allegrex/AllegrexVFPU.h>
#include <Core/Allegrex/AllegrexState.h>
#include <Core/Allegrex/Allegrex.h>
#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

#define R(i)   (cpu.reg[i])
#define V(i)   (cpu.vprf[voffset[i]])
#define VI(i)  (cpu.vpr[voffset[i]])
#define FI(i)  (cpu.fi[i])
#define FsI(i) (cpu.fs[i])
#define PC     (cpu.pc)

#define _RS   ((op>>21) & 0x1F)
#define _RT   ((op>>16) & 0x1F)
#define _RD   ((op>>11) & 0x1F)
#define _FS   ((op>>11) & 0x1F)
#define _FT   ((op>>16) & 0x1F)
#define _FD   ((op>>6 ) & 0x1F)
#define _POS  ((op>>6 ) & 0x1F)
#define _SIZE ((op>>11) & 0x1F)

#define HI cpu.hi
#define LO cpu.lo

#ifndef M_LOG2E
#define M_E        2.71828182845904523536f
#define M_LOG2E    1.44269504088896340736f
#define M_LOG10E   0.434294481903251827651f
#define M_LN2      0.693147180559945309417f
#define M_LN10     2.30258509299404568402f
#undef M_PI
#define M_PI       3.14159265358979323846f
#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923f
#endif
#define M_PI_4     0.785398163397448309616f
#define M_1_PI     0.318309886183790671538f
#define M_2_PI     0.636619772367581343076f
#define M_2_SQRTPI 1.12837916709551257390f
#define M_SQRT2    1.41421356237309504880f
#define M_SQRT1_2  0.707106781186547524401f
#endif

static const bool USE_VFPU_DOT = true;
static const bool USE_VFPU_SQRT = true;

enum VCondition
{
    VC_FL,
    VC_EQ,
    VC_LT,
    VC_LE,
    VC_TR,
    VC_NE,
    VC_GE,
    VC_GT,
    VC_EZ,
    VC_EN,
    VC_EI,
    VC_ES,
    VC_NZ,
    VC_NN,
    VC_NI,
    VC_NS
};

namespace Core::Memory {
    void *GetPointerRange(uint32_t addr, int size) {
        return getPointerUnchecked(addr);
    }

    void *GetPointerWriteRange(uint32_t addr, int size) {
        return getPointerUnchecked(addr);
    }

    uint32_t Read_U32(uint32_t addr) {
        return read32(addr);
    }

    void Write_Float(float data, uint32_t addr) {
        float *p = (float *) getPointer(addr);
        if (p)
            *p = data;
    }

    float Read_Float(uint32_t addr) {
        float *p = (float *) getPointer(addr);
        if (p)
            return *p;
        return 0.f;
    }

    void Write_U32(uint32_t data, uint32_t addr) {
        write32(addr, data);
    }
}

using namespace Core::Allegrex;
using namespace Core;

union FloatBits {
    float f[4];
    u32 u[4];
    int i[4];
};

typedef unsigned short float16;

inline uint32_t BytesToUint32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
    return (a) | (b << 8) | (c << 16) | (d << 24);
}

constexpr int32_t SignExtend8ToS32(uint32_t value) {
    // This extends this sign at the 8th bit to the other 24 bits.
    return (int8_t)(value & 0xFF);
}

constexpr uint32_t SignExtend8ToU32(uint32_t value) {
    // Just treat the bits as unsigned.
    return (uint32_t)SignExtend8ToS32(value);
}

constexpr int32_t SignExtend16ToS32(uint32_t value) {
    // Same as SignExtend8toS32, but from the 16th bit.
    return (int16_t)(value & 0xFFFF);
}

constexpr uint32_t SignExtend16ToU32(uint32_t value) {
    return (uint32_t)SignExtend16ToS32(value);
}

// This ain't a 1.5.10 float16, it's a stupid hack format where we chop 16 bits off a float.
// This choice is subject to change. Don't think I'm using this for anything at all now anyway.
// DEPRECATED
inline float16 FloatToFloat16(float x) {
    int ix;
    memcpy(&ix, &x, sizeof(float));
    return ix >> 16;
}

inline float Float16ToFloat(float16 ix) {
    float x;
    memcpy(&x, &ix, sizeof(float));
    return x;
}

inline bool isPowerOf2(int n) {
    return n == 1 || (n & (n - 1)) == 0;
}

// Next power of 2.
inline uint32_t RoundUpToPowerOf2(uint32_t v) {
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}

inline uint32_t RoundUpToPowerOf2(uint32_t v, uint32_t power) {
    return (v + power - 1) & ~(power - 1);
}

inline uint32_t log2i(uint32_t val) {
    unsigned int ret = -1;
    while (val != 0) {
        val >>= 1; ret++;
    }
    return ret;
}

#define PI 3.141592653589793f
#ifndef M_PI
#define M_PI 3.141592653589793f
#endif

template<class T>
inline T clamp_value(T val, T floor, T cap) {
    if (val > cap)
        return cap;
    else if (val < floor)
        return floor;
    else
        return val;
}

// Very common operation, familiar from shaders.
inline float saturatef(float x) {
    if (x > 1.0f) return 1.0f;
    else if (x < 0.0f) return 0.0f;
    else return x;
}

#define ROUND_UP(x, a)   (((x) + (a) - 1) & ~((a) - 1))
#define ROUND_DOWN(x, a) ((x) & ~((a) - 1))

template<class T>
inline void Clamp(T* val, const T& min, const T& max)
{
    if (*val < min)
        *val = min;
    else if (*val > max)
        *val = max;
}

template<class T>
inline T Clamp(const T val, const T& min, const T& max)
{
    T ret = val;
    Clamp(&ret, min, max);
    return ret;
}

union FP32 {
    uint32_t u;
    float f;
};

struct FP16 {
    uint16_t u;
};

inline bool my_isinf(float f) {
    FP32 f2u;
    f2u.f = f;
    return f2u.u == 0x7f800000 ||
        f2u.u == 0xff800000;
}

inline bool my_isnan(float f) {
    FP32 f2u;
    f2u.f = f;
    // NaNs have non-zero mantissa
    return ((f2u.u & 0x7F800000) == 0x7F800000) && (f2u.u & 0x7FFFFF);
}

inline bool my_isnanorinf(float f) {
    FP32 f2u;
    f2u.f = f;
    // NaNs have non-zero mantissa, infs have zero mantissa. That is, we just ignore the mantissa here.
    return ((f2u.u & 0x7F800000) == 0x7F800000);
}

inline float InfToZero(float f) {
    return my_isinf(f) ? 0.0f : f;
}

inline int is_even(float d) {
    float int_part;
    modff(d / 2.0f, &int_part);
    return 2.0f * int_part == d;
}

// Rounds *.5 to closest even number
inline double round_ieee_754(double d) {
    float i = (float)floor(d);
    d -= i;
    if (d < 0.5f)
        return i;
    if (d > 0.5f)
        return i + 1.0f;
    if (is_even(i))
        return i;
    return i + 1.0f;
}

// magic code from ryg: http://fgiesen.wordpress.com/2012/03/28/half-to-float-done-quic/
// See also SSE2 version: https://gist.github.com/rygorous/2144712
inline FP32 half_to_float_fast5(FP16 h)
{
    static const FP32 magic = { (127 + (127 - 15)) << 23 };
    static const FP32 was_infnan = { (127 + 16) << 23 };
    FP32 o;
    o.u = (h.u & 0x7fff) << 13;     // exponent/mantissa bits
    o.f *= magic.f;                 // exponent adjust
    if (o.f >= was_infnan.f)        // make sure Inf/NaN survive (retain the low bits)
        o.u = (255 << 23) | (h.u & 0x03ff);
    o.u |= (h.u & 0x8000) << 16;    // sign bit
    return o;
}

inline float ExpandHalf(uint16_t half) {
    FP16 fp16;
    fp16.u = half;
    FP32 fp = half_to_float_fast5(fp16);
    return fp.f;
}

// More magic code: https://gist.github.com/rygorous/2156668
inline FP16 float_to_half_fast3(FP32 f)
{
    static const FP32 f32infty = { 255 << 23 };
    static const FP32 f16infty = { 31 << 23 };
    static const FP32 magic = { 15 << 23 };
    static const uint32_t sign_mask = 0x80000000u;
    static const uint32_t round_mask = ~0xfffu;
    FP16 o = { 0 };

    uint32_t sign = f.u & sign_mask;
    f.u ^= sign;

    if (f.u >= f32infty.u) // Inf or NaN (all exponent bits set)
        o.u = (f.u > f32infty.u) ? (0x7e00 | (f.u & 0x3ff)) : 0x7c00; // NaN->qNaN and Inf->Inf
    else // (De)normalized number or zero
    {
        f.u &= round_mask;
        f.f *= magic.f;
        f.u -= round_mask;
        if (f.u > f16infty.u) f.u = f16infty.u; // Clamp to signed infinity if overflowed

        o.u = f.u >> 13; // Take the bits!
    }

    o.u |= sign >> 16;
    return o;
}

inline uint16_t ShrinkToHalf(float full) {
    FP32 fp32;
    fp32.f = full;
    FP16 fp = float_to_half_fast3(fp32);
    return fp.u;
}

// Preserves NaN in first param, takes sign of equal second param.
// Technically, std::max may do this but it's undefined.
inline float nanmax(float f, float cst)
{
    return f <= cst ? cst : f;
}

// Preserves NaN in first param, takes sign of equal second param.
inline float nanmin(float f, float cst)
{
    return f >= cst ? cst : f;
}

// Preserves NaN in first param, takes sign of equal value in others.
inline float nanclamp(float f, float lower, float upper)
{
    return nanmin(nanmax(f, lower), upper);
}

static void ApplyPrefixST(float *r, u32 data, VectorSize size, float invalid = 0.0f) {
    // Check for no prefix.
    if (data == 0xe4)
        return;

    int n = GetNumVectorElements(size);
    float origV[4]{ invalid, invalid, invalid, invalid };
    static const float constantArray[8] = {0.f, 1.f, 2.f, 0.5f, 3.f, 1.f/3.f, 0.25f, 1.f/6.f};

    for (int i = 0; i < n; i++) {
        origV[i] = r[i];
    }

    for (int i = 0; i < n; i++) {
        int regnum = (data >> (i*2)) & 3;
        int abs    = (data >> (8+i)) & 1;
        int negate = (data >> (16+i)) & 1;
        int constants = (data >> (12+i)) & 1;

        if (!constants) {
            r[i] = origV[regnum];
            if (abs)
                ((u32 *)r)[i] = ((u32 *)r)[i] & 0x7FFFFFFF;
        } else {
            r[i] = constantArray[regnum + (abs<<2)];
        }

        if (negate)
            ((u32 *)r)[i] = ((u32 *)r)[i] ^ 0x80000000;
    }
}

inline void ApplySwizzleS(float *v, VectorSize size, float invalid = 0.0f)
{
    ApplyPrefixST(v, cpu.vfpuCtrl[VFPU_CTRL_SPREFIX], size, invalid);
}

inline void ApplySwizzleT(float *v, VectorSize size, float invalid = 0.0f)
{
    ApplyPrefixST(v, cpu.vfpuCtrl[VFPU_CTRL_TPREFIX], size, invalid);
}

void ApplyPrefixD(float *v, VectorSize size, bool onlyWriteMask = false)
{
    u32 data = cpu.vfpuCtrl[VFPU_CTRL_DPREFIX];
    if (!data || onlyWriteMask)
        return;
    int n = GetNumVectorElements(size);
    for (int i = 0; i < n; i++)
    {
        int sat = (data >> (i * 2)) & 3;
        if (sat == 1)
            v[i] = vfpu_clamp(v[i], 0.0f, 1.0f);
        else if (sat == 3)
            v[i] = vfpu_clamp(v[i], -1.0f, 1.0f);
    }
}

static void RetainInvalidSwizzleST(float *d, VectorSize sz) {
    // Somehow it's like a supernan, maybe wires through to zero?
    // Doesn't apply to all ops.
    int sPrefix = cpu.vfpuCtrl[VFPU_CTRL_SPREFIX];
    int tPrefix = cpu.vfpuCtrl[VFPU_CTRL_TPREFIX];
    int n = GetNumVectorElements(sz);

    // TODO: We can probably do some faster check of sPrefix and tPrefix to skip over this loop.
    for (int i = 0; i < n; i++) {
        int swizzleS = (sPrefix >> (i + i)) & 3;
        int swizzleT = (tPrefix >> (i + i)) & 3;
        int constS = (sPrefix >> (12 + i)) & 1;
        int constT = (tPrefix >> (12 + i)) & 1;
        if ((swizzleS >= n && !constS) || (swizzleT >= n && !constT))
            d[i] = 0.0f;
    }
}

void EatPrefixes()
{
    cpu.vfpuCtrl[VFPU_CTRL_SPREFIX] = 0xe4;  // passthru
    cpu.vfpuCtrl[VFPU_CTRL_TPREFIX] = 0xe4;  // passthru
    cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = 0;
}

namespace Core::Allegrex {
static const char *logType = "VFPU";

static int voffset[128] = {
    0x00, 0x04, 0x08, 0x0c, 0x10, 0x14, 0x18, 0x1c, 0x20, 0x24, 0x28, 0x2c, 0x30, 0x34, 0x38, 0x3c, 0x40, 0x44, 0x48, 0x4c, 0x50, 0x54, 0x58, 0x5c, 0x60, 0x64, 0x68, 0x6c, 0x70, 0x74, 0x78, 0x7c, 0x01, 0x05, 0x09, 0x0d, 0x11, 0x15, 0x19, 0x1d, 0x21, 0x25, 0x29, 0x2d, 0x31, 0x35, 0x39, 0x3d, 0x41, 0x45, 0x49, 0x4d, 0x51, 0x55, 0x59, 0x5d, 0x61, 0x65, 0x69, 0x6d, 0x71, 0x75, 0x79, 0x7d, 0x02, 0x06, 0x0a, 0x0e, 0x12, 0x16, 0x1a, 0x1e, 0x22, 0x26, 0x2a, 0x2e, 0x32, 0x36, 0x3a, 0x3e, 0x42, 0x46, 0x4a, 0x4e, 0x52, 0x56, 0x5a, 0x5e, 0x62, 0x66, 0x6a, 0x6e, 0x72, 0x76, 0x7a, 0x7e, 0x03, 0x07, 0x0b, 0x0f, 0x13, 0x17, 0x1b, 0x1f, 0x23, 0x27, 0x2b, 0x2f, 0x33, 0x37, 0x3b, 0x3f, 0x43, 0x47, 0x4b, 0x4f, 0x53, 0x57, 0x5b, 0x5f, 0x63, 0x67, 0x6b, 0x6f, 0x73, 0x77, 0x7b, 0x7f,
};

typedef void (*MIPSInstruction)(uint32_t op);

void Int_SV(MIPSOpcode op);
void Int_SVQ(MIPSOpcode op);
void Int_Mftv(MIPSOpcode op);
void Int_VecDo3(MIPSOpcode op);
void Int_Vcst(MIPSOpcode op);
void Int_VMatrixInit(MIPSOpcode op);
void Int_VVectorInit(MIPSOpcode op);
void Int_Vmmul(MIPSOpcode op);
void Int_Vmscl(MIPSOpcode op);
void Int_Vmmov(MIPSOpcode op);
void Int_VV2Op(MIPSOpcode op);
void Int_Vrot(MIPSOpcode op);
void Int_VDot(MIPSOpcode op);
void Int_VHdp(MIPSOpcode op);
void Int_Vavg(MIPSOpcode op);
void Int_Vfad(MIPSOpcode op);
void Int_Vocp(MIPSOpcode op);
void Int_Vsocp(MIPSOpcode op);
void Int_Vsgn(MIPSOpcode op);
void Int_Vtfm(MIPSOpcode op);
void Int_Viim(MIPSOpcode op);
void Int_VScl(MIPSOpcode op);
void Int_Vidt(MIPSOpcode op);
void Int_Vcmp(MIPSOpcode op);
void Int_Vminmax(MIPSOpcode op);
void Int_Vscmp(MIPSOpcode op);
void Int_Vcrs(MIPSOpcode op);
void Int_Vdet(MIPSOpcode op);
void Int_Vcmov(MIPSOpcode op);
void Int_CrossQuat(MIPSOpcode op);
void Int_VPFX(MIPSOpcode op);
void Int_Vflush(MIPSOpcode op);
void Int_Vbfy(MIPSOpcode op);
void Int_Vsrt1(MIPSOpcode op);
void Int_Vsrt2(MIPSOpcode op);
void Int_Vsrt3(MIPSOpcode op);
void Int_Vsrt4(MIPSOpcode op);
void Int_Vf2i(MIPSOpcode op);
void Int_Vi2f(MIPSOpcode op);
void Int_Vi2x(MIPSOpcode op);
void Int_Vx2i(MIPSOpcode op);
void Int_VBranch(MIPSOpcode op);
void Int_Vrnds(MIPSOpcode op);
void Int_VrndX(MIPSOpcode op);
void Int_ColorConv(MIPSOpcode op);
void Int_Vh2f(MIPSOpcode op);
void Int_Vf2h(MIPSOpcode op);
void Int_Vsge(MIPSOpcode op);
void Int_Vslt(MIPSOpcode op);
void Int_Vmfvc(MIPSOpcode op);
void Int_Vmtvc(MIPSOpcode op);
void Int_Vlgb(MIPSOpcode op);
void Int_Vwbn(MIPSOpcode op);
void Int_Vsbn(MIPSOpcode op);
void Int_Vsbz(MIPSOpcode op);

#define ENCODING(n) handle##n
#define INSTR(name, comp, dis, inter, flags) inter

#define INVALID nullptr
#define INVALID_X_8 INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID,INVALID

void handleCop2(uint32_t op);
void handleCop2BC2(uint32_t op);
void handleVFPU0(uint32_t op);
void handleVFPU1(uint32_t op);
void handleVFPU3(uint32_t op);
void handleVFPU4Jump(uint32_t op);
void handleVFPU5(uint32_t op);
void handleVFPU6(uint32_t op);
void handleVFPU4(uint32_t op);
void handleVFPU7(uint32_t op);
void handleVFPU9(uint32_t op);
void handleVFPUMatrix1(uint32_t op);

// regregreg instructions
static const MIPSInstruction tableImmediate[64] = // xxxxxx ..... ..... ................
{
    //0
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    //8
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    //16
    INVALID, //cop0
    INVALID, //cop1
    ENCODING(Cop2), //cop2
    INVALID, //copU

    INVALID, //L = likely
    INVALID,
    INVALID,
    INVALID,
    //24
    ENCODING(VFPU0),
    ENCODING(VFPU1),
    INVALID,
    ENCODING(VFPU3),
    INVALID, //special2
    INVALID,
    INVALID,
    INVALID, //special3
    //32
    INVALID_X_8,
    //40
    INVALID_X_8,
    //48
    INVALID,
    INVALID,
    INSTR("lv.s", JITFUNC(Comp_SV), Dis_SV, Int_SV, IN_MEM|IN_IMM16|IN_RS_ADDR|OUT_OTHER|IS_VFPU|VFPU_NO_PREFIX|MEMTYPE_FLOAT),
    INVALID,
    ENCODING(VFPU4Jump),
    INSTR("lv", JITFUNC(Comp_SVQ), Dis_SVLRQ, Int_SVQ, IN_MEM|IN_IMM16|IN_RS_ADDR|OUT_OTHER|IS_VFPU|VFPU_NO_PREFIX|MEMTYPE_VQUAD),
    INSTR("lv.q", JITFUNC(Comp_SVQ), Dis_SVQ, Int_SVQ, IN_MEM|IN_IMM16|IN_RS_ADDR|OUT_OTHER|IS_VFPU|VFPU_NO_PREFIX|MEMTYPE_VQUAD), //copU
    ENCODING(VFPU5),
    //56
    INVALID,
    INVALID,
    INSTR("sv.s", JITFUNC(Comp_SV), Dis_SV, Int_SV, IN_IMM16|IN_RS_ADDR|IN_OTHER|OUT_MEM|IS_VFPU|VFPU_NO_PREFIX|MEMTYPE_FLOAT),
    INVALID,
    //60
    ENCODING(VFPU6),
    INSTR("sv", JITFUNC(Comp_SVQ), Dis_SVLRQ, Int_SVQ, IN_IMM16|IN_RS_ADDR|IN_OTHER|OUT_MEM|IS_VFPU|VFPU_NO_PREFIX|MEMTYPE_VQUAD), //copU
    INSTR("sv.q", JITFUNC(Comp_SVQ), Dis_SVQ, Int_SVQ, IN_IMM16|IN_RS_ADDR|IN_OTHER|OUT_MEM|IS_VFPU|VFPU_NO_PREFIX|MEMTYPE_VQUAD),
    // Some call this VFPU7 (vflush/vnop/vsync), but it's not super important.
    INVALID
};

static const MIPSInstruction tableVFPU0[8] = // 011000 xxx ....... . ....... . .......
{
    INSTR("vadd", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_VecDo3, MIPSInfo(IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX, 2)),
    INSTR("vsub", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_VecDo3, MIPSInfo(IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX, 2)),
    // TODO: Disasm is wrong.
    INSTR("vsbn", JITFUNC(Comp_Generic), Dis_VectorSet3, Int_Vsbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID, INVALID, INVALID, INVALID,

    INSTR("vdiv", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_VecDo3, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
};

static const MIPSInstruction tableVFPU1[8] = // 011001 xxx ....... . ....... . .......
{
    INSTR("vmul", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_VecDo3, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vdot", JITFUNC(Comp_VDot), Dis_VectorDot, Int_VDot, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vscl", JITFUNC(Comp_VScl), Dis_VScl, Int_VScl, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INSTR("vhdp", JITFUNC(Comp_VHdp), Dis_VectorDot, Int_VHdp, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vcrs", JITFUNC(Comp_VCrs), Dis_Vcrs, Int_Vcrs, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vdet", JITFUNC(Comp_VDet), Dis_VectorDot, Int_Vdet, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
};

static const MIPSInstruction tableVFPU3[8] = // 011011 xxx ....... . ....... . .......
{
    INSTR("vcmp", JITFUNC(Comp_Vcmp), Dis_Vcmp, Int_Vcmp, IN_OTHER|OUT_VFPU_CC|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INSTR("vmin", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_Vminmax, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmax", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_Vminmax, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INSTR("vscmp", JITFUNC(Comp_Generic), Dis_VectorSet3, Int_Vscmp, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsge", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_Vsge, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vslt", JITFUNC(Comp_VecDo3), Dis_VectorSet3, Int_Vslt, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
};

static const MIPSInstruction tableVFPU4Jump[32] = // 110100 xxxxx ..... . ....... . .......
{
    ENCODING(VFPU4),
    ENCODING(VFPU7),
    ENCODING(VFPU9),
    INSTR("vcst", JITFUNC(Comp_Vcst), Dis_Vcst, Int_Vcst, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID, INVALID, INVALID, INVALID,

    //8
    INVALID_X_8,

    //16
    INSTR("vf2in", JITFUNC(Comp_Vf2i), Dis_Vf2i, Int_Vf2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vf2iz", JITFUNC(Comp_Vf2i), Dis_Vf2i, Int_Vf2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vf2iu", JITFUNC(Comp_Vf2i), Dis_Vf2i, Int_Vf2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vf2id", JITFUNC(Comp_Vf2i), Dis_Vf2i, Int_Vf2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //20
    INSTR("vi2f", JITFUNC(Comp_Vi2f), Dis_Vf2i, Int_Vi2f, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vcmov", JITFUNC(Comp_Vcmov), Dis_Vcmov, Int_Vcmov, IN_OTHER|IN_VFPU_CC|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INVALID,
    //24 - 110100 11 ........ . ....... . .......
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vwbn", JITFUNC(Comp_Generic), Dis_Vwbn, Int_Vwbn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
};

static const MIPSInstruction tableVFPU7[32] = // 110100 00001 xxxxx . ....... . .......
{
    INSTR("vrnds", JITFUNC(Comp_Generic), Dis_Vrnds, Int_Vrnds, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vrndi", JITFUNC(Comp_Generic), Dis_VrndX, Int_VrndX, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vrndf1", JITFUNC(Comp_Generic), Dis_VrndX, Int_VrndX, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vrndf2", JITFUNC(Comp_Generic), Dis_VrndX, Int_VrndX, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INVALID, INVALID, INVALID, INVALID,
    //8
    INVALID, INVALID, INVALID, INVALID,
    INVALID, INVALID, INVALID, INVALID,
    //16
    INVALID,
    INVALID,
    INSTR("vf2h", JITFUNC(Comp_Generic), Dis_Vf2h, Int_Vf2h, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vh2f", JITFUNC(Comp_Vh2f), Dis_Vh2f, Int_Vh2f, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INVALID,
    INVALID,
    INSTR("vsbz", JITFUNC(Comp_Generic), Dis_Generic, Int_Vsbz, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vlgb", JITFUNC(Comp_Generic), Dis_Generic, Int_Vlgb, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //24
    INSTR("vuc2i", JITFUNC(Comp_Vx2i), Dis_Vs2i, Int_Vx2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),  // Seen in BraveStory, initialization  110100 00001110000 000 0001 0000 0000
    INSTR("vc2i",  JITFUNC(Comp_Vx2i), Dis_Vs2i, Int_Vx2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vus2i", JITFUNC(Comp_Vx2i), Dis_Vs2i, Int_Vx2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vs2i",  JITFUNC(Comp_Vx2i), Dis_Vs2i, Int_Vx2i, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INSTR("vi2uc", JITFUNC(Comp_Vi2x), Dis_Vi2x, Int_Vi2x, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vi2c",  JITFUNC(Comp_Vi2x), Dis_Vi2x, Int_Vi2x, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vi2us", JITFUNC(Comp_Vi2x), Dis_Vi2x, Int_Vi2x, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vi2s",  JITFUNC(Comp_Vi2x), Dis_Vi2x, Int_Vi2x, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
};

// 110100 00000 10100 0000000000000000
// 110100 00000 10111 0000000000000000
static const MIPSInstruction tableVFPU4[32] = // 110100 00000 xxxxx . ....... . .......
{
    INSTR("vmov", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vabs", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vneg", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vidt", JITFUNC(Comp_VIdt), Dis_VectorSet1, Int_Vidt, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsat0", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsat1", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vzero", JITFUNC(Comp_VVectorInit), Dis_VectorSet1, Int_VVectorInit, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vone",  JITFUNC(Comp_VVectorInit), Dis_VectorSet1, Int_VVectorInit, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //8
    INVALID_X_8,
    //16
    INSTR("vrcp", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vrsq", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsin", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vcos", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vexp2", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vlog2", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsqrt", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vasin", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //24
    INSTR("vnrcp", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INSTR("vnsin", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INSTR("vrexp2", JITFUNC(Comp_VV2Op), Dis_VectorSet2, Int_VV2Op, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID, INVALID, INVALID,
};

static const MIPSInstruction tableVFPU5[8] = // 110111 xxx ....... ................
{
    INSTR("vpfxs", JITFUNC(Comp_VPFX), Dis_VPFXST, Int_VPFX, IN_IMM16|OUT_OTHER|IS_VFPU),
    INSTR("vpfxs", JITFUNC(Comp_VPFX), Dis_VPFXST, Int_VPFX, IN_IMM16|OUT_OTHER|IS_VFPU),
    INSTR("vpfxt", JITFUNC(Comp_VPFX), Dis_VPFXST, Int_VPFX, IN_IMM16|OUT_OTHER|IS_VFPU),
    INSTR("vpfxt", JITFUNC(Comp_VPFX), Dis_VPFXST, Int_VPFX, IN_IMM16|OUT_OTHER|IS_VFPU),
    INSTR("vpfxd", JITFUNC(Comp_VPFX), Dis_VPFXD,  Int_VPFX, IN_IMM16|OUT_OTHER|IS_VFPU),
    INSTR("vpfxd", JITFUNC(Comp_VPFX), Dis_VPFXD,  Int_VPFX, IN_IMM16|OUT_OTHER|IS_VFPU),
    INSTR("viim.s", JITFUNC(Comp_Viim), Dis_Viim, Int_Viim, IN_IMM16|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vfim.s", JITFUNC(Comp_Vfim), Dis_Viim, Int_Viim, IN_IMM16|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
};

static const MIPSInstruction tableVFPU6[32] = // 111100 xxxxx ..... . ....... . .......
{
    //0
    INSTR("vmmul", JITFUNC(Comp_Vmmul), Dis_MatrixMult, Int_Vmmul, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmmul", JITFUNC(Comp_Vmmul), Dis_MatrixMult, Int_Vmmul, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmmul", JITFUNC(Comp_Vmmul), Dis_MatrixMult, Int_Vmmul, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmmul", JITFUNC(Comp_Vmmul), Dis_MatrixMult, Int_Vmmul, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INSTR("v(h)tfm2", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm2", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm2", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm2", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //8
    INSTR("v(h)tfm3", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm3", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm3", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm3", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INSTR("v(h)tfm4", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm4", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm4", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("v(h)tfm4", JITFUNC(Comp_Vtfm), Dis_Vtfm, Int_Vtfm, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //16
    INSTR("vmscl", JITFUNC(Comp_Vmscl), Dis_Vmscl, Int_Vmscl, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmscl", JITFUNC(Comp_Vmscl), Dis_Vmscl, Int_Vmscl, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmscl", JITFUNC(Comp_Vmscl), Dis_Vmscl, Int_Vmscl, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmscl", JITFUNC(Comp_Vmscl), Dis_Vmscl, Int_Vmscl, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INSTR("vcrsp.t/vqmul.q", JITFUNC(Comp_VCrossQuat), Dis_CrossQuat, Int_CrossQuat, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vcrsp.t/vqmul.q", JITFUNC(Comp_VCrossQuat), Dis_CrossQuat, Int_CrossQuat, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vcrsp.t/vqmul.q", JITFUNC(Comp_VCrossQuat), Dis_CrossQuat, Int_CrossQuat, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vcrsp.t/vqmul.q", JITFUNC(Comp_VCrossQuat), Dis_CrossQuat, Int_CrossQuat, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //24
    INVALID,
    INVALID,
    INVALID,
    INVALID,
    //28
    ENCODING(VFPUMatrix1),
    INSTR("vrot", JITFUNC(Comp_VRot), Dis_VRot, Int_Vrot, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INVALID,
};

// TODO: Should this only be when bit 20 is 0?
static const MIPSInstruction tableVFPUMatrixSet1[16] = // 111100 11100 .xxxx . ....... . .......  (rm x is 16)
{
    INSTR("vmmov", JITFUNC(Comp_Vmmov), Dis_MatrixSet2, Int_Vmmov, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    INVALID,
    INSTR("vmidt", JITFUNC(Comp_VMatrixInit), Dis_MatrixSet1, Int_VMatrixInit, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INVALID,
    INVALID,
    INSTR("vmzero", JITFUNC(Comp_VMatrixInit), Dis_MatrixSet1, Int_VMatrixInit, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vmone",  JITFUNC(Comp_VMatrixInit), Dis_MatrixSet1, Int_VMatrixInit, OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    INVALID_X_8,
};

static const MIPSInstruction tableVFPU9[32] = // 110100 00010 xxxxx . ....... . .......
{
    INSTR("vsrt1", JITFUNC(Comp_Generic), Dis_Vbfy, Int_Vsrt1, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsrt2", JITFUNC(Comp_Generic), Dis_Vbfy, Int_Vsrt2, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vbfy1", JITFUNC(Comp_Vbfy), Dis_Vbfy, Int_Vbfy, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vbfy2", JITFUNC(Comp_Vbfy), Dis_Vbfy, Int_Vbfy, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //4
    INSTR("vocp", JITFUNC(Comp_Vocp), Dis_Vbfy, Int_Vocp, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),  // one's complement
    INSTR("vsocp", JITFUNC(Comp_Generic), Dis_Vbfy, Int_Vsocp, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vfad", JITFUNC(Comp_Vhoriz), Dis_Vfad, Int_Vfad, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vavg", JITFUNC(Comp_Vhoriz), Dis_Vfad, Int_Vavg, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    //8
    INSTR("vsrt3", JITFUNC(Comp_Generic), Dis_Vbfy, Int_Vsrt3, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsrt4", JITFUNC(Comp_Generic), Dis_Vbfy, Int_Vsrt4, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vsgn", JITFUNC(Comp_Vsgn), Dis_Vbfy, Int_Vsgn, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INVALID,
    //12
    INVALID,
    INVALID,
    INVALID,
    INVALID,

    //16
    INSTR("vmfvc", JITFUNC(Comp_Vmfvc), Dis_Vmfvc, Int_Vmfvc, IN_OTHER|IN_VFPU_CC|OUT_OTHER|IS_VFPU),
    INSTR("vmtvc", JITFUNC(Comp_Vmtvc), Dis_Vmtvc, Int_Vmtvc, IN_OTHER|OUT_VFPU_CC|OUT_OTHER|IS_VFPU|OUT_VFPU_PREFIX),
    INVALID,
    INVALID,

    //20
    INVALID, INVALID, INVALID, INVALID,
    //24
    INVALID,
    INSTR("vt4444", JITFUNC(Comp_ColorConv), Dis_ColorConv, Int_ColorConv, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vt5551", JITFUNC(Comp_ColorConv), Dis_ColorConv, Int_ColorConv, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),
    INSTR("vt5650", JITFUNC(Comp_ColorConv), Dis_ColorConv, Int_ColorConv, IN_OTHER|OUT_OTHER|IS_VFPU|OUT_EAT_PREFIX),

    //28
    INVALID, INVALID, INVALID, INVALID,
};

static const MIPSInstruction tableCop2[32] = // 010010 xxxxx ..... ................
{
    INSTR("mfc2", JITFUNC(Comp_Generic), Dis_Generic, 0, OUT_RT),
    INVALID,
    INSTR("cfc2", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("mfv", JITFUNC(Comp_Mftv), Dis_Mftv, Int_Mftv, IN_OTHER|IN_VFPU_CC|OUT_RT|IS_VFPU),
    INSTR("mtc2", JITFUNC(Comp_Generic), Dis_Generic, 0, IN_RT),
    INVALID,
    INSTR("ctc2", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("mtv", JITFUNC(Comp_Mftv), Dis_Mftv, Int_Mftv, IN_RT|OUT_VFPU_CC|OUT_OTHER|IS_VFPU|OUT_VFPU_PREFIX),
    //8
    ENCODING(Cop2BC2),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    INSTR("??", JITFUNC(Comp_Generic), Dis_Generic, 0, 0),
    //16
    INVALID_X_8,
    INVALID_X_8,
};

static const MIPSInstruction tableCop2BC2[4] = // 010010 01000 ...xx ................
{
    INSTR("bvf", JITFUNC(Comp_VBranch), Dis_VBranch, Int_VBranch, IS_CONDBRANCH|IN_IMM16|IN_VFPU_CC|DELAYSLOT|IS_VFPU),
    INSTR("bvt", JITFUNC(Comp_VBranch), Dis_VBranch, Int_VBranch, IS_CONDBRANCH|IN_IMM16|IN_VFPU_CC|DELAYSLOT|IS_VFPU),
    INSTR("bvfl", JITFUNC(Comp_VBranch), Dis_VBranch, Int_VBranch, IS_CONDBRANCH|IN_IMM16|IN_VFPU_CC|DELAYSLOT|LIKELY|IS_VFPU),
    INSTR("bvtl", JITFUNC(Comp_VBranch), Dis_VBranch, Int_VBranch, IS_CONDBRANCH|IN_IMM16|IN_VFPU_CC|DELAYSLOT|LIKELY|IS_VFPU),
};


const float cst_constants[32] = {
    0,
    std::numeric_limits<float>::max(),  // all these are verified on real PSP
    sqrtf(2.0f),
    sqrtf(0.5f),
    2.0f/sqrtf((float)M_PI),
    2.0f/(float)M_PI,
    1.0f/(float)M_PI,
    (float)M_PI/4,
    (float)M_PI/2,
    (float)M_PI,
    (float)M_E,
    (float)M_LOG2E,
    (float)M_LOG10E,
    (float)M_LN2,
    (float)M_LN10,
    2*(float)M_PI,
    (float)M_PI/6,
    log10f(2.0f),
    logf(10.0f)/logf(2.0f),
    sqrtf(3.0f)/2.0f,
};

/*
EncodingBitsInfo(23, 3), //VFPU0
EncodingBitsInfo(23, 3), //VFPU1
EncodingBitsInfo(23, 3), //VFPU3
EncodingBitsInfo(21, 5), //VFPU4Jump
EncodingBitsInfo(16, 5), //VFPU7
EncodingBitsInfo(16, 5), //VFPU4
EncodingBitsInfo(23, 3), //VFPU5
EncodingBitsInfo(21, 5), //VFPU6
EncodingBitsInfo(16, 4), //VFPUMatrix1
EncodingBitsInfo(16, 5), //VFPU9
*/

void handleCop2(uint32_t op) {
    auto func = tableCop2[(op >> 21) & 0x1F];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleCop2BC2(uint32_t op) {
    auto func = tableCop2BC2[(op >> 16) & 3];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU0(uint32_t op) {
    auto func = tableVFPU0[(op >> 23) & 7];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU1(uint32_t op) {
    auto func = tableVFPU1[(op >> 23) & 7];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU3(uint32_t op) {
    auto func = tableVFPU3[(op >> 23) & 7];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU4Jump(uint32_t op) {
    auto func = tableVFPU4Jump[(op >> 21) & 0x1F];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU5(uint32_t op) {
    auto func = tableVFPU5[(op >> 23) & 7];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU6(uint32_t op) {
    auto func = tableVFPU6[(op >> 21) & 0x1F];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU4(uint32_t op) {
    auto func = tableVFPU4[(op >> 16) & 0x1F];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU7(uint32_t op) {
    auto func = tableVFPU7[(op >> 16) & 0x1F];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPU9(uint32_t op) {
    auto func = tableVFPU9[(op >> 16) & 0x1F];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void handleVFPUMatrix1(uint32_t op) {
    auto func = tableVFPUMatrixSet1[(op >> 16) & 0xF];
    if (func) {
        func(op);
    } else {
        LOG_ERROR(logType, "unimplemented %s 0x%08x!", __func__, op);
        Core::Allegrex::setProcessorFailed(true);
    }
}

void Int_VBranch(MIPSOpcode op)
{
    int imm = (signed short)(op&0xFFFF)<<2;
    u32 addr = PC + imm + 4;

    // x, y, z, w, any, all, (invalid), (invalid)
    int imm3 = (op>>18)&7;
    int val = (cpu.vfpuCtrl[VFPU_CTRL_CC] >> imm3) & 1;

    switch ((op >> 16) & 3)
    {
    case 0: if (!val) cpu.jump(addr); else PC += 4; break; //bvf
    case 1: if ( val) cpu.jump(addr); else PC += 4; break; //bvt
    case 2: if (!val) cpu.jump(addr); else PC += 8; break; //bvfl
    case 3: if ( val) cpu.jump(addr); else PC += 8; break; //bvtl
    }
}

void Int_VPFX(MIPSOpcode op)
{
    int data = op & 0x000FFFFF;
    int regnum = (op >> 24) & 3;
    if (regnum == VFPU_CTRL_DPREFIX)
        data &= 0x00000FFF;
    cpu.vfpuCtrl[VFPU_CTRL_SPREFIX + regnum] = data;
    PC += 4;
}

void Int_SVQ(MIPSOpcode op)
{
    int imm = SignExtend16ToS32(op & 0xFFFC);
    int rs = _RS;
    int vt = (((op >> 16) & 0x1f)) | ((op&1) << 5);

    u32 addr = R(rs) + imm;
    float *f;
    const float *cf;

    switch (op >> 26)
    {
    case 53: //lvl.q/lvr.q
    {
        if (addr & 0x3)
        {
        }
        float d[4];
        ReadVector(d, V_Quad, vt);
        int offset = (addr >> 2) & 3;
        if ((op & 2) == 0)
        {
            // It's an LVL
            for (int i = 0; i < offset + 1; i++)
            {
                d[3 - i] = Memory::Read_Float(addr - 4 * i);
            }
        }
        else
        {
            // It's an LVR
            for (int i = 0; i < (3 - offset) + 1; i++)
            {
                d[i] = Memory::Read_Float(addr + 4 * i);
            }
        }
        WriteVector(d, V_Quad, vt);
    }
    break;

    case 54: //lv.q
    if (addr & 0xF)
    {
    }
#ifndef COMMON_BIG_ENDIAN
    cf = reinterpret_cast<const float *>(Memory::GetPointerRange(addr, 16));
    if (cf)
        WriteVector(cf, V_Quad, vt);
#else
    float lvqd[4];

    lvqd[0] = Memory::Read_Float(addr);
    lvqd[1] = Memory::Read_Float(addr + 4);
    lvqd[2] = Memory::Read_Float(addr + 8);
    lvqd[3] = Memory::Read_Float(addr + 12);

    WriteVector(lvqd, V_Quad, vt);
#endif
    break;

    case 61: // svl.q/svr.q
    {
        if (addr & 0x3)
        {
        }
        float d[4];
        ReadVector(d, V_Quad, vt);
        int offset = (addr >> 2) & 3;
        if ((op&2) == 0)
        {
            // It's an SVL
            for (int i = 0; i < offset + 1; i++)
            {
                Memory::Write_Float(d[3 - i], addr - i * 4);
            }
        }
        else
        {
            // It's an SVR
            for (int i = 0; i < (3 - offset) + 1; i++)
            {
                Memory::Write_Float(d[i], addr + 4 * i);
            }
        }
        break;
    }

    case 62: //sv.q
    if (addr & 0xF)
    {
    }
#ifndef COMMON_BIG_ENDIAN
    f = reinterpret_cast<float *>(Memory::GetPointerWriteRange(addr, 16));
    if (f)
        ReadVector(f, V_Quad, vt);
#else
    float svqd[4];
    ReadVector(svqd, V_Quad, vt);

    Memory::Write_Float(svqd[0], addr);
    Memory::Write_Float(svqd[1], addr + 4);
    Memory::Write_Float(svqd[2], addr + 8);
    Memory::Write_Float(svqd[3], addr + 12);
#endif
    break;

    default:
    break;
    }
    PC += 4;
}

void Int_VMatrixInit(MIPSOpcode op) {
    static const float idt[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1,
    };
    static const float zero[16] = {
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
        0,0,0,0,
    };
    static const float one[16] = {
        1,1,1,1,
        1,1,1,1,
        1,1,1,1,
        1,1,1,1,
    };
    int vd = _VD;
    MatrixSize sz = GetMtxSize(op);
    const float *m;

    switch ((op >> 16) & 0xF) {
    case 3: m=idt; break; //identity   // vmidt
    case 6: m=zero; break;             // vmzero
    case 7: m=one; break;              // vmone
    default:
    PC += 4;
    EatPrefixes();
    return;
    }

    // The S prefix generates constants, but only for the final (possibly transposed) row.
    if (cpu.vfpuCtrl[VFPU_CTRL_SPREFIX] & 0xF0F00) {
        float prefixed[16];
        memcpy(prefixed, m, sizeof(prefixed));

        int off = GetMatrixSide(sz) - 1;
        u32 sprefixRemove = VFPU_ANY_SWIZZLE();
        u32 sprefixAdd = 0;
        switch ((op >> 16) & 0xF) {
        case 3:
        {
            VFPUConst constX = off == 0 ? VFPUConst::ONE : VFPUConst::ZERO;
            VFPUConst constY = off == 1 ? VFPUConst::ONE : VFPUConst::ZERO;
            VFPUConst constZ = off == 2 ? VFPUConst::ONE : VFPUConst::ZERO;
            VFPUConst constW = off == 3 ? VFPUConst::ONE : VFPUConst::ZERO;
            sprefixAdd = VFPU_MAKE_CONSTANTS(constX, constY, constZ, constW);
            break;
        }
        case 6:
        sprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ZERO, VFPUConst::ZERO, VFPUConst::ZERO, VFPUConst::ZERO);
        break;
        case 7:
        sprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ONE, VFPUConst::ONE, VFPUConst::ONE, VFPUConst::ONE);
        break;
        default:
        break;
        }
        ApplyPrefixST(&prefixed[off * 4], VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), V_Quad);
        WriteMatrix(prefixed, sz, vd);
    } else {
        // Write mask applies to the final (maybe transposed) row.  Sat causes hang.
        WriteMatrix(m, sz, vd);
    }
    PC += 4;
    EatPrefixes();
}

void Int_VVectorInit(MIPSOpcode op)
{
    int vd = _VD;
    VectorSize sz = GetVecSize(op);
    float d[4];

    VFPUConst constant = VFPUConst::ZERO;
    switch ((op >> 16) & 0xF) {
    case 6: constant = VFPUConst::ZERO; break;  //vzero
    case 7: constant = VFPUConst::ONE; break;   //vone
    default:
    PC += 4;
    EatPrefixes();
    return;
    }

    // The S prefix generates constants, but negate is still respected.
    u32 sprefixRemove = VFPU_ANY_SWIZZLE();
    u32 sprefixAdd = VFPU_MAKE_CONSTANTS(constant, constant, constant, constant);
    ApplyPrefixST(d, VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), sz);

    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);

    EatPrefixes();
    PC += 4;
}

void Int_Viim(MIPSOpcode op) {
    int vt = _VT;
    s32 imm = SignExtend16ToS32(op & 0xFFFF);
    u16 uimm16 = (op&0xFFFF);
    float f[1];
    int type = (op >> 23) & 7;
    if (type == 6) {
        f[0] = (float)imm;  // viim
    } else if (type == 7) {
        f[0] = Float16ToFloat32((u16)uimm16);   // vfim
    } else {
        f[0] = 0;
    }

    ApplyPrefixD(f, V_Single);
    WriteVector(f, V_Single, vt);
    PC += 4;
    EatPrefixes();
}

void Int_Vidt(MIPSOpcode op) {
    int vd = _VD;
    VectorSize sz = GetVecSize(op);
    float f[4];

    // The S prefix generates constants, but negate is still respected.
    int offmask = sz == V_Quad || sz == V_Triple ? 3 : 1;
    int off = vd & offmask;
    // If it's a pair, the identity starts in a different position.
    VFPUConst constX = off == (0 & offmask) ? VFPUConst::ONE : VFPUConst::ZERO;
    VFPUConst constY = off == (1 & offmask) ? VFPUConst::ONE : VFPUConst::ZERO;
    VFPUConst constZ = off == (2 & offmask) ? VFPUConst::ONE : VFPUConst::ZERO;
    VFPUConst constW = off == (3 & offmask) ? VFPUConst::ONE : VFPUConst::ZERO;

    u32 sprefixRemove = VFPU_ANY_SWIZZLE();
    u32 sprefixAdd = VFPU_MAKE_CONSTANTS(constX, constY, constZ, constW);
    ApplyPrefixST(f, VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), sz);

    ApplyPrefixD(f, sz);
    WriteVector(f, sz, vd);
    PC += 4;
    EatPrefixes();
}

// The test really needs some work.
void Int_Vmmul(MIPSOpcode op) {
    float s[16]{}, t[16]{}, d[16];

    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    MatrixSize sz = GetMtxSize(op);
    int n = GetMatrixSide(sz);

    ReadMatrix(s, sz, vs);
    ReadMatrix(t, sz, vt);

    for (int a = 0; a < n; a++) {
        for (int b = 0; b < n; b++) {
            union { float f; uint32_t u; } sum = { 0.0f };
            if (a == n - 1 && b == n - 1) {
                // S and T prefixes work on the final (or maybe first, in reverse?) dot.
                ApplySwizzleS(&s[b * 4], V_Quad);
                ApplySwizzleT(&t[a * 4], V_Quad);
            }

            if (a == n - 1 && b == n - 1) {
                for (int c = 0; c < 4; c++) {
                    sum.f += s[b * 4 + c] * t[a * 4 + c];
                }
            } else {
                for (int c = 0; c < n; c++) {
                    sum.f += s[b * 4 + c] * t[a * 4 + c];
                }
            }

            d[a * 4 + b] = sum.f;
        }
    }

    // The D prefix applies ONLY to the final element, but sat does work.
    u32 lastmask = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & (1 << 8)) << (n - 1);
    u32 lastsat = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & 3) << (n + n - 2);
    cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = lastmask | lastsat;
    ApplyPrefixD(&d[4 * (n - 1)], V_Quad, false);
    WriteMatrix(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vmscl(MIPSOpcode op) {
    float s[16]{}, t[4]{}, d[16];

    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    MatrixSize sz = GetMtxSize(op);
    int n = GetMatrixSide(sz);

    ReadMatrix(s, sz, vs);
    ReadVector(t, V_Single, vt);

    for (int a = 0; a < n - 1; a++) {
        for (int b = 0; b < n; b++) {
            d[a * 4 + b] = s[a * 4 + b] * t[0];
        }
    }

    // S prefix applies to the last row.
    ApplySwizzleS(&s[(n - 1) * 4], V_Quad);
    // T prefix applies only for the last row, and is used per element.
    // This is like vscl, but instead of zzzz it uses xxxx.
    int tlane = (vt >> 5) & 3;
    t[tlane] = t[0];
    u32 tprefixRemove = VFPU_ANY_SWIZZLE();
    u32 tprefixAdd = VFPU_SWIZZLE(tlane, tlane, tlane, tlane);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);

    for (int b = 0; b < n; b++) {
        d[(n - 1) * 4 + b] = s[(n - 1) * 4 + b] * t[b];
    }

    // The D prefix is applied to the last row.
    ApplyPrefixD(&d[(n - 1) * 4], V_Quad);
    WriteMatrix(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vmmov(MIPSOpcode op) {
    float s[16]{};
    int vd = _VD;
    int vs = _VS;
    MatrixSize sz = GetMtxSize(op);
    ReadMatrix(s, sz, vs);
    // S and D prefixes are applied to the last row.
    int off = GetMatrixSide(sz) - 1;
    ApplySwizzleS(&s[off * 4], V_Quad);
    ApplyPrefixD(&s[off * 4], V_Quad);
    WriteMatrix(s, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vflush(MIPSOpcode op)
{
    PC += 4;
    // Anything with 0xFC000000 is a nop, but only 0xFFFF0000 retains prefixes.
    if ((op & 0xFFFF0000) != 0xFFFF0000)
        EatPrefixes();
}

void Int_VV2Op(MIPSOpcode op) {
    float s[4], d[4];
    int vd = _VD;
    int vs = _VS;
    int optype = (op >> 16) & 0x1f;
    VectorSize sz = GetVecSize(op);
    u32 n = GetNumVectorElements(sz);
    ReadVector(s, sz, vs);
    // Some of these are prefix hacks (affects constants, etc.)
    switch (optype) {
    case 1:
    ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, 0, VFPU_ABS(1, 1, 1, 1)), sz);
    break;
    case 2:
    ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, 0, VFPU_NEGATE(1, 1, 1, 1)), sz);
    break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    // Similar to vdiv.  Some of the behavior using the invalid constant is iffy.
    ApplySwizzleS(&s[n - 1], V_Single, INFINITY);
    break;
    case 24:
    case 26:
    // Similar to above, but also ignores negate.
    ApplyPrefixST(&s[n - 1], VFPURewritePrefix(VFPU_CTRL_SPREFIX, VFPU_NEGATE(1, 0, 0, 0), 0), V_Single, -INFINITY);
    break;
    case 28:
    // Similar to above, but also ignores negate.
    ApplyPrefixST(&s[n - 1], VFPURewritePrefix(VFPU_CTRL_SPREFIX, VFPU_NEGATE(1, 0, 0, 0), 0), V_Single, INFINITY);
    break;
    default:
    ApplySwizzleS(s, sz);
    break;
    }
    for (int i = 0; i < (int)n; i++) {
        switch (optype) {
        case 0: d[i] = s[i]; break; //vmov
        case 1: d[i] = s[i]; break; //vabs (prefix)
        case 2: d[i] = s[i]; break; //vneg (prefix)
            // vsat0 changes -0.0 to +0.0, both retain NAN.
        case 4: if (s[i] <= 0) d[i] = 0; else {if(s[i] > 1.0f) d[i] = 1.0f; else d[i] = s[i];} break;    // vsat0
        case 5: if (s[i] < -1.0f) d[i] = -1.0f; else {if(s[i] > 1.0f) d[i] = 1.0f; else d[i] = s[i];} break;  // vsat1
        case 16: { d[i] = vfpu_rcp(s[i]); } break; //vrcp
        case 17: d[i] = 1.0f / std::sqrtf(s[i]); break; //vrsq

        case 18: { d[i] = std::sin(s[i]); } break; //vsin
        case 19: { d[i] = std::cos(s[i]); } break; //vcos
        case 20: { d[i] = std::exp2(s[i]); } break; //vexp2
        case 21: { d[i] = std::log2(s[i]); } break; //vlog2
        case 22: d[i] = fabsf(sqrtf(s[i])); break; //vsqrt
        case 23: { d[i] = std::asin(s[i]); } break; //vasin
        case 24: { d[i] = -vfpu_rcp(s[i]); } break; // vnrcp
        case 26: { d[i] = -std::sin(s[i]); } break; // vnsin
        case 28: { d[i] = vfpu_rexp2(s[i]); } break; // vrexp2
        default:
        break;
        }
    }
    // vsat1 is a prefix hack, so 0:1 doesn't apply.  Others don't process sat at all.
    switch (optype) {
    case 5:
    ApplyPrefixD(d, sz, true);
    break;
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 26:
    case 28:
    {
        // Only the last element gets the mask applied.
        u32 lastmask = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & (1 << 8)) << (n - 1);
        u32 lastsat = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & 3) << (n + n - 2);
        cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = lastmask | lastsat;
        ApplyPrefixD(d, sz);
        break;
    }
    default:
    ApplyPrefixD(d, sz);
    }
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vocp(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);

    // S prefix forces the negate flags.
    u32 sprefixAdd = VFPU_NEGATE(1, 1, 1, 1);
    ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, 0, sprefixAdd), sz);

    // T prefix forces constants on and regnum to 1.
    // That means negate still works, and abs activates a different constant.
    u32 tprefixRemove = VFPU_ANY_SWIZZLE();
    u32 tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ONE, VFPUConst::ONE, VFPUConst::ONE, VFPUConst::ONE);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    for (int i = 0; i < GetNumVectorElements(sz); i++) {
        // Always positive NaN.  Note that s is always negated from the registers.
        d[i] = my_isnan(s[i]) ? fabsf(s[i]) : t[i] + s[i];
    }
    RetainInvalidSwizzleST(d, sz);
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsocp(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    VectorSize outSize = GetDoubleVectorSizeSafe(sz);
    if (outSize == V_Invalid)
        outSize = V_Quad;
    ReadVector(s, sz, vs);

    // S prefix forces negate in even/odd and xxyy swizzle.
    // abs works, and applies to final position (not source.)
    u32 sprefixRemove = VFPU_ANY_SWIZZLE() | VFPU_NEGATE(1, 1, 1, 1);
    u32 sprefixAdd = VFPU_SWIZZLE(0, 0, 1, 1) | VFPU_NEGATE(1, 0, 1, 0);
    ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), outSize);

    // T prefix forces constants on and regnum to 1, 0, 1, 0.
    // That means negate still works, and abs activates a different constant.
    u32 tprefixRemove = VFPU_ANY_SWIZZLE();
    u32 tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ONE, VFPUConst::ZERO, VFPUConst::ONE, VFPUConst::ZERO);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), outSize);

    // Essentially D prefix saturation is forced.
    d[0] = nanclamp(t[0] + s[0], 0.0f, 1.0f);
    d[1] = nanclamp(t[1] + s[1], 0.0f, 1.0f);
    if (outSize == V_Quad) {
        d[2] = nanclamp(t[2] + s[2], 0.0f, 1.0f);
        d[3] = nanclamp(t[3] + s[3], 0.0f, 1.0f);
    }
    ApplyPrefixD(d, sz, true);
    WriteVector(d, outSize, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsgn(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);

    // Not sure who would do this, but using abs/neg allows a compare against 3 or -3.
    u32 tprefixRemove = VFPU_ANY_SWIZZLE();
    u32 tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ZERO, VFPUConst::ZERO, VFPUConst::ZERO, VFPUConst::ZERO);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    int n = GetNumVectorElements(sz);
    if (n < 4) {
        // Compare with a swizzled value out of bounds always produces 0.
        memcpy(&s[n], &t[n], sizeof(float) * (4 - n));
    }
    ApplySwizzleS(s, V_Quad);

    for (int i = 0; i < n; i++) {
        float diff = s[i] - t[i];
        // To handle NaNs correctly, we do this with integer hackery
        u32 val;
        memcpy(&val, &diff, sizeof(u32));
        if (val == 0 || val == 0x80000000)
            d[i] = 0.0f;
        else if ((val >> 31) == 0)
            d[i] = 1.0f;
        else
            d[i] = -1.0f;
    }
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

inline int round_vfpu_n(double param) {
    // return floorf(param);
    return (int)round_ieee_754(param);
}

void Int_Vf2i(MIPSOpcode op) {
    float s[4];
    int d[4];
    int vd = _VD;
    int vs = _VS;
    int imm = (op >> 16) & 0x1f;
    float mult = (float)(1UL << imm);
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    // Negate, abs, and constants apply as you'd expect to the bits.
    ApplySwizzleS(s, sz);
    for (int i = 0; i < GetNumVectorElements(sz); i++) {
        if (my_isnan(s[i])) {
            d[i] = 0x7FFFFFFF;
            continue;
        }
        double sv = s[i] * mult; // (float)0x7fffffff == (float)0x80000000
        // Cap/floor it to 0x7fffffff / 0x80000000
        if (sv > (double)0x7fffffff) {
            d[i] = 0x7fffffff;
        } else if (sv <= (double)(int)0x80000000) {
            d[i] = 0x80000000;
        } else {
            switch ((op >> 21) & 0x1f)
            {
            case 16: d[i] = (int)round_ieee_754(sv); break; //(floor(sv + 0.5f)); break; //n
            case 17: d[i] = s[i]>=0 ? (int)floor(sv) : (int)ceil(sv); break; //z
            case 18: d[i] = (int)std::ceil(sv); break; //u
            case 19: d[i] = (int)std::floor(sv); break; //d
            default: d[i] = 0x7FFFFFFF; break;
            }
        }
    }
    // Does not apply sat, but does apply mask.
    ApplyPrefixD(reinterpret_cast<float *>(d), sz, true);
    WriteVector(reinterpret_cast<float *>(d), sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vi2f(MIPSOpcode op) {
    int s[4];
    float d[4];
    int vd = _VD;
    int vs = _VS;
    int imm = (op >> 16) & 0x1f;
    float mult = 1.0f/(float)(1UL << imm);
    VectorSize sz = GetVecSize(op);
    ReadVector(reinterpret_cast<float *>(s), sz, vs);
    // Negate, abs, and constants apply as you'd expect to the bits.
    ApplySwizzleS(reinterpret_cast<float *>(s), sz);
    for (int i = 0; i < GetNumVectorElements(sz); i++) {
        d[i] = (float)s[i] * mult;
    }
    // Sat and mask apply normally.
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vh2f(MIPSOpcode op) {
    u32 s[4];
    float d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(reinterpret_cast<float *>(s), sz, vs);
    ApplySwizzleS(reinterpret_cast<float *>(s), sz);

    VectorSize outsize = V_Pair;
    switch (sz) {
    case V_Single:
    outsize = V_Pair;
    d[0] = ExpandHalf(s[0] & 0xFFFF);
    d[1] = ExpandHalf(s[0] >> 16);
    break;
    case V_Pair:
    default:
    // All other sizes are treated the same.
    outsize = V_Quad;
    d[0] = ExpandHalf(s[0] & 0xFFFF);
    d[1] = ExpandHalf(s[0] >> 16);
    d[2] = ExpandHalf(s[1] & 0xFFFF);
    d[3] = ExpandHalf(s[1] >> 16);
    break;
    }
    ApplyPrefixD(d, outsize);
    WriteVector(d, outsize, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vf2h(MIPSOpcode op) {
    float s[4]{};
    u32 d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    // Swizzle can cause V_Single to properly write both components.
    ApplySwizzleS(s, V_Quad);
    // Negate should not actually apply to invalid swizzle.
    RetainInvalidSwizzleST(s, V_Quad);

    VectorSize outsize = V_Single;
    switch (sz) {
    case V_Single:
    case V_Pair:
    outsize = V_Single;
    d[0] = ShrinkToHalf(s[0]) | ((u32)ShrinkToHalf(s[1]) << 16);
    break;
    case V_Triple:
    case V_Quad:
    outsize = V_Pair;
    d[0] = ShrinkToHalf(s[0]) | ((u32)ShrinkToHalf(s[1]) << 16);
    d[1] = ShrinkToHalf(s[2]) | ((u32)ShrinkToHalf(s[3]) << 16);
    break;

    default:

    break;
    }
    ApplyPrefixD(reinterpret_cast<float *>(d), outsize);
    WriteVector(reinterpret_cast<float *>(d), outsize, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vx2i(MIPSOpcode op) {
    u32 s[4], d[4]{};
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    VectorSize oz = sz;
    ReadVector(reinterpret_cast<float *>(s), sz, vs);
    ApplySwizzleS(reinterpret_cast<float *>(s), sz);

    // TODO: Similar to colorconv, invalid swizzle seems to reuse last output.
    switch ((op >> 16) & 3) {
    case 0:  // vuc2i  
    // Quad is the only option.
    // This converts 8-bit unsigned to 31-bit signed, swizzling to saturate.
    // Similar to 5-bit to 8-bit color swizzling, but clamping to INT_MAX.
    {
        u32 value = s[0];
        for (int i = 0; i < 4; i++) {
            d[i] = (u32)((u32)(value & 0xFF) * 0x01010101UL) >> 1;
            value >>= 8;
        }
        oz = V_Quad;
    }
    break;

    case 1:  // vc2i
    // Quad is the only option
    // Unlike vuc2i, the source and destination are signed so there is no shift.
    // It lacks the swizzle because of negative values.
    {
        u32 value = s[0];
        d[0] = (value & 0xFF) << 24;
        d[1] = (value & 0xFF00) << 16;
        d[2] = (value & 0xFF0000) << 8;
        d[3] = (value & 0xFF000000);
        oz = V_Quad;
    }
    break;

    case 2:  // vus2i
    // Note: for some reason, this skips swizzle such that 0xFFFF -> 0x7FFF8000 unlike vuc2i.
    oz = V_Pair;
    switch (sz) {
    case V_Quad:
    case V_Triple:
    sz = V_Pair;
    // Intentional fallthrough.
    case V_Pair:
    oz = V_Quad;
    // Intentional fallthrough.
    case V_Single:
    for (int i = 0; i < GetNumVectorElements(sz); i++) {
        u32 value = s[i];
        d[i * 2] = (value & 0xFFFF) << 15;
        d[i * 2 + 1] = (value & 0xFFFF0000) >> 1;
    }
    break;

    default:
    break;
    }
    break;

    case 3:  // vs2i
    oz = V_Pair;
    switch (sz) {
    case V_Quad:
    case V_Triple:
    sz = V_Pair;
    // Intentional fallthrough.
    case V_Pair:
    oz = V_Quad;
    // Intentional fallthrough.
    case V_Single:
    for (int i = 0; i < GetNumVectorElements(sz); i++) {
        u32 value = s[i];
        d[i * 2] = (value & 0xFFFF) << 16;
        d[i * 2 + 1] = value & 0xFFFF0000;
    }
    break;

    default:
    break;
    }
    break;

    default:
    break;
    }

    // Saturation does in fact apply.
    ApplyPrefixD(reinterpret_cast<float *>(d),oz);
    WriteVector(reinterpret_cast<float *>(d), oz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vi2x(MIPSOpcode op) {
    int s[4]{};
    u32 d[2]{};
    const int vd = _VD;
    const int vs = _VS;
    const VectorSize sz = GetVecSize(op);
    VectorSize oz;
    ReadVector(reinterpret_cast<float *>(s), sz, vs);
    // Negate, const, etc. apply as expected.
    ApplySwizzleS(reinterpret_cast<float *>(s), V_Quad);

    // TODO: Similar to colorconv, invalid swizzle seems to reuse last output.
    switch ((op >> 16) & 3) {
    case 0: //vi2uc
    for (int i = 0; i < 4; i++) {
        int v = s[i];
        if (v < 0) v = 0;
        v >>= 23;
        d[0] |= ((u32)v & 0xFF) << (i * 8);
    }
    oz = V_Single;
    break;

    case 1: //vi2c
    for (int i = 0; i < 4; i++) {
        u32 v = s[i];
        d[0] |= (v >> 24) << (i * 8);
    }
    oz = V_Single;
    break;

    case 2:  //vi2us
    {
        int elems = (GetNumVectorElements(sz) + 1) / 2;
        for (int i = 0; i < elems; i++) {
            int low = s[i * 2];
            int high = s[i * 2 + 1];
            if (low < 0) low = 0;
            if (high < 0) high = 0;
            low >>= 15;
            high >>= 15;
            d[i] = low | (high << 16);
        }
        switch (sz) {
        case V_Quad: oz = V_Pair; break;
        case V_Triple: oz = V_Pair; break;
        case V_Pair: oz = V_Single; break;
        case V_Single: oz = V_Single; break;
        default:
        oz = V_Single;
        break;
        }
        break;
    }
    case 3:  //vi2s
    {
        int elems = (GetNumVectorElements(sz) + 1) / 2;
        for (int i = 0; i < elems; i++) {
            u32 low = s[i * 2];
            u32 high = s[i * 2 + 1];
            low >>= 16;
            high >>= 16;
            d[i] = low | (high << 16);
        }
        switch (sz) {
        case V_Quad: oz = V_Pair; break;
        case V_Triple: oz = V_Pair; break;
        case V_Pair: oz = V_Single; break;
        case V_Single: oz = V_Single; break;
        default:
        oz = V_Single;
        break;
        }
        break;
    }
    default:
    oz = V_Single;
    break;
    }
    // D prefix applies as expected.
    ApplyPrefixD(reinterpret_cast<float *>(d), oz);
    WriteVector(reinterpret_cast<float *>(d), oz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_ColorConv(MIPSOpcode op)
{
    int vd = _VD;
    int vs = _VS;
    u32 s[4];
    VectorSize isz = GetVecSize(op);
    VectorSize sz = V_Quad;
    ReadVector(reinterpret_cast<float *>(s), sz, vs);
    ApplySwizzleS(reinterpret_cast<float *>(s), sz);
    u16 colors[4];
    // TODO: Invalid swizzle values almost seem to use the last value converted in a
    // previous execution of these ops.  It's a bit odd.
    for (int i = 0; i < 4; i++)
    {
        u32 in = s[i];
        u16 col = 0;
        switch ((op >> 16) & 3)
        {
        case 1:  // 4444
        {
            int a = ((in >> 24) & 0xFF) >> 4;
            int b = ((in >> 16) & 0xFF) >> 4;
            int g = ((in >> 8) & 0xFF) >> 4;
            int r = ((in) & 0xFF) >> 4;
            col = (a << 12) | (b << 8) | (g << 4) | (r);
            break;
        }
        case 2:  // 5551
        {
            int a = ((in >> 24) & 0xFF) >> 7;
            int b = ((in >> 16) & 0xFF) >> 3;
            int g = ((in >> 8) & 0xFF) >> 3;
            int r = ((in) & 0xFF) >> 3;
            col = (a << 15) | (b << 10) | (g << 5) | (r);
            break;
        }
        case 3:  // 565
        {
            int b = ((in >> 16) & 0xFF) >> 3;
            int g = ((in >> 8) & 0xFF) >> 2;
            int r = ((in) & 0xFF) >> 3;
            col = (b << 11) | (g << 5) | (r); 
            break;
        }
        }
        colors[i] = col;
    }
    u32 ov[2] = {(u32)colors[0] | (colors[1] << 16), (u32)colors[2] | (colors[3] << 16)};
    ApplyPrefixD(reinterpret_cast<float *>(ov), V_Pair);
    WriteVector((const float *)ov, isz == V_Single ? V_Single : V_Pair, vd);
    PC += 4;
    EatPrefixes();
}

void Int_VDot(MIPSOpcode op) {
    float s[4]{}, t[4]{};
    union { float f; uint32_t u; } d;
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, V_Quad);
    ReadVector(t, sz, vt);
    ApplySwizzleT(t, V_Quad);

    d.f = 0.0f;
    for (int i = 0; i < 4; i++) {
        d.f += s[i] * t[i];
    }

    ApplyPrefixD(&d.f, V_Single);
    WriteVector(&d.f, V_Single, vd);
    PC += 4;
    EatPrefixes();
}

void Int_VHdp(MIPSOpcode op) {
    float s[4]{}, t[4]{};
    float d;
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ReadVector(t, sz, vt);
    ApplySwizzleT(t, V_Quad);

    // S prefix forces constant 1 for the last element (w for quad.)
    // Otherwise it is the same as vdot.
    u32 sprefixRemove;
    u32 sprefixAdd;
    if (sz == V_Quad) {
        sprefixRemove = VFPU_SWIZZLE(0, 0, 0, 3);
        sprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::NONE, VFPUConst::NONE, VFPUConst::NONE, VFPUConst::ONE);
    } else if (sz == V_Triple) {
        sprefixRemove = VFPU_SWIZZLE(0, 0, 3, 0);
        sprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::NONE, VFPUConst::NONE, VFPUConst::ONE, VFPUConst::NONE);
    } else if (sz == V_Pair) {
        sprefixRemove = VFPU_SWIZZLE(0, 3, 0, 0);
        sprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::NONE, VFPUConst::ONE, VFPUConst::NONE, VFPUConst::NONE);
    } else {
        sprefixRemove = VFPU_SWIZZLE(3, 0, 0, 0);
        sprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ONE, VFPUConst::NONE, VFPUConst::NONE, VFPUConst::NONE);
    }
    ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), V_Quad);

    float sum = 0.0f;
    for (int i = 0; i < 4; i++) {
        sum += s[i] * t[i];
    }

    d = my_isnan(sum) ? std::fabsf(sum) : sum;
    ApplyPrefixD(&d, V_Single);
    WriteVector(&d, V_Single, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vbfy(MIPSOpcode op) {
    float s[4]{}, t[4]{}, d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ReadVector(t, sz, vs);

    if (op & 0x10000) {
        // vbfy2
        // S prefix forces the negate flags (so z and w are negative.)
        u32 sprefixAdd = VFPU_NEGATE(0, 0, 1, 1);
        ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, 0, sprefixAdd), sz);

        // T prefix forces swizzle (zwxy.)
        // That means negate still works, but constants are a bit weird.
        u32 tprefixRemove = VFPU_ANY_SWIZZLE();
        u32 tprefixAdd = VFPU_SWIZZLE(2, 3, 0, 1);
        ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

        // Other sizes don't seem completely predictable.
        if (sz != V_Quad) {
        }
    } else {
        // vbfy1
        // S prefix forces the negate flags (so y and w are negative.)
        u32 sprefixAdd = VFPU_NEGATE(0, 1, 0, 1);
        ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, 0, sprefixAdd), sz);

        // T prefix forces swizzle (yxwz.)
        // That means negate still works, but constants are a bit weird.
        u32 tprefixRemove = VFPU_ANY_SWIZZLE();
        u32 tprefixAdd = VFPU_SWIZZLE(1, 0, 3, 2);
        ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

        if (sz != V_Quad && sz != V_Pair) {
        }
    }

    d[0] = s[0] + t[0];
    d[1] = s[1] + t[1];
    d[2] = s[2] + t[2];
    d[3] = s[3] + t[3];

    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsrt1(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vs);

    // T is force swizzled to yxwz from S.
    u32 tprefixRemove = VFPU_SWIZZLE(3, 3, 3, 3);
    u32 tprefixAdd = VFPU_SWIZZLE(1, 0, 3, 2);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    // TODO: May mishandle NAN / negative zero / etc.
    d[0] = std::min(s[0], t[0]);
    d[1] = std::max(s[1], t[1]);
    d[2] = std::min(s[2], t[2]);
    d[3] = std::max(s[3], t[3]);
    RetainInvalidSwizzleST(d, sz);
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsrt2(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vs);

    // T is force swizzled to wzyx from S.
    u32 tprefixRemove = VFPU_SWIZZLE(3, 3, 3, 3);
    u32 tprefixAdd = VFPU_SWIZZLE(3, 2, 1, 0);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    // TODO: May mishandle NAN / negative zero / etc.
    d[0] = std::min(s[0], t[0]);
    d[1] = std::min(s[1], t[1]);
    d[2] = std::max(s[2], t[2]);
    d[3] = std::max(s[3], t[3]);
    RetainInvalidSwizzleST(d, sz);
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsrt3(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vs);

    // T is force swizzled to yxwz from S.
    u32 tprefixRemove = VFPU_SWIZZLE(3, 3, 3, 3);
    u32 tprefixAdd = VFPU_SWIZZLE(1, 0, 3, 2);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    // TODO: May mishandle NAN / negative zero / etc.
    d[0] = std::max(s[0], t[0]);
    d[1] = std::min(s[1], t[1]);
    d[2] = std::max(s[2], t[2]);
    d[3] = std::min(s[3], t[3]);
    RetainInvalidSwizzleST(d, sz);
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsrt4(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vs);

    // T is force swizzled to wzyx from S.
    u32 tprefixRemove = VFPU_SWIZZLE(3, 3, 3, 3);
    u32 tprefixAdd = VFPU_SWIZZLE(3, 2, 1, 0);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    // TODO: May mishandle NAN / negative zero / etc.
    d[0] = std::max(s[0], t[0]);
    d[1] = std::max(s[1], t[1]);
    d[2] = std::min(s[2], t[2]);
    d[3] = std::min(s[3], t[3]);
    RetainInvalidSwizzleST(d, sz);
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vcrs(MIPSOpcode op) {
    //half a cross product
    float s[4]{}, t[4]{}, d[4];
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ReadVector(t, sz, vt);

    // S prefix forces swizzle (yzx?.)
    // That means negate still works, but constants are a bit weird.
    u32 sprefixRemove = VFPU_SWIZZLE(3, 3, 3, 0);
    u32 sprefixAdd = VFPU_SWIZZLE(1, 2, 0, 0);
    ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), sz);

    // T prefix forces swizzle (zxy?.)
    u32 tprefixRemove = VFPU_SWIZZLE(3, 3, 3, 0);
    u32 tprefixAdd = VFPU_SWIZZLE(2, 0, 1, 0);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), sz);

    d[0] = s[0] * t[0];
    d[1] = s[1] * t[1];
    d[2] = s[2] * t[2];
    d[3] = s[3] * t[3];
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vdet(MIPSOpcode op) {
    float s[4]{}, t[4]{}, d[4];
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);
    // This is normally V_Pair.  Unfilled s/t values are treated as zero.
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, V_Quad);
    ReadVector(t, sz, vt);

    // T prefix forces swizzle for x and y (yx??.)
    // That means negate still works, but constants are a bit weird.
    // Note: there is no forced negation here.
    u32 tprefixRemove = VFPU_SWIZZLE(3, 3, 0, 0);
    u32 tprefixAdd = VFPU_SWIZZLE(1, 0, 0, 0);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);

    d[0] = s[0] * t[0] - s[1] * t[1];
    d[0] += s[2] * t[2] + s[3] * t[3];

    ApplyPrefixD(d, V_Single);
    WriteVector(d, V_Single, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vfad(MIPSOpcode op) {
    float s[4]{}, t[4]{};
    float d;
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, V_Quad);

    // T prefix generates constants, but abs can change the constant.
    u32 tprefixRemove = VFPU_ANY_SWIZZLE();
    u32 tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ONE, VFPUConst::ONE, VFPUConst::ONE, VFPUConst::ONE);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);

    d = 0.0f;
    for (int i = 0; i < 4; i++) {
        d += s[i] * t[i];
    }
    ApplyPrefixD(&d, V_Single);
    WriteVector(&d, V_Single, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vavg(MIPSOpcode op) {
    float s[4]{}, t[4]{};
    float d;
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, V_Quad);

    // T prefix generates constants, but supports negate.
    u32 tprefixRemove = VFPU_ANY_SWIZZLE() | VFPU_ABS(1, 1, 1, 1);
    u32 tprefixAdd;
    if (sz == V_Single)
        tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::ZERO, VFPUConst::ZERO, VFPUConst::ZERO, VFPUConst::ZERO);
    else if (sz == V_Pair)
        tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::HALF, VFPUConst::HALF, VFPUConst::HALF, VFPUConst::HALF);
    else if (sz == V_Triple)
        tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::THIRD, VFPUConst::THIRD, VFPUConst::THIRD, VFPUConst::THIRD);
    else if (sz == V_Quad)
        tprefixAdd = VFPU_MAKE_CONSTANTS(VFPUConst::FOURTH, VFPUConst::FOURTH, VFPUConst::FOURTH, VFPUConst::FOURTH);
    else
        tprefixAdd = 0;
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);

    d = 0.0f;
    for (int i = 0; i < 4; i++) {
        d += s[i] * t[i];
    }

    ApplyPrefixD(&d, V_Single);
    WriteVector(&d, V_Single, vd);
    PC += 4;
    EatPrefixes();
}

void Int_VScl(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);

    // T prefix forces swizzle (zzzz for some reason, so we force V_Quad.)
    // That means negate still works, but constants are a bit weird.
    int tlane = (vt >> 5) & 3;
    t[tlane] = V(vt);
    u32 tprefixRemove = VFPU_ANY_SWIZZLE();
    u32 tprefixAdd = VFPU_SWIZZLE(tlane, tlane, tlane, tlane);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);

    int n = GetNumVectorElements(sz);
    for (int i = 0; i < n; i++) {
        d[i] = s[i] * t[i];
    }
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vrnds(MIPSOpcode op) {
    int vd = _VD;
    int seed = VI(vd);
    // Swizzles apply a constant value, constants/abs/neg work to vary the seed.
    ApplySwizzleS(reinterpret_cast<float *>(&seed), V_Single);
    vrnd_init(uint32_t(seed), cpu.vfpuCtrl + VFPU_CTRL_RCX0);
    PC += 4;
    EatPrefixes();
}

void Int_VrndX(MIPSOpcode op) {
    FloatBits d;
    int vd = _VD;
    VectorSize sz = GetVecSize(op);
    u32 n = GetNumVectorElements(sz);
    // Values are written in backwards order.
    for (int i = n - 1; i >= 0; i--) {
        switch ((op >> 16) & 0x1f) {
        case 1: d.u[i] = vrnd_generate(cpu.vfpuCtrl + VFPU_CTRL_RCX0); break;  // vrndi
        case 2: d.u[i] = 0x3F800000 | (vrnd_generate(cpu.vfpuCtrl + VFPU_CTRL_RCX0) & 0x007FFFFF); break; // vrndf1 (>= 1, < 2)
        case 3: d.u[i] = 0x40000000 | (vrnd_generate(cpu.vfpuCtrl + VFPU_CTRL_RCX0) & 0x007FFFFF); break; // vrndf2 (>= 2, < 4)
        default: ;
        }
    }
    // D prefix is broken and applies to the last element only (mask and sat.)
    u32 lastmask = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & (1 << 8)) << (n - 1);
    u32 lastsat = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & 3) << (n + n - 2);
    cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = lastmask | lastsat;
    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

// Generates one line of a rotation matrix around one of the three axes
void Int_Vrot(MIPSOpcode op) {
    float d[4]{};
    int vd = _VD;
    int vs = _VS;
    int imm = (op >> 16) & 0x1f;
    VectorSize sz = GetVecSize(op);
    bool negSin = (imm & 0x10) != 0;
    int sineLane = (imm >> 2) & 3;
    int cosineLane = imm & 3;

    float sine, cosine;
    if (cpu.vfpuCtrl[VFPU_CTRL_SPREFIX] == 0x000E4) {
        vfpu_sincos(V(vs), sine, cosine);
        if (negSin)
            sine = -sine;
    } else {
        // Swizzle on S is a bit odd here, but generally only applies to sine.
        float s[4]{};
        ReadVector(s, V_Single, vs);
        u32 sprefixRemove = VFPU_NEGATE(1, 0, 0, 0);
        // We apply negSin later, not here.  This handles zero a bit better.
        u32 sprefixAdd = VFPU_NEGATE(0, 0, 0, 0);
        ApplyPrefixST(s, VFPURewritePrefix(VFPU_CTRL_SPREFIX, sprefixRemove, sprefixAdd), V_Single);

        // Cosine ignores all prefixes, so take the original.
        cosine = vfpu_cos(V(vs));
        sine = vfpu_sin(s[0]);

        if (negSin)
            sine = -sine;
        RetainInvalidSwizzleST(&sine, V_Single);
    }

    if (sineLane == cosineLane) {
        for (int i = 0; i < 4; i++)
            d[i] = sine;
    } else {
        d[sineLane] = sine;
    }

    if (((vd >> 2) & 7) == ((vs >> 2) & 7)) {
        u8 dregs[4]{};
        GetVectorRegs(dregs, sz, vd);
        // Calculate cosine based on sine/zero result.
        bool written = false;
        for (int i = 0; i < 4; i++) {
            if (vs == dregs[i]) {
                d[cosineLane] = vfpu_cos(d[i]);
                written = true;
                break;
            }
        }
        if (!written)
            d[cosineLane] = cosine;
    } else {
        d[cosineLane] = cosine;
    }

    // D prefix works, just not for the cosine lane.
    uint32_t dprefixRemove = (3 << cosineLane) | (1 << (8 + cosineLane));
    cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] &= 0xFFFFF ^ dprefixRemove;
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vtfm(MIPSOpcode op) {
    float s[16]{}, t[4]{};
    FloatBits d;
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    int ins = (op >> 23) & 3;

    VectorSize sz = (VectorSize)(ins + 1);
    MatrixSize msz = (MatrixSize)(ins + 1);
    int n = GetNumVectorElements(GetVecSize(op));

    int tn = std::min(n, ins + 1);
    ReadMatrix(s, msz, vs);
    ReadVector(t, sz, vt);

    for (int i = 0; i < ins; i++) {
        d.f[i] = s[i * 4] * t[0];
        for (int k = 1; k < tn; k++) {
            d.f[i] += s[i * 4 + k] * t[k];
        }
        if (ins >= n) {
            d.f[i] += s[i * 4 + ins];
        }
    }

    // S and T prefixes apply for the final row only.
    // The T prefix is used to apply zero/one constants, but abs still changes it.
    ApplySwizzleS(&s[ins * 4], V_Quad);
    VFPUConst constX = VFPUConst::NONE;
    VFPUConst constY = n < 2 ? VFPUConst::ZERO : VFPUConst::NONE;
    VFPUConst constZ = n < 3 ? VFPUConst::ZERO : VFPUConst::NONE;
    VFPUConst constW = n < 4 ? VFPUConst::ZERO : VFPUConst::NONE;
    if (ins >= n) {
        if (ins == 1) {
            constY = VFPUConst::ONE;
        } else if (ins == 2) {
            constZ = VFPUConst::ONE;
        } else if (ins == 3) {
            constW = VFPUConst::ONE;
        }
    }
    u32 tprefixRemove = VFPU_SWIZZLE(0, n < 2 ? 3 : 0, n < 3 ? 3 : 0, n < 4 ? 3 : 0);
    u32 tprefixAdd = VFPU_MAKE_CONSTANTS(constX, constY, constZ, constW);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);

    d.f[ins] = s[ins * 4] * t[0];
    for (int k = 1; k < 4; k++) {
        d.f[ins] += s[ins * 4 + k] * t[k];
    }

    // D prefix applies to the last element only.
    u32 lastmask = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & (1 << 8)) << ins;
    u32 lastsat = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & 3) << (ins + ins);
    cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = lastmask | lastsat;
    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_SV(MIPSOpcode op)
{
    s32 imm = SignExtend16ToS32(op & 0xFFFC);
    int vt = ((op >> 16) & 0x1f) | ((op & 3) << 5);
    int rs = _RS;
    u32 addr = R(rs) + imm;

    switch (op >> 26)
    {
    case 50: //lv.s
    VI(vt) = Memory::Read_U32(addr);
    break;
    case 58: //sv.s
    Memory::Write_U32(VI(vt), addr);
    break;
    default:
    break;
    }
    PC += 4;
}


void Int_Mftv(MIPSOpcode op)
{
    int imm = op & 0xFF;
    int rt = _RT;
    switch ((op >> 21) & 0x1f)
    {
    case 3: //mfv / mfvc
    // rt = 0, imm = 255 appears to be used as a CPU interlock by some games.
    if (rt != 0) {
        if (imm < 128) {
            R(rt) = VI(imm);
        } else if (imm < 128 + VFPU_CTRL_MAX) { //mfvc
            R(rt) = cpu.vfpuCtrl[imm - 128];
        } else {
            //ERROR - maybe need to make this value too an "interlock" value?
        }
    }
    break;

    case 7: //mtv
    if (imm < 128) {
        VI(imm) = R(rt);
    } else if (imm < 128 + VFPU_CTRL_MAX) { //mtvc
        u32 mask;
        if (GetVFPUCtrlMask(imm - 128, &mask)) {
            cpu.vfpuCtrl[imm - 128] = R(rt) & mask;
        }
    } else {
        //ERROR
    }
    break;

    default:
    break;
    }
    PC += 4;
}

void Int_Vmfvc(MIPSOpcode op) {
    int vd = _VD;
    int imm = (op >> 8) & 0x7F;
    if (imm < VFPU_CTRL_MAX) {
        VI(vd) = cpu.vfpuCtrl[imm];
    } else {
        VI(vd) = 0;
    }
    PC += 4;
}

void Int_Vmtvc(MIPSOpcode op) {
    int vs = _VS;
    int imm = op & 0x7F;
    if (imm < VFPU_CTRL_MAX) {
        u32 mask;
        if (GetVFPUCtrlMask(imm, &mask)) {
            cpu.vfpuCtrl[imm] = VI(vs) & mask;
        }
    }
    PC += 4;
}

void Int_Vcst(MIPSOpcode op)
{
    int conNum = (op >> 16) & 0x1f;
    int vd = _VD;

    VectorSize sz = GetVecSize(op);
    float c = cst_constants[conNum];
    float temp[4] = {c,c,c,c};
    ApplyPrefixD(temp, sz);
    WriteVector(temp, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vcmp(MIPSOpcode op)
{
    int vs = _VS;
    int vt = _VT;
    int cond = op & 0xf;
    VectorSize sz = GetVecSize(op);
    int n = GetNumVectorElements(sz);
    float s[4];
    float t[4];
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vt);
    ApplySwizzleT(t, sz);
    int cc = 0;
    int or_val = 0;
    int and_val = 1;
    int affected_bits = (1 << 4) | (1 << 5);  // 4 and 5
    for (int i = 0; i < n; i++)
    {
        int c;
        // These set c to 0 or 1, nothing else.
        switch (cond)
        {
        case VC_FL: c = 0; break;
        case VC_EQ: c = s[i] == t[i]; break;
        case VC_LT: c = s[i] < t[i]; break;
        case VC_LE: c = s[i] <= t[i]; break;

        case VC_TR: c = 1; break;
        case VC_NE: c = s[i] != t[i]; break;
        case VC_GE: c = s[i] >= t[i]; break;
        case VC_GT: c = s[i] > t[i]; break;

        case VC_EZ: c = s[i] == 0.0f || s[i] == -0.0f; break;
        case VC_EN: c = my_isnan(s[i]); break;
        case VC_EI: c = my_isinf(s[i]); break;
        case VC_ES: c = my_isnanorinf(s[i]); break;   // Tekken Dark Resurrection

        case VC_NZ: c = s[i] != 0; break;
        case VC_NN: c = !my_isnan(s[i]); break;
        case VC_NI: c = !my_isinf(s[i]); break;
        case VC_NS: c = !(my_isnanorinf(s[i])); break;   // How about t[i] ?

        default:
        PC += 4;
        EatPrefixes();
        return;
        }
        cc |= (c<<i);
        or_val |= c;
        and_val &= c;
        affected_bits |= 1 << i;
    }
    // Use masking to only change the affected bits
    cpu.vfpuCtrl[VFPU_CTRL_CC] =
        (cpu.vfpuCtrl[VFPU_CTRL_CC] & ~affected_bits) |
        ((cc | (or_val << 4) | (and_val << 5)) & affected_bits);
    PC += 4;
    EatPrefixes();
}

void Int_Vminmax(MIPSOpcode op) {
    FloatBits s, t, d;
    int vt = _VT;
    int vs = _VS;
    int vd = _VD;
    int cond = op&15;
    VectorSize sz = GetVecSize(op);
    int numElements = GetNumVectorElements(sz);

    ReadVector(s.f, sz, vs);
    ApplySwizzleS(s.f, sz);
    ReadVector(t.f, sz, vt);
    ApplySwizzleT(t.f, sz);

    // If both are zero, take t's sign.
    // Otherwise: -NAN < -INF < real < INF < NAN (higher mantissa is farther from 0.)

    switch ((op >> 23) & 3) {
    case 2: // vmin
    for (int i = 0; i < numElements; i++) {
        if (my_isnanorinf(s.f[i]) || my_isnanorinf(t.f[i])) {
            // If both are negative, we flip the comparison (not two's compliment.)
            if (s.i[i] < 0 && t.i[i] < 0) {
                // If at least one side is NAN, we take the highest mantissa bits.
                d.i[i] = std::max(t.i[i], s.i[i]);
            } else {
                // Otherwise, we take the lowest value (negative or lowest mantissa.)
                d.i[i] = std::min(t.i[i], s.i[i]);
            }
        } else {
            d.f[i] = std::min(t.f[i], s.f[i]);
        }
    }
    break;
    case 3: // vmax
    for (int i = 0; i < numElements; i++) {
        // This is the same logic as vmin, just reversed.
        if (my_isnanorinf(s.f[i]) || my_isnanorinf(t.f[i])) {
            if (s.i[i] < 0 && t.i[i] < 0) {
                d.i[i] = std::min(t.i[i], s.i[i]);
            } else {
                d.i[i] = std::max(t.i[i], s.i[i]);
            }
        } else {
            d.f[i] = std::max(t.f[i], s.f[i]);
        }
    }
    break;
    default:
    PC += 4;
    EatPrefixes();
    return;
    }
    RetainInvalidSwizzleST(d.f, sz);
    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vscmp(MIPSOpcode op) {
    FloatBits s, t, d;
    int vt = _VT;
    int vs = _VS;
    int vd = _VD;
    VectorSize sz = GetVecSize(op);
    ReadVector(s.f, sz, vs);
    ApplySwizzleS(s.f, sz);
    ReadVector(t.f, sz, vt);
    ApplySwizzleT(t.f, sz);
    int n = GetNumVectorElements(sz);
    for (int i = 0; i < n ; i++) {
        float a = s.f[i] - t.f[i];
        if (my_isnan(a)) {
            // NAN/INF are treated as just larger numbers, as in vmin/vmax.
            int sMagnitude = s.u[i] & 0x7FFFFFFF;
            int tMagnitude = t.u[i] & 0x7FFFFFFF;
            int b = (s.i[i] < 0 ? -sMagnitude : sMagnitude) - (t.i[i] < 0 ? -tMagnitude : tMagnitude);
            d.f[i] = (float)((0 < b) - (b < 0));
        } else {
            d.f[i] = (float)((0.0f < a) - (a < 0.0f));
        }
    }
    RetainInvalidSwizzleST(d.f, sz);
    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsge(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vt = _VT;
    int vs = _VS;
    int vd = _VD;
    VectorSize sz = GetVecSize(op);
    int numElements = GetNumVectorElements(sz);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vt);
    ApplySwizzleT(t, sz);
    for (int i = 0; i < numElements; i++) {
        if ( my_isnan(s[i]) || my_isnan(t[i]) )
            d[i] = 0.0f;
        else
            d[i] = s[i] >= t[i] ? 1.0f : 0.0f;
    }
    RetainInvalidSwizzleST(d, sz);
    // The clamp cannot matter, so skip it.
    ApplyPrefixD(d, sz, true);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vslt(MIPSOpcode op) {
    float s[4], t[4], d[4];
    int vt = _VT;
    int vs = _VS;
    int vd = _VD;
    VectorSize sz = GetVecSize(op);
    int numElements = GetNumVectorElements(sz);
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    ReadVector(t, sz, vt);
    ApplySwizzleT(t, sz);
    for (int i = 0; i < numElements; i++) {
        if ( my_isnan(s[i]) || my_isnan(t[i]) )
            d[i] = 0.0f;
        else
            d[i] = s[i] < t[i] ? 1.0f : 0.0f;
    }
    RetainInvalidSwizzleST(d, sz);
    // The clamp cannot matter, so skip it.
    ApplyPrefixD(d, sz, true);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}


void Int_Vcmov(MIPSOpcode op) {
    int vs = _VS;
    int vd = _VD;
    int tf = (op >> 19) & 1;
    int imm3 = (op >> 16) & 7;
    VectorSize sz = GetVecSize(op);
    int n = GetNumVectorElements(sz);
    float s[4];
    float d[4];
    ReadVector(s, sz, vs);
    ApplySwizzleS(s, sz);
    // Not only is D read (as T), but the T prefix applies to it.
    ReadVector(d, sz, vd);
    ApplySwizzleT(d, sz);

    int CC = cpu.vfpuCtrl[VFPU_CTRL_CC];

    if (imm3 < 6) {
        if (((CC >> imm3) & 1) == !tf) {
            for (int i = 0; i < n; i++)
                d[i] = s[i];
        }
    } else if (imm3 == 6) {
        for (int i = 0; i < n; i++) {
            if (((CC >> i) & 1) == !tf)
                d[i] = s[i];
        }
    }
    ApplyPrefixD(d, sz);
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_VecDo3(MIPSOpcode op) {
    float s[4], t[4];
    FloatBits d;
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);

    int optype = 0;
    switch (op >> 26) {
    case 24: //VFPU0
    switch ((op >> 23) & 7) {
    case 0: optype = 0; break;
    case 1: optype = 1; break;
    case 7: optype = 7; break;
    default: goto bad;
    }
    break;
    case 25: //VFPU1
    switch ((op >> 23) & 7) {
    case 0: optype = 8; break;
    default: goto bad;
    }
    break;
    default:
    bad:
    break;
    }

    u32 n = GetNumVectorElements(sz);
    ReadVector(s, sz, vs);
    ReadVector(t, sz, vt);
    if (optype != 7) {
        ApplySwizzleS(s, sz);
        ApplySwizzleT(t, sz);
    } else {
        // The prefix handling of S/T is a bit odd, probably the HW doesn't do it in parallel.
        // The X prefix is applied to the last element in sz.
        // TODO: This doesn't match exactly for a swizzle past x in some cases...
        ApplySwizzleS(&s[n - 1], V_Single, -INFINITY);
        ApplySwizzleT(&t[n - 1], V_Single, -INFINITY);
    }

    for (int i = 0; i < (int)n; i++) {
        switch (optype) {
        case 0: d.f[i] = s[i] + t[i]; break; //vadd
        case 1: d.f[i] = s[i] - t[i]; break; //vsub
        case 7: d.f[i] = s[i] / t[i]; break; //vdiv
        case 8: d.f[i] = s[i] * t[i]; break; //vmul
        }
    }

    // For vdiv only, the D prefix only applies mask (and like S/T, x applied to last.)
    if (optype == 7) {
        u32 lastmask = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & (1 << 8)) << (n - 1);
        u32 lastsat = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & 3) << (n + n - 2);
        cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = lastmask | lastsat;
        ApplyPrefixD(d.f, sz);
    } else {
        RetainInvalidSwizzleST(d.f, sz);
        ApplyPrefixD(d.f, sz);
    }
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_CrossQuat(MIPSOpcode op) {
    float s[4]{}, t[4]{}, d[4];
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);
    u32 n = GetNumVectorElements(sz);
    ReadVector(s, sz, vs);
    ReadVector(t, sz, vt);

    u32 tprefixRemove = VFPU_ANY_SWIZZLE() | VFPU_NEGATE(1, 1, 1, 1);
    u32 tprefixAdd;

    switch (sz) {
    case V_Triple:  // vcrsp.t
    {
        d[0] = s[1] * t[2] - s[2] * t[1];
        d[1] = s[2] * t[0] - s[0] * t[2];

        // T prefix forces swizzle and negate, can be used to have weird constants.
        tprefixAdd = VFPU_SWIZZLE(1, 0, 3, 2) | VFPU_NEGATE(0, 1, 0, 0);
        ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);
        ApplySwizzleS(s, V_Quad);
        d[2] = s[0] * t[0] + s[1] * t[1] + s[2] * t[2] + s[3] * t[3];
        break;
    }

    case V_Quad:   // vqmul.q
    {
        d[0] = s[0] * t[3] + s[1] * t[2] - s[2] * t[1] + s[3] * t[0];
        d[1] = -s[0] * t[2] + s[1] * t[3] + s[2] * t[0] + s[3] * t[1];
        d[2] = s[0] * t[1] - s[1] * t[0] + s[2] * t[3] + s[3] * t[2];

        // T prefix forces swizzle and negate, can be used to have weird constants.
        tprefixAdd = VFPU_SWIZZLE(0, 1, 2, 3) | VFPU_NEGATE(1, 1, 1, 0);
        ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);
        ApplySwizzleS(s, sz);
        d[3] = s[0] * t[0] + s[1] * t[1] + s[2] * t[2] + s[3] * t[3];
        break;
    }

    case V_Pair:
    // t swizzles invalid so the multiply is always zero.
    d[0] = 0;

    tprefixAdd = VFPU_SWIZZLE(0, 0, 0, 0) | VFPU_NEGATE(0, 0, 0, 0);
    ApplyPrefixST(t, VFPURewritePrefix(VFPU_CTRL_TPREFIX, tprefixRemove, tprefixAdd), V_Quad);
    ApplySwizzleS(s, V_Quad);
    // It's possible to populate a value by swizzling s[2].
    d[1] = s[2] * t[2];
    break;

    case V_Single:
    // t swizzles invalid so the multiply is always zero.
    d[0] = 0;
    break;

    default:

    break;
    }

    // D prefix applies to the last element only (mask and sat) for pair and larger.
    if (sz != V_Single) {
        u32 lastmask = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & (1 << 8)) << (n - 1);
        u32 lastsat = (cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] & 3) << (n + n - 2);
        cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = lastmask | lastsat;
        ApplyPrefixD(d, sz);
    } else {
        // Single always seems to write out zero.
        cpu.vfpuCtrl[VFPU_CTRL_DPREFIX] = 0;
    }
    WriteVector(d, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vlgb(MIPSOpcode op) {
    // Vector log binary (extract exponent)
    FloatBits d, s;
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);

    ReadVector(s.f, sz, vs);
    ApplySwizzleS(s.f, sz);

    int exp = (s.u[0] & 0x7F800000) >> 23;
    if (exp == 0xFF) {
        d.f[0] = s.f[0];
    } else if (exp == 0) {
        d.f[0] = -INFINITY;
    } else {
        d.f[0] = (float)(exp - 127);
    }

    // If sz is greater than V_Single, the rest are copied unchanged.
    for (int i = 1; i < GetNumVectorElements(sz); ++i) {
        d.u[i] = s.u[i];
    }

    RetainInvalidSwizzleST(d.f, sz);
    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vwbn(MIPSOpcode op) {
    FloatBits d, s;
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);
    u8 exp = (u8)((op >> 16) & 0xFF);

    ReadVector(s.f, sz, vs);
    ApplySwizzleS(s.f, sz);

    u32 sigbit = s.u[0] & 0x80000000;
    u32 prevExp = (s.u[0] & 0x7F800000) >> 23;
    u32 mantissa = (s.u[0] & 0x007FFFFF) | 0x00800000;
    if (prevExp != 0xFF && prevExp != 0) {
        if (exp > prevExp) {
            s8 shift = (exp - prevExp) & 0xF;
            mantissa = mantissa >> shift;
        } else {
            s8 shift = (prevExp - exp) & 0xF;
            mantissa = mantissa << shift;
        }
        d.u[0] = sigbit | (mantissa & 0x007FFFFF) | (exp << 23);
    } else {
        d.u[0] = s.u[0] | (exp << 23);
    }

    // If sz is greater than V_Single, the rest are copied unchanged.
    for (int i = 1; i < GetNumVectorElements(sz); ++i) {
        d.u[i] = s.u[i];
    }

    RetainInvalidSwizzleST(d.f, sz);
    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsbn(MIPSOpcode op) {
    FloatBits d, s, t;
    int vd = _VD;
    int vs = _VS;
    int vt = _VT;
    VectorSize sz = GetVecSize(op);

    ReadVector(s.f, sz, vs);
    ApplySwizzleS(s.f, sz);
    ReadVector(t.f, sz, vt);
    ApplySwizzleT(t.f, sz);
    // Swizzle does apply to the value read as an integer.
    u8 exp = (u8)(127 + t.i[0]);

    // Simply replace the exponent bits.
    u32 prev = s.u[0] & 0x7F800000;
    if (prev != 0 && prev != 0x7F800000) {
        d.u[0] = (s.u[0] & ~0x7F800000) | (exp << 23);
    } else {
        d.u[0] = s.u[0];
    }

    // If sz is greater than V_Single, the rest are copied unchanged.
    for (int i = 1; i < GetNumVectorElements(sz); ++i) {
        d.u[i] = s.u[i];
    }

    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void Int_Vsbz(MIPSOpcode op) {
    // Vector scale by zero (set exp to 0 to extract mantissa)
    FloatBits d, s;
    int vd = _VD;
    int vs = _VS;
    VectorSize sz = GetVecSize(op);

    ReadVector(s.f, sz, vs);
    ApplySwizzleS(s.f, sz);

    // NAN and denormals pass through.
    if (my_isnan(s.f[0]) || (s.u[0] & 0x7F800000) == 0) {
        d.u[0] = s.u[0];
    } else {
        d.u[0] = (127 << 23) | (s.u[0] & 0x007FFFFF);
    }

    // If sz is greater than V_Single, the rest are copied unchanged.
    for (int i = 1; i < GetNumVectorElements(sz); ++i) {
        d.u[i] = s.u[i];
    }

    ApplyPrefixD(d.f, sz);
    WriteVector(d.f, sz, vd);
    PC += 4;
    EatPrefixes();
}

void _executeVFPU(uint32_t opcode) {
    auto func = tableImmediate[(opcode >> 26) & 0x3F];

    if (func)
        func(opcode);
    else {
        debugAllegrexState();
        LOG_ERROR(logType, "unknown VFPU opcode 0x%02x @ 0x%08x", (opcode >> 26) & 0x3F, PC);
        Core::Allegrex::setProcessorFailed(true);
    }
}
}