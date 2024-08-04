// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#include <cstdint>
#include <limits>
#include <cstdio>
#include <cstring>

#include <Core/Allegrex/AllegrexState.h>

#include "BitScan.h"

#include "MIPSVFPUUtils.h"
#include "MIPSVFPUFallbacks.h"

#ifdef _MSC_VER
#pragma warning(disable: 4146)
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;

union float2int {
    uint32_t i;
    float f;
};

void GetVectorRegs(u8 regs[4], VectorSize N, int vectorReg) {
    int mtx = (vectorReg >> 2) & 7;
    int col = vectorReg & 3;
    int row = 0;
    int length = 0;
    int transpose = (vectorReg>>5) & 1;

    switch (N) {
    case V_Single: transpose = 0; row=(vectorReg>>5)&3; length = 1; break;
    case V_Pair:   row=(vectorReg>>5)&2; length = 2; break;
    case V_Triple: row=(vectorReg>>6)&1; length = 3; break;
    case V_Quad:   row=(vectorReg>>5)&2; length = 4; break;
    }

    for (int i = 0; i < length; i++) {
        int index = mtx * 4;
        if (transpose)
            index += ((row+i)&3) + col*32;
        else
            index += col + ((row+i)&3)*32;
        regs[i] = index;
    }
}

void GetMatrixRegs(u8 regs[16], MatrixSize N, int matrixReg) {
    int mtx = (matrixReg >> 2) & 7;
    int col = matrixReg & 3;

    int row = 0;
    int side = 0;
    int transpose = (matrixReg >> 5) & 1;

    switch (N) {
    case M_1x1: transpose = 0; row = (matrixReg >> 5) & 3; side = 1; break;
    case M_2x2: row = (matrixReg >> 5) & 2; side = 2; break;
    case M_3x3: row = (matrixReg >> 6) & 1; side = 3; break;
    case M_4x4: row = (matrixReg >> 5) & 2; side = 4; break;
    }

    for (int i = 0; i < side; i++) {
        for (int j = 0; j < side; j++) {
            int index = mtx * 4;
            if (transpose)
                index += ((row+i)&3) + ((col+j)&3)*32;
            else
                index += ((col+j)&3) + ((row+i)&3)*32;
            regs[j*4 + i] = index;
        }
    }
}


int GetColumnName(int matrix, MatrixSize msize, int column, int offset) {
    return matrix * 4 + column + offset * 32;
}

int GetRowName(int matrix, MatrixSize msize, int column, int offset) {
    return 0x20 | (matrix * 4 + column + offset * 32);
}

void GetMatrixColumns(int matrixReg, MatrixSize msize, u8 vecs[4]) {
    int n = GetMatrixSide(msize);

    int col = matrixReg & 3;
    int row = (matrixReg >> 5) & 2;
    int transpose = (matrixReg >> 5) & 1;

    for (int i = 0; i < n; i++) {
        vecs[i] = (transpose << 5) | (row << 5) | (matrixReg & 0x1C) | (i + col);
    }
}

void GetMatrixRows(int matrixReg, MatrixSize msize, u8 vecs[4]) {
    int n = GetMatrixSide(msize);
    int col = matrixReg & 3;
    int row = (matrixReg >> 5) & 2;

    int swappedCol = row ? (msize == M_3x3 ? 1 : 2) : 0;
    int swappedRow = col ? 2 : 0;
    int transpose = ((matrixReg >> 5) & 1) ^ 1;

    for (int i = 0; i < n; i++) {
        vecs[i] = (transpose << 5) | (swappedRow << 5) | (matrixReg & 0x1C) | (i + swappedCol);
    }
}

static u8 voffset[128];

