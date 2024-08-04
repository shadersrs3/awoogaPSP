#pragma once

namespace Core::Allegrex {
enum MIPSRegisterName : int {
    MIPS_REG_ZERO = 0,
    MIPS_REG_AT = 1,
    MIPS_REG_V0 = 2,
    MIPS_REG_V1 = 3,
    MIPS_REG_A0 = 4,
    MIPS_REG_A1 = 5,
    MIPS_REG_A2 = 6,
    MIPS_REG_A3 = 7,
    MIPS_REG_A4 = 8,
    MIPS_REG_A5 = 9,
    MIPS_REG_T0 = 8,
    MIPS_REG_T1 = 9,
    MIPS_REG_T2 = 10,
    MIPS_REG_T3 = 11,
    MIPS_REG_T4 = 12,
    MIPS_REG_T5 = 13,
    MIPS_REG_T6 = 14,
    MIPS_REG_T7 = 15,
    MIPS_REG_S0 = 16,
    MIPS_REG_S1 = 17,
    MIPS_REG_S2 = 18,
    MIPS_REG_S3 = 19,
    MIPS_REG_S4 = 20,
    MIPS_REG_S5 = 21,
    MIPS_REG_S6 = 22,
    MIPS_REG_S7 = 23,
    MIPS_REG_T8 = 24,
    MIPS_REG_T9 = 25,
    MIPS_REG_K0 = 26,
    MIPS_REG_K1 = 27,
    MIPS_REG_GP = 28,
    MIPS_REG_SP = 29,
    MIPS_REG_FP = 30,
    MIPS_REG_RA = 31,
};

enum VFPUControlRegisterEnum : int {
    VFPU_CTRL_SPREFIX, // vs
    VFPU_CTRL_TPREFIX, // vt
    VFPU_CTRL_DPREFIX, // vd
    VFPU_CTRL_CC,
    VFPU_CTRL_INF4,
    VFPU_CTRL_RSV5,
    VFPU_CTRL_RSV6,
    VFPU_CTRL_REV,
    VFPU_CTRL_RCX0,
    VFPU_CTRL_RCX1,
    VFPU_CTRL_RCX2,
    VFPU_CTRL_RCX3,
    VFPU_CTRL_RCX4,
    VFPU_CTRL_RCX5,
    VFPU_CTRL_RCX6,
    VFPU_CTRL_RCX7,
    VFPU_CTRL_MAX
};
}