void ReadVector(float *rd, VectorSize size, int reg) {
    int row;
    int length;
    switch (size) {
    case V_Single: rd[0] = Core::Allegrex::cpu.vprf[voffset[reg]]; return; // transpose = 0; row=(reg>>5)&3; length = 1; break;
    case V_Pair:   row=(reg>>5)&2; length = 2; break;
    case V_Triple: row=(reg>>6)&1; length = 3; break;
    case V_Quad:   row=(reg>>5)&2; length = 4; break;
    default: length = 0; break;
    }
    int transpose = (reg >> 5) & 1;
    const int mtx = ((reg << 2) & 0x70);
    const int col = reg & 3;
    // NOTE: We now skip the voffset lookups.
    if (transpose) {
        const int base = mtx + col;
        for (int i = 0; i < length; i++)
            rd[i] = Core::Allegrex::cpu.vprf[base + ((row + i) & 3) * 4];
    } else {
        const int base = mtx + col * 4;

        for (int i = 0; i < length; i++) {
            rd[i] = Core::Allegrex::cpu.vprf[base + ((row + i) & 3)];
        }
    }
}

void WriteVector(const float *rd, VectorSize size, int reg) {
    int row;
    int length;

    switch (size) {
    case V_Single: if (!Core::Allegrex::cpu.VfpuWriteMask(0)) Core::Allegrex::cpu.vprf[voffset[reg]] = rd[0]; return; // transpose = 0; row=(reg>>5)&3; length = 1; break;
    case V_Pair:   row=(reg>>5)&2; length = 2; break;
    case V_Triple: row=(reg>>6)&1; length = 3; break;
    case V_Quad:   row=(reg>>5)&2; length = 4; break;
    default: length = 0; break;
    }

    const int mtx = ((reg << 2) & 0x70);
    const int col = reg & 3;
    bool transpose = (reg >> 5) & 1;
    // NOTE: We now skip the voffset lookups.
    if (transpose) {
        const int base = mtx + col;
        if (Core::Allegrex::cpu.VfpuWriteMask() == 0) {
            for (int i = 0; i < length; i++)
                Core::Allegrex::cpu.vprf[base + ((row+i) & 3) * 4] = rd[i];
        } else {
            for (int i = 0; i < length; i++) {
                if (!Core::Allegrex::cpu.VfpuWriteMask(i)) {
                    Core::Allegrex::cpu.vprf[base + ((row+i) & 3) * 4] = rd[i];
                }
            }
        }
    } else {
        const int base = mtx + col * 4;
        if (Core::Allegrex::cpu.VfpuWriteMask() == 0) {
            for (int i = 0; i < length; i++)
                Core::Allegrex::cpu.vprf[base + ((row + i) & 3)] = rd[i];
        } else {
            for (int i = 0; i < length; i++) {
                if (!Core::Allegrex::cpu.VfpuWriteMask(i)) {
                    Core::Allegrex::cpu.vprf[base + ((row + i) & 3)] = rd[i];
                }
            }
        }
    }
}

u32 VFPURewritePrefix(int ctrl, u32 remove, u32 add) {
    u32 prefix = Core::Allegrex::cpu.vfpuCtrl[ctrl];
    return (prefix & ~remove) | add;
}

void ReadMatrix(float *rd, MatrixSize size, int reg) {
    int row = 0;
    int side = 0;
    int transpose = (reg >> 5) & 1;

    switch (size) {
    case M_1x1: transpose = 0; row = (reg >> 5) & 3; side = 1; break;
    case M_2x2: row = (reg >> 5) & 2; side = 2; break;
    case M_3x3: row = (reg >> 6) & 1; side = 3; break;
    case M_4x4: row = (reg >> 5) & 2; side = 4; break;
    default: side = 0; break;
    }

    int mtx = (reg >> 2) & 7;
    int col = reg & 3;

    // The voffset ordering is now integrated in these formulas,
    // eliminating a table lookup.
    const float *v = Core::Allegrex::cpu.vprf + (size_t)mtx * 16;
    if (transpose) {
        if (side == 4 && col == 0 && row == 0) {
            // Fast path: Simple 4x4 transpose. TODO: Optimize.
            for (int j = 0; j < 4; j++) {
                for (int i = 0; i < 4; i++) {
                    rd[j * 4 + i] = v[i * 4 + j];
                }
            }
        } else {
            for (int j = 0; j < side; j++) {
                for (int i = 0; i < side; i++) {
                    int index = ((row + i) & 3) * 4 + ((col + j) & 3);
                    rd[j * 4 + i] = v[index];
                }
            }
        }
    } else {
        if (side == 4 && col == 0 && row == 0) {
            // Fast path
            memcpy(rd, v, sizeof(float) * 16);  // rd[j * 4 + i] = v[j * 4 + i];
        } else {
            for (int j = 0; j < side; j++) {
                for (int i = 0; i < side; i++) {
                    int index = ((col + j) & 3) * 4 + ((row + i) & 3);
                    rd[j * 4 + i] = v[index];
                }
            }
        }
    }
}

void WriteMatrix(const float *rd, MatrixSize size, int reg) {
    int mtx = (reg>>2)&7;
    int col = reg&3;

    int row;
    int side;
    int transpose = (reg >> 5) & 1;

    switch (size) {
    case M_1x1: transpose = 0; row = (reg >> 5) & 3; side = 1; break;
    case M_2x2: row = (reg >> 5) & 2; side = 2; break;
    case M_3x3: row = (reg >> 6) & 1; side = 3; break;
    case M_4x4: row = (reg >> 5) & 2; side = 4; break;
    default: side = 0;
    }

    // The voffset ordering is now integrated in these formulas,
    // eliminating a table lookup.
    float *v = cpu.vprf + (size_t)mtx * 16;
    if (transpose) {
        if (side == 4 && row == 0 && col == 0 && Core::Allegrex::cpu.VfpuWriteMask() == 0x0) {
            // Fast path: Simple 4x4 transpose. TODO: Optimize.
            for (int j = 0; j < side; j++) {
                for (int i = 0; i < side; i++) {
                    v[i * 4 + j] = rd[j * 4 + i];
                }
            }
        } else {
            for (int j = 0; j < side; j++) {
                for (int i = 0; i < side; i++) {
                    if (j != side - 1 || !Core::Allegrex::cpu.VfpuWriteMask(i)) {
                        int index = ((row + i) & 3) * 4 + ((col + j) & 3);
                        v[index] = rd[j * 4 + i];
                    }
                }
            }
        }
    } else {
        if (side == 4 && row == 0 && col == 0 && Core::Allegrex::cpu.VfpuWriteMask() == 0x0) {
            memcpy(v, rd, sizeof(float) * 16);  // v[j * 4 + i] = rd[j * 4 + i];
        } else {
            for (int j = 0; j < side; j++) {
                for (int i = 0; i < side; i++) {
                    if (j != side - 1 || !Core::Allegrex::cpu.VfpuWriteMask(i)) {
                        int index = ((col + j) & 3) * 4 + ((row + i) & 3);
                        v[index] = rd[j * 4 + i];
                    }
                }
            }
        }
    }
}

int GetVectorOverlap(int vec1, VectorSize size1, int vec2, VectorSize size2) {
    // Different matrices?  Can't overlap, return early.
    if (((vec1 >> 2) & 7) != ((vec2 >> 2) & 7))
        return 0;

    int n1 = GetNumVectorElements(size1);
    int n2 = GetNumVectorElements(size2);
    u8 regs1[4];
    u8 regs2[4];
    GetVectorRegs(regs1, size1, vec1);
    GetVectorRegs(regs2, size1, vec2);
    int count = 0;
    for (int i = 0; i < n1; i++) {
        for (int j = 0; j < n2; j++) {
            if (regs1[i] == regs2[j])
                count++;
        }
    }
    return count;
}

VectorSize GetHalfVectorSizeSafe(VectorSize sz) {
    switch (sz) {
    case V_Pair: return V_Single;
    case V_Quad: return V_Pair;
    default: return V_Invalid;
    }
}

VectorSize GetHalfVectorSize(VectorSize sz) {
    VectorSize res = GetHalfVectorSizeSafe(sz);
    return res;
}

VectorSize GetDoubleVectorSizeSafe(VectorSize sz) {
    switch (sz) {
    case V_Single: return V_Pair;
    case V_Pair: return V_Quad;
    default: return V_Invalid;
    }
}

VectorSize GetDoubleVectorSize(VectorSize sz) {
    VectorSize res = GetDoubleVectorSizeSafe(sz);
    return res;
}

VectorSize GetVectorSizeSafe(MatrixSize sz) {
    switch (sz) {
    case M_1x1: return V_Single;
    case M_2x2: return V_Pair;
    case M_3x3: return V_Triple;
    case M_4x4: return V_Quad;
    default: return V_Invalid;
    }
}

VectorSize GetVectorSize(MatrixSize sz) {
    VectorSize res = GetVectorSizeSafe(sz);
    return res;
}

MatrixSize GetMatrixSizeSafe(VectorSize sz) {
    switch (sz) {
    case V_Single: return M_1x1;
    case V_Pair: return M_2x2;
    case V_Triple: return M_3x3;
    case V_Quad: return M_4x4;
    default: return M_Invalid;
    }
}

MatrixSize GetMatrixSize(VectorSize sz) {
    MatrixSize res = GetMatrixSizeSafe(sz);
    return res;
}

VectorSize MatrixVectorSizeSafe(MatrixSize sz) {
    switch (sz) {
    case M_1x1: return V_Single;
    case M_2x2: return V_Pair;
    case M_3x3: return V_Triple;
    case M_4x4: return V_Quad;
    default: return V_Invalid;
    }
}

VectorSize MatrixVectorSize(MatrixSize sz) {
    VectorSize res = MatrixVectorSizeSafe(sz);
    return res;
}

int GetMatrixSideSafe(MatrixSize sz) {
    switch (sz) {
    case M_1x1: return 1;
    case M_2x2: return 2;
    case M_3x3: return 3;
    case M_4x4: return 4;
    default: return 0;
    }
}

int GetMatrixSide(MatrixSize sz) {
    int res = GetMatrixSideSafe(sz);
    return res;
}

// TODO: Optimize
MatrixOverlapType GetMatrixOverlap(int mtx1, int mtx2, MatrixSize msize) {
    int n = GetMatrixSide(msize);

    if (mtx1 == mtx2)
        return OVERLAP_EQUAL;

    u8 m1[16];
    u8 m2[16];
    GetMatrixRegs(m1, msize, mtx1);
    GetMatrixRegs(m2, msize, mtx2);

    // Simply do an exhaustive search.
    for (int x = 0; x < n; x++) {
        for (int y = 0; y < n; y++) {
            int val = m1[y * 4 + x];
            for (int a = 0; a < n; a++) {
                for (int b = 0; b < n; b++) {
                    if (m2[a * 4 + b] == val) {
                        return OVERLAP_PARTIAL;
                    }
                }
            }
        }
    }

    return OVERLAP_NONE;
}

bool GetVFPUCtrlMask(int reg, u32 *mask) {
    switch ((VFPUControlRegisterEnum)reg) {
    case VFPUControlRegisterEnum::VFPU_CTRL_SPREFIX:
    case VFPUControlRegisterEnum::VFPU_CTRL_TPREFIX:
        *mask = 0x000FFFFF;
        return true;
    case VFPUControlRegisterEnum::VFPU_CTRL_DPREFIX:
        *mask = 0x00000FFF;
        return true;
    case VFPUControlRegisterEnum::VFPU_CTRL_CC:
        *mask = 0x0000003F;
        return true;
    case VFPUControlRegisterEnum::VFPU_CTRL_INF4:
        *mask = 0xFFFFFFFF;
        return true;
    case VFPUControlRegisterEnum::VFPU_CTRL_RSV5:
    case VFPUControlRegisterEnum::VFPU_CTRL_RSV6:
    case VFPUControlRegisterEnum::VFPU_CTRL_REV:
        // Don't change anything, these regs are read only.
        return false;
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX0:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX1:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX2:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX3:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX4:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX5:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX6:
    case VFPUControlRegisterEnum::VFPU_CTRL_RCX7:
        *mask = 0x3FFFFFFF;
        return true;
    default:
        return false;
    }
}

float Float16ToFloat32(unsigned short l)
{
    float2int f2i;

    unsigned short float16 = l;
    unsigned int sign = (float16 >> VFPU_SH_FLOAT16_SIGN) & VFPU_MASK_FLOAT16_SIGN;
    int exponent = (float16 >> VFPU_SH_FLOAT16_EXP) & VFPU_MASK_FLOAT16_EXP;
    unsigned int fraction = float16 & VFPU_MASK_FLOAT16_FRAC;

    float f;
    if (exponent == VFPU_FLOAT16_EXP_MAX)
    {
        f2i.i = sign << 31;
        f2i.i |= 255 << 23;
        f2i.i |= fraction;
        f = f2i.f;
    }
    else if (exponent == 0 && fraction == 0)
    {
        f = sign == 1 ? -0.0f : 0.0f;
    }
    else
    {
        if (exponent == 0)
        {
            do
            {
                fraction <<= 1;
                exponent--;
            }
            while (!(fraction & (VFPU_MASK_FLOAT16_FRAC + 1)));

            fraction &= VFPU_MASK_FLOAT16_FRAC;
        }

        /* Convert to 32-bit single-precision IEEE754. */
        f2i.i = sign << 31;
        f2i.i |= (exponent + 112) << 23;
        f2i.i |= fraction << 13;
        f=f2i.f;
    }
    return f;
}

// Reference C++ version.
static float vfpu_dot_cpp(const float a[4], const float b[4]) {
    static const int EXTRA_BITS = 2;
    float2int result;
    float2int src[2];

    int32_t exps[4];
    int32_t mants[4];
    int32_t signs[4];
    int32_t max_exp = 0;
    int32_t last_inf = -1;

    for (int i = 0; i < 4; i++) {
        src[0].f = a[i];
        src[1].f = b[i];

        int32_t aexp = get_uexp(src[0].i);
        int32_t bexp = get_uexp(src[1].i);
        int32_t amant = get_mant(src[0].i) << EXTRA_BITS;
        int32_t bmant = get_mant(src[1].i) << EXTRA_BITS;

        exps[i] = aexp + bexp - 127;
        if (aexp == 255) {
            // INF * 0 = NAN
            if ((src[0].i & 0x007FFFFF) != 0 || bexp == 0) {
                result.i = 0x7F800001;
                return result.f;
            }
            mants[i] = get_mant(0) << EXTRA_BITS;
            exps[i] = 255;
        } else if (bexp == 255) {
            if ((src[1].i & 0x007FFFFF) != 0 || aexp == 0) {
                result.i = 0x7F800001;
                return result.f;
            }
            mants[i] = get_mant(0) << EXTRA_BITS;
            exps[i] = 255;
        } else {
            // TODO: Adjust precision?
            uint64_t adjust = (uint64_t)amant * (uint64_t)bmant;
            mants[i] = (adjust >> (23 + EXTRA_BITS)) & 0x7FFFFFFF;
        }
        signs[i] = get_sign(src[0].i) ^ get_sign(src[1].i);

        if (exps[i] > max_exp) {
            max_exp = exps[i];
        }
        if (exps[i] >= 255) {
            // Infinity minus infinity is not a real number.
            if (last_inf != -1 && signs[i] != last_inf) {
                result.i = 0x7F800001;
                return result.f;
            }
            last_inf = signs[i];
        }
    }

    int32_t mant_sum = 0;
    for (int i = 0; i < 4; i++) {
        int exp = max_exp - exps[i];
        if (exp >= 32) {
            mants[i] = 0;
        } else {
            mants[i] >>= exp;
        }
        if (signs[i]) {
            mants[i] = -mants[i];
        }
        mant_sum += mants[i];
    }

    uint32_t sign_sum = 0;
    if (mant_sum < 0) {
        sign_sum = 0x80000000;
        mant_sum = -mant_sum;
    }

    // Truncate off the extra bits now.  We want to zero them for rounding purposes.
    mant_sum >>= EXTRA_BITS;

    if (mant_sum == 0 || max_exp <= 0) {
        return 0.0f;
    }

    int8_t shift = (int8_t)clz32_nonzero(mant_sum) - 8;
    if (shift < 0) {
        // Round to even if we'd shift away a 0.5.
        const uint32_t round_bit = 1 << (-shift - 1);
        if ((mant_sum & round_bit) && (mant_sum & (round_bit << 1))) {
            mant_sum += round_bit;
            shift = (int8_t)clz32_nonzero(mant_sum) - 8;
        } else if ((mant_sum & round_bit) && (mant_sum & (round_bit - 1))) {
            mant_sum += round_bit;
            shift = (int8_t)clz32_nonzero(mant_sum) - 8;
        }
        mant_sum >>= -shift;
        max_exp += -shift;
    } else {
        mant_sum <<= shift;
        max_exp -= shift;
    }

    if (max_exp >= 255) {
        max_exp = 255;
        mant_sum = 0;
    } else if (max_exp <= 0) {
        return 0.0f;
    }

    result.i = sign_sum | (max_exp << 23) | (mant_sum & 0x007FFFFF);
    return result.f;
}

float vfpu_dot(const float a[4], const float b[4]) {
    return vfpu_dot_cpp(a, b);
}

//==============================================================================
// The code below attempts to exactly match behaviour of
// PSP's vrnd instructions. See investigation starting around
// https://github.com/hrydgard/ppsspp/issues/16946#issuecomment-1467261209
// for details.

// Redundant currently, since MIPSState::Init() already
// does this on its own, but left as-is to be self-contained.
void vrnd_init_default(uint32_t *rcx) {
    rcx[0] = 0x00000001;
    rcx[1] = 0x00000002;
    rcx[2] = 0x00000004;
    rcx[3] = 0x00000008;
    rcx[4] = 0x00000000;
    rcx[5] = 0x00000000;
    rcx[6] = 0x00000000;
    rcx[7] = 0x00000000;
}

void vrnd_init(uint32_t seed, uint32_t *rcx) {
    for(int i = 0; i < 8; ++i) rcx[i] =
        0x3F800000u |                          // 1.0f mask.
        ((seed >> ((i / 4) * 16)) & 0xFFFFu) | // lower or upper half of the seed.
        (((seed >> (4 * i)) & 0xF) << 16);     // remaining nibble.

}

uint32_t vrnd_generate(uint32_t *rcx) {
    // The actual RNG state appears to be 5 parts
    // (32-bit each) stored into the registers as follows:
    uint32_t A = (rcx[0] & 0xFFFFu) | (rcx[4] << 16);
    uint32_t B = (rcx[1] & 0xFFFFu) | (rcx[5] << 16);
    uint32_t C = (rcx[2] & 0xFFFFu) | (rcx[6] << 16);
    uint32_t D = (rcx[3] & 0xFFFFu) | (rcx[7] << 16);
    uint32_t E = (((rcx[0] >> 16) & 0xF) <<  0) |
        (((rcx[1] >> 16) & 0xF) <<  4) |
        (((rcx[2] >> 16) & 0xF) <<  8) |
        (((rcx[3] >> 16) & 0xF) << 12) |
        (((rcx[4] >> 16) & 0xF) << 16) |
        (((rcx[5] >> 16) & 0xF) << 20) |
        (((rcx[6] >> 16) & 0xF) << 24) |
        (((rcx[7] >> 16) & 0xF) << 28);
    // Update.
    // LCG with classic parameters.
    A = 69069u * A + 1u; // NOTE: decimal constants.
    // Xorshift, with classic parameters. Algorithm "xor" from p. 4 of Marsaglia, "Xorshift RNGs".
    B ^= B << 13;
    B ^= B >> 17;
    B ^= B <<  5;
    // Sequence similar to Pell numbers ( https://en.wikipedia.org/wiki/Pell_number ),
    // except with different starting values, and an occasional increment (E).
    uint32_t t= 2u * D + C + E;
    // NOTE: the details of how E-part is set are somewhat of a guess
    // at the moment. The expression below looks weird, but does match
    // the available test data.
    E = uint32_t((uint64_t(C) + uint64_t(D >> 1) + uint64_t(E)) >> 32);
    C = D;
    D = t;
    // Store.
    rcx[0] = 0x3F800000u | (((E >>  0) & 0xF) << 16) | (A & 0xFFFFu);
    rcx[1] = 0x3F800000u | (((E >>  4) & 0xF) << 16) | (B & 0xFFFFu);
    rcx[2] = 0x3F800000u | (((E >>  8) & 0xF) << 16) | (C & 0xFFFFu);
    rcx[3] = 0x3F800000u | (((E >> 12) & 0xF) << 16) | (D & 0xFFFFu);
    rcx[4] = 0x3F800000u | (((E >> 16) & 0xF) << 16) | (A >> 16);
    rcx[5] = 0x3F800000u | (((E >> 20) & 0xF) << 16) | (B >> 16);
    rcx[6] = 0x3F800000u | (((E >> 24) & 0xF) << 16) | (C >> 16);
    rcx[7] = 0x3F800000u | (((E >> 28) & 0xF) << 16) | (D >> 16);
    // Return value.
    return A + B + D;
}

float vfpu_sin(float x) {
    return vfpu_sin_fallback(x);
}

float vfpu_cos(float x) {
    return vfpu_cos_fallback(x);
}

void vfpu_sincos(float a, float &s, float &c) {
    // Just invoke both sin and cos.
    // Suboptimal but whatever.
    s = vfpu_sin(a);
    c = vfpu_cos(a);
}

// Integer square root of 2^23*x (rounded to zero).
// Input is in 2^23 <= x < 2^25, and representable in float.
static inline uint32_t isqrt23(uint32_t x) {
    return uint32_t(int32_t(sqrt(double(x) * 8388608.0)));
}

float vfpu_sqrt(float x) {
    return vfpu_sqrt_fallback(x);
}

// Returns floor(2^33/sqrt(x)), for 2^22 <= x < 2^24.
static inline uint32_t rsqrt_floor22(uint32_t x) {
    return uint32_t(8589934592.0 / sqrt(double(x))); // 0x1p33
}

float vfpu_rsqrt(float x) {
    return vfpu_rsqrt_fallback(x);
}

float vfpu_asin(float x) {
    return vfpu_asin_fallback(x);
}

float vfpu_exp2(float x) {
    return vfpu_exp2_fallback(x);
}

float vfpu_rexp2(float x) {
    return vfpu_exp2(-x);
}

// Matches PSP output on all known values.
float vfpu_log2(float x) {
    return vfpu_log2_fallback(x);
}

static inline uint32_t vfpu_rcp_approx(uint32_t i) {
    return 0x3E800000u + (uint32_t((1ull << 47) / ((1ull << 23) + i)) & -4u);
}

float vfpu_rcp(float x) {
    return vfpu_rcp_fallback(x);
}