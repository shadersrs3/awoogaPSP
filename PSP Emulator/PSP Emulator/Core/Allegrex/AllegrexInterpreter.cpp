#include <cmath>

#include <Core/Allegrex/Allegrex.h>
#include <Core/Allegrex/AllegrexState.h>
#include <Core/Allegrex/AllegrexInterpreter.h>
#include <Core/Allegrex/AllegrexVFPU.h>
#include <Core/Allegrex/AllegrexVFPUTable.h>
#include <Core/Allegrex/AllegrexSyscallHandler.h>
#include <Core/Allegrex/CPURegisterName.h>

#include <Core/Kernel/sceKernelThread.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Logger.h>

#include "BitScan.h"

#define RD() ((opcode >> 11) & 0x1F)
#define RS() ((opcode >> 21) & 0x1F)
#define RT() ((opcode >> 16) & 0x1F)
#define SA() ((opcode >> 6) & 0x1F)
#define FS() ((opcode >> 11) & 0x1F)
#define FT() ((opcode >> 16) & 0x1F)
#define FD() ((opcode >> 6 ) & 0x1F)
#define IMM() ((int)(int16_t)(opcode & 0xFFFF))
#define IMM2() ((uint32_t)opcode & 0xFFFF)
#define OPCODE() ((opcode >> 26) & 0x3F)
#define FUNC() (opcode & 0x3F)
#define TARGET() (opcode & 0x3FFFFFF)
#define POS()  ((opcode >> 6) & 0x1F)
#define SIZE() ((opcode >> 11) & 0x1F)

#define reg cpu.reg
#define pc cpu.pc
#define lo cpu.lo
#define hi cpu.hi

#define f cpu.f
#define _fs cpu.fs
#define fi cpu.fi

namespace Core::Allegrex {
static const char *logType = "AllegrexInterpreter";

static uint32_t __rotr(const uint32_t& x, int shift) {
    shift &= 31;
    if (!shift) return x;
    return (x >> shift) | (x << (32 - shift));
}

void handleSLL(uint32_t opcode) {
    reg[RD()] = reg[RT()] << SA();
    pc += 4;
}

void handleSRL(uint32_t opcode) {
    int rs = RS();
    int shift = SA();

    if (rs == 0) {
        reg[RD()] = reg[RT()] >> shift;
    } else if (rs == 1) {
        reg[RD()] = __rotr(reg[RT()], shift);
    } else {
        LOG_WARN(logType, "SRL/ROTR instruction RS encoding is wrong 0x%08X got %d!", pc, rs);
    }

    pc += 4;
}

void handleSRA(uint32_t opcode) {
    reg[RD()] = ((int32_t)reg[RT()]) >> SA();
    pc += 4;
}

void handleSLLV(uint32_t opcode) {
    reg[RD()] = reg[RT()] << (reg[RS()] & 0x1F);
    pc += 4;
}

void handleSRLV(uint32_t opcode) {
    int fd = FD();

    if (fd == 0) {
        reg[RD()] = reg[RT()] >> (reg[RS()] & 0x1F);
    } else if (fd == 1) {
        reg[RD()] = __rotr(reg[RT()], reg[RS()]);
    } else {
        LOG_WARN(logType, "SRLV/ROTRV instruction RS encoding is wrong 0x%08X got %d!", pc, fd);
    }

    pc += 4;
}

void handleSRAV(uint32_t opcode) {
    reg[RD()] = ((int32_t)reg[RT()]) >> (reg[RS()] & 0x1F);
    pc += 4;
}

void handleJR(uint32_t opcode) {
    if (!cpu.requiredBranching)
        cpu.jr(reg[RS()]);
}

void handleJALR(uint32_t opcode) {
    uint32_t linkAddress = pc + 8;
    int rd = RD();
    if (!cpu.requiredBranching)
        cpu.jal(reg[RS()]);
    else
        std::exit(0);
    reg[rd] = linkAddress;
}

void handleMOVZ(uint32_t opcode) {
    if (reg[RT()] == 0)
        reg[RD()] = reg[RS()];
    pc += 4;
}

void handleMOVN(uint32_t opcode) {
    if (reg[RT()] != 0)
        reg[RD()] = reg[RS()];
    pc += 4;
}

void handleSYSCALL(uint32_t opcode) {
    bool state = handleSyscallFromAddress(pc);

    if (!state) {
        setProcessorFailed(true);
        return;
    }

    pc += 4;
    reg[MIPS_REG_AT] = 0xDEADBEEF;
    for (int i = MIPS_REG_A0; i <= MIPS_REG_T7; i++)
        reg[i] = 0xDEADBEEF;

    reg[MIPS_REG_T8] = 0xDEADBEEF;
    reg[MIPS_REG_T9] = 0xDEADBEEF;
    hi = 0xDEADBEEF;
    lo = 0xDEADBEEF;
}

void handleMFHI(uint32_t opcode) {
    reg[RD()] = hi;
    pc += 4;
}

void handleMTHI(uint32_t opcode) {
    hi = reg[RS()];
    pc += 4;
}

void handleMFLO(uint32_t opcode) {
    reg[RD()] = lo;
    pc += 4;
}

void handleMTLO(uint32_t opcode) {
    lo = reg[RS()];
    pc += 4;
}

void handleCLZ(uint32_t opcode) {
    reg[RD()] = clz32(reg[RS()]);
    pc += 4;
}

void handleCLO(uint32_t opcode) {
    reg[RD()] = clz32(~reg[RS()]);
    pc += 4;
}

void handleMULT(uint32_t opcode) {
    int64_t result = (int64_t)(int32_t)reg[RS()] * (int64_t)(int32_t)reg[RT()];
    hi = (uint32_t)(result >> 32);
    lo = (uint32_t)result;
    pc += 4;
}

void handleMULTU(uint32_t opcode) {
    uint64_t result = (uint64_t)reg[RS()] * (uint64_t)reg[RT()];
    hi = (uint32_t)(result >> 32uLL);
    lo = (uint32_t)result;
    pc += 4;
}

void handleMADD(uint32_t opcode) {
    int64_t resultHiLo = ((uint64_t)hi << 32) | (uint64_t)lo;
    int64_t resultMultiply = (int64_t)(int32_t)reg[RS()] * (int64_t)(int32_t)reg[RT()];
    uint64_t result = resultHiLo + resultMultiply;

    hi = (uint32_t)(result >> 32uLL);
    lo = (uint32_t)result;
    pc += 4;
}

void handleMADDU(uint32_t opcode) {
    uint64_t resultHiLo = ((uint64_t)hi << 32) | (uint64_t)lo;
    uint64_t resultMultiply = (uint64_t)reg[RS()] * (uint64_t)reg[RT()];
    uint64_t result = resultHiLo + resultMultiply;

    hi = (uint32_t)(result >> 32uLL);
    lo = (uint32_t)result;
    pc += 4;
}

void handleMSUB(uint32_t opcode) {
    int64_t resultHiLo = ((uint64_t)hi << 32) | (uint64_t)lo;
    int64_t resultMultiply = (int64_t)(int32_t)reg[RS()] * (int64_t)(int32_t)reg[RT()];
    uint64_t result = resultHiLo - resultMultiply;

    hi = (uint32_t)(result >> 32uLL);
    lo = (uint32_t)result;
    pc += 4;
}

void handleMSUBU(uint32_t opcode) {
    uint64_t resultHiLo = ((uint64_t)hi << 32) | (uint64_t)lo;
    uint64_t resultMultiply = (uint64_t)reg[RS()] * (uint64_t)reg[RT()];
    uint64_t result = resultHiLo - resultMultiply;

    hi = (uint32_t)(result >> 32uLL);
    lo = (uint32_t)result;
    pc += 4;
}

void handleDIV(uint32_t opcode) {
    int rs = RS();
    int rt = RT();

    int32_t a = (int32_t)reg[rs];
    int32_t b = (int32_t)reg[rt];
    if (a == (int32_t)0x80000000 && b == -1) {
        lo = 0x80000000;
        hi = 0xFFFFFFFF;
    } else if (b != 0) {
        lo = (uint32_t)(a / b);
        hi = (uint32_t)(a % b);
    } else {
        lo = a < 0 ? 1 : -1;
        hi = a;
    }

    pc += 4;
}

void handleDIVU(uint32_t opcode) {
    int rs = RS();
    int rt = RT();

    uint32_t a = (int32_t)reg[rs];
    uint32_t b = (int32_t)reg[rt];

    if (b != 0) {
        lo = (a / b);
        hi = (a % b);
    } else {
        lo = a <= 0xFFFF ? 0xFFFF : -1;
        hi = a;
    }

    pc += 4;
}

void handleADD(uint32_t opcode) {
    reg[RD()] = reg[RS()] + reg[RT()];
    pc += 4;
}

void handleADDU(uint32_t opcode) {
    reg[RD()] = reg[RS()] + reg[RT()];
    pc += 4;
}

void handleSUB(uint32_t opcode) {
    reg[RD()] = reg[RS()] - reg[RT()];
    pc += 4;
}

void handleSUBU(uint32_t opcode) {
    reg[RD()] = reg[RS()] - reg[RT()];
    pc += 4;
}

void handleAND(uint32_t opcode) {
    reg[RD()] = reg[RS()] & reg[RT()];
    pc += 4;
}

void handleOR(uint32_t opcode) {
    reg[RD()] = reg[RS()] | reg[RT()];
    pc += 4;
}

void handleXOR(uint32_t opcode) {
    reg[RD()] = reg[RS()] ^ reg[RT()];
    pc += 4;
}

void handleNOR(uint32_t opcode) {
    reg[RD()] = ~(reg[RS()] | reg[RT()]);
    pc += 4;
}

void handleSLT(uint32_t opcode) {
    reg[RD()] = int32_t(reg[RS()]) < int32_t(reg[RT()]);
    pc += 4;
}

void handleSLTU(uint32_t opcode) {
    reg[RD()] = reg[RS()] < reg[RT()];
    pc += 4;
}

void handleMAX(uint32_t opcode) {
    const int32_t &a = reg[RS()], &b = reg[RT()];
    reg[RD()] = a > b ? a : b;
    pc += 4;
}

void handleMIN(uint32_t opcode) {
    const int32_t& a = reg[RS()], &b = reg[RT()];
    reg[RD()] = a < b ? a : b;
    pc += 4;
}

void handleJ(uint32_t opcode) {
    uint32_t target = TARGET();
    uint32_t jumpAddress = (pc & 0xF000'0000) | (target << 2);
    cpu.jal(jumpAddress);
}

void handleJAL(uint32_t opcode) {
    uint32_t target = TARGET();
    uint32_t jumpAddress = (pc & 0xF000'0000) | (target << 2);
    reg[31] = pc + 8;
    cpu.jal(jumpAddress);
}

void handleBranch(uint32_t opcode) {
    int op = OPCODE();

    int rs = RS();
    int rt = RT();

    int offset = (signed short)(opcode & 0xFFFF) << 2;
    uint32_t branchAddress = pc + (int32_t)offset + 4;

    switch (op) {
    case 0x01:
    {
        int branchOp = (opcode >> 16) & 0x1F;
        switch (branchOp) {
        case 0x10: reg[MIPS_REG_RA] = pc + 8; [[fallthrough]]; // bltzal
        case 0x00: if ((int32_t)reg[rs] <  0) cpu.jump(branchAddress); else pc += 4; break;//bltz

        case 0x11: reg[MIPS_REG_RA] = pc + 8; [[fallthrough]]; // bgezal
        case 0x01: if ((int32_t)reg[rs] >= 0) cpu.jump(branchAddress); else pc += 4; break;//bgez

        case 0x12: reg[MIPS_REG_RA] = pc + 8; [[fallthrough]]; // bltzall
        case 0x02: if ((int32_t)reg[rs] < 0)  cpu.jump(branchAddress); else pc += 8; break;//bltzl

        case 0x13: reg[MIPS_REG_RA] = pc + 8; [[fallthrough]]; // bgezall
        case 0x03: if ((int32_t)reg[rs] >= 0) cpu.jump(branchAddress); else pc += 8; break;//bgezl
        default:
            LOG_ERROR(logType, "unknown branch opcode op: 0x01 branch: 0x%02x", branchOp);
            setProcessorFailed(true);
        }
        break;
    }
    case 0x04: if (reg[rs] == reg[rt])    cpu.jump(branchAddress); else pc += 4; break; // beq
    case 0x05: if (reg[rs] != reg[rt])    cpu.jump(branchAddress); else pc += 4; break; // bne
    case 0x06: if ((int32_t)reg[rs] <= 0) cpu.jump(branchAddress); else pc += 4; break; // blez
    case 0x07: if ((int32_t)reg[rs] > 0)  cpu.jump(branchAddress); else pc += 4; break; // bgtz
    case 0x14: if (reg[rs] == reg[rt])    cpu.jump(branchAddress); else pc += 8; break; // beql
    case 0x15: if (reg[rs] != reg[rt])    cpu.jump(branchAddress); else pc += 8; break; // bnel
    case 0x16: if ((int32_t)reg[rs] <= 0) cpu.jump(branchAddress); else pc += 8; break; // blezl
    case 0x17: if ((int32_t)reg[rs] > 0)  cpu.jump(branchAddress); else pc += 8; break; // bgtzl
    default:
        LOG_ERROR(logType, "unknown branch opcode 0x%02x", op);
        setProcessorFailed(true);
    }
}

void handleADDIU(uint32_t opcode) {
    reg[RT()] = reg[RS()] + (int32_t)IMM();
    pc += 4;
}

void handleSLTI(uint32_t opcode) {
    reg[RT()] = (int32_t)reg[RS()] < (int32_t)IMM();
    pc += 4;
}

void handleSLTIU(uint32_t opcode) {
    reg[RT()] = reg[RS()] < IMM2();
    pc += 4;
}

void handleANDI(uint32_t opcode) {
    reg[RT()] = reg[RS()] & IMM2();
    pc += 4;
}

void handleORI(uint32_t opcode) {
    reg[RT()] = reg[RS()] | IMM2();
    pc += 4;
}

void handleXORI(uint32_t opcode) {
    reg[RT()] = reg[RS()] ^ IMM2();
    pc += 4;
}

void handleLUI(uint32_t opcode) {
    reg[RT()] = IMM2() << 16;
    pc += 4;
}

union FP32 {
    uint32_t u;
    float a;
};

struct FP16 {
    uint16_t u;
};

inline bool my_isinf(float a) {
    FP32 f2u;
    f2u.a = a;
    return f2u.u == 0x7f800000 ||
        f2u.u == 0xff800000;
}

inline bool my_isnan(float a) {
    FP32 f2u;
    f2u.a = a;
    // NaNs have non-zero mantissa
    return ((f2u.u & 0x7F800000) == 0x7F800000) && (f2u.u & 0x7FFFFF);
}

inline bool my_isnanorinf(float a) {
    FP32 f2u;
    f2u.a = a;
    // NaNs have non-zero mantissa, infs have zero mantissa. That is, we just ignore the mantissa here.
    return ((f2u.u & 0x7F800000) == 0x7F800000);
}

void handleCOP1(uint32_t opcode) {
    uint8_t cop1Opcode = RS();

    switch (cop1Opcode) {
    case 0x00: // mfc1
        reg[RT()] = fi[FS()];
        pc += 4;
        break;
    case 0x02: // cfc1
        if (FS() == 31) {
            cpu.fcr31 = (cpu.fcr31 & ~(1 << 23)) | ((cpu.fpcond & 1) << 23);
            reg[RT()] = cpu.fcr31;
        } else if (FS() == 0) {
            reg[RT()] = 0x00003351;
        }
        pc += 4;
        break;
    case 0x04: // mtc1
        fi[FS()] = reg[RT()];
        pc += 4;
        break;
    case 0x06: // ctc1
    {
        uint32_t value = reg[RT()];
        if (FS() == 31) {
            cpu.fcr31 = value & 0x0181FFFF;
            cpu.fpcond = (value >> 23) & 1;
        }
        pc += 4;
        break;
    }
    case 0x08: // cop1bc
    {
        int offset = (signed short)(opcode & 0xFFFF) << 2;
        uint32_t branchAddress = pc + offset + 4;
        switch (int op = (opcode >> 16) & 0x1F) {
        case 0: // bc1f
            if (!cpu.fpcond) {
                cpu.jump(branchAddress);
            } else {
                pc += 4;
            }
            break;
        case 1: // bc1t
            if (cpu.fpcond) {
                cpu.jump(branchAddress);
            } else {
                pc += 4;
            }
            break;
        case 2: // bc1fl
            if (!cpu.fpcond) {
                cpu.jump(branchAddress);
            } else {
                pc += 8;
            }
            break;
        case 3: // bc1tl
            if (cpu.fpcond) {
                cpu.jump(branchAddress);
            } else {
                pc += 8;
            }
            break;
        default:
            LOG_ERROR(logType, "unknown floating-point branch 0x%02x\n", op);
            setProcessorFailed(true);
        }
        break;
    }
    case 0x10: // cop1s
        switch (FUNC()) {
            case 0x00: // add.s
                f[FD()] = f[FS()] + f[FT()];
                pc += 4;
                break;
            case 0x01: // sub.s
                f[FD()] = f[FS()] - f[FT()];
                pc += 4;
                break;
            case 0x02: // mul.s
                if ((my_isinf(f[FS()]) && f[FT()] == 0.0f) || (my_isinf(f[FT()]) && f[FS()] == 0.0f)) {
                    fi[FD()] = 0x7fc00000;
                } else {
                    f[FD()] = f[FS()] * f[FT()];
                }
                pc += 4;
                break;
            case 0x03: // div.s
                f[FD()] = f[FS()] / f[FT()];
                pc += 4;
                break;
            case 0x04: // sqrt.s
                f[FD()] = sqrtf(f[FS()]);
                pc += 4;
                break;
            case 0x05: // abs.s
                f[FD()] = fabsf(f[FS()]);
                pc += 4;
                break;
            case 0x06: // mov.s
                f[FD()] = f[FS()];
                pc += 4;
                break;
            case 0x07: // neg.s
                f[FD()] = -f[FS()];
                pc += 4;
                break;
            case 12:
            case 13:
            case 14:
            case 15:
                if (my_isnanorinf(f[FS()]))
                {
                    fi[FD()] = my_isinf(f[FS()]) && f[FS()] < 0.0f ? -2147483648LL : 2147483647LL;
                    pc += 4;
                    break;
                }

                switch (FUNC())
                {
                case 12: fi[FD()] = (int)floorf(f[FS()] + 0.5f); break; //round.w.s
                case 13: //trunc.w.s
                    if (f[FS()] >= 0.0f) {
                        fi[FD()] = (int)floorf(f[FS()]);
                        // Overflow, but it was positive.
                        if (fi[FD()] == -2147483648LL) {
                            fi[FD()] = 2147483647LL;
                        }
                    } else {
                        fi[FD()] = (int)ceilf(f[FS()]);
                    }
                    break;
                case 14: fi[FD()] = (int)ceilf(f[FS()]); break; //ceil.w.s
                case 15: fi[FD()] = (int)floorf(f[FS()]); break; //floor.w.s
                }

                pc += 4;
                break;

            case 36:
                if (my_isnanorinf(f[FS()]))
                {
                    fi[FD()] = my_isinf(f[FS()]) && f[FS()] < 0.0f ? -2147483648LL : 2147483647LL;
                    pc += 4;
                    break;
                }

                switch (cpu.fcr31 & 3) {
                case 0: fi[FD()] = (int)std::round(f[FS()]); break;  // RINT_0
                case 1: fi[FD()] = (int)f[FS()]; break;  // CAST_1
                case 2: fi[FD()] = (int)ceilf(f[FS()]); break;  // CEIL_2
                case 3: fi[FD()] = (int)floorf(f[FS()]); break;  // FLOOR_3
                }

                pc += 4;
                break; // cvt.w.s
            case 0x30:
            case 0x31:
            case 0x32:
            case 0x33:
            case 0x34:
            case 0x35:
            case 0x36:
            case 0x37:
            case 0x38:
            case 0x39:
            case 0x3a:
            case 0x3b:
            case 0x3c:
            case 0x3d:
            case 0x3e:
            case 0x3f:
            {
                int fs = FS();
                int ft = FT();
                bool cond;
                switch (opcode & 0xf)
                {
                case 0: //f
                case 8: //sf
                    cond = false;
                    break;

                case 1: //un
                case 9: //ngle
                    cond = my_isnan(f[fs]) || my_isnan(f[ft]);
                    break;

                case 2: //eq
                case 10: //seq
                    cond = !my_isnan(f[fs]) && !my_isnan(f[ft]) && (f[fs] == f[ft]);
                    break;

                case 3: //ueq
                case 11: //ngl
                    cond = (f[fs] == f[ft]) || my_isnan(f[fs]) || my_isnan(f[ft]);
                    break;

                case 4: //olt
                case 12: //lt
                    cond = (f[fs] < f[ft]);
                    break;

                case 5: //ult
                case 13: //nge
                    cond = (f[fs] < f[ft]) || my_isnan(f[fs]) || my_isnan(f[ft]);
                    break;

                case 6: //ole
                case 14: //le
                    cond = (f[fs] <= f[ft]);
                    break;

                case 7: //ule
                case 15: //ngt
                    cond = (f[fs] <= f[ft]) || my_isnan(f[fs]) || my_isnan(f[ft]);
                    break;

                default:
                    cond = false;
                    break;
                }

                cpu.fpcond = cond;
                pc += 4;
                break;
            }
        default:
            LOG_ERROR(logType, "unknown cop1.s opcode 0x%02x PC 0x%08x\n", FUNC(), pc);
            setProcessorFailed(true);
        }
        break;
    case 0x14: // cop1w
        switch (FUNC()) {
        case 0x20:
        {
            f[FD()] = (float) cpu.fs[FS()];
            pc += 4;
            break;
        }
        default:
            LOG_ERROR(logType, "unknown cop1.w opcode 0x%02x PC %x\n", FUNC(), pc);
            setProcessorFailed(true);
        }
        break;
    default:
        LOG_ERROR(logType, "unknown cop1 opcode 0x%02x PC 0x%08x\n", cop1Opcode, pc);
        setProcessorFailed(true);
    }
}

void handleSPC3(uint32_t opcode) {
    int func = FUNC();

    int rd = RD();
    int rt = RT();
    int pos = POS();

    auto _Allegrex2Handler = [rd, rt](const uint32_t& opcode) -> void {
        int allegrexOpcode = opcode & 0x3FF;

        auto __swap32 = [](const uint32_t& r) {
            return ((r >> 24) & 0xFF) | ((r << 8) & 0xFF0000) | ((r >> 8) & 0xFF00) | ((r << 24) & 0xFF000000);
        };

        switch (allegrexOpcode) {
        case 0xA0: // wsbh
            reg[rd] = ((reg[rt] & 0xFF00FF00) >> 8) | ((reg[rt] & 0x00FF00FF) << 8);
            break;
        case 0xE0: // wsbw
            reg[rd] = __swap32(reg[rt]);
            break;
        default:
            LOG_ERROR(logType, "can't interpret ALLEGREX2 opcode 0x%02x PC 0x%08x", allegrexOpcode, pc);
            setProcessorFailed(true);
        }
    };

    switch (func) {
    case 0x0: //ext
    {
        int size = SIZE() + 1;
        uint32_t sourcemask = 0xFFFFFFFFUL >> (32 - size);
        reg[rt] = (reg[RS()] >> pos) & sourcemask;
        break;
    }
    case 0x4: // ins
    {
        int size = (SIZE() + 1) - pos;
        uint32_t sourcemask = 0xFFFFFFFFUL >> (32 - size);
        uint32_t destmask = sourcemask << pos;
        reg[rt] = (reg[rt] & ~destmask) | ((reg[RS()] & sourcemask) << pos);
        break;
    }
    case 0x20:
    {
        int allegrexOpcode = (opcode >> 6) & 0x1F;
        
        switch (allegrexOpcode) {
        case 0x02: _Allegrex2Handler(opcode); break;
        case 0x03: _Allegrex2Handler(opcode); break;
        case 0x10: reg[rd] = (int)(int8_t)reg[rt]; break; // seb
        case 0x18: reg[rd] = (int)(int16_t)reg[rt]; break; // seh
        case 0x14: // bitrev
        {
            uint32_t temp = 0;

            for (int i = 0; i < 32; i++) {
                if (reg[rt] & (1 << i)) {
                    temp |= (0x80000000 >> i);
                }
            }

            reg[rd] = temp;
            break;
        }
        default:
            LOG_ERROR(logType, "can't interpret ALLEGREX opcode 0x%02x PC 0x%08x", allegrexOpcode, pc);
            setProcessorFailed(true);
        }
        break;
    }
    default:
        LOG_ERROR(logType, "can't interpret SPECIAL3 opcode 0x%02x PC 0x%08x", func, pc);
        setProcessorFailed(true);
    }

    pc += 4;
}

void handleMemoryAccess(uint32_t opcode) {
    void *addressPtr;
    int op = OPCODE();

    int offset = (signed short)(opcode & 0xFFFF);
    int rs = RS(), rt = RT();

    uint32_t address = reg[rs] + offset;
    uint32_t& value = reg[rt];

    addressPtr = Core::Memory::getPointerUnchecked(address);

    if (!addressPtr) {
        debugAllegrexState();
        LOG_ERROR(logType, "invalid memory access opcode 0x%02x! got address 0x%08x @ PC 0x%08x %s", op, address, pc, Core::Kernel::current->name);
        setProcessorFailed(true);
        pc += 4;
        uint32_t x = 0;
        addressPtr = &x;
        return;
    }

    // static int ctr;

    switch (op) {
    case 0x20: value = *(int8_t *) addressPtr; break; // lb
    case 0x21: value = *(int16_t *)addressPtr; break; // lh
    case 0x23: value = *(uint32_t *)addressPtr; break; // lw
    case 0x24: value = *(uint8_t *)addressPtr; break; // lbu
    case 0x25: value = *(uint16_t *)addressPtr; break; // lhu
    case 0x28: *(uint8_t *)addressPtr = value & 0xFF; break; // sb
    case 0x29: *(uint16_t *)addressPtr = value & 0xFFFF; break; // sh
    case 0x2B: *(uint32_t *)addressPtr = value; break; // sw
    case 0x22: case 0x26: case 0x2A: case 0x2E:
    {
        uint32_t shift = (address & 3) * 8;
        uint32_t mem = Core::Memory::read32(address & 0xfffffffc);

        switch (op) {
        case 0x22: //lwl
        {
            uint32_t result = (reg[rt] & (0x00ffffff >> shift)) | (mem << (24 - shift));
            reg[rt] = result;
            break;
        }
        case 0x26: //lwr
        {
            uint32_t regval = reg[rt];
            uint32_t result = (regval & (0xffffff00 << (24 - shift))) | (mem >> shift);
            reg[rt] = result;
            break;
        }
        case 0x2A: //swl
        {
            uint32_t result = ((reg[rt] >> (24 - shift))) | (mem & (0xffffff00 << shift));
            Core::Memory::write32(address & 0xfffffffc, result);
            break;
        }
        case 0x2E: //swr
        {
            uint32_t result = ((reg[rt] << shift) | (mem & (0x00ffffff >> (24 - shift))));
            Core::Memory::write32(address & 0xfffffffc, result);
            break;
        }
        }
        break;
    }
    case 0x31: fi[FT()] = *(uint32_t *)addressPtr; break;
    case 0x39: *(uint32_t *)addressPtr = fi[FT()]; break;
    default:
        LOG_ERROR(logType, "unknown memory access opcode 0x%02x", op);
        setProcessorFailed(true);
        return;
    }

    pc += 4;
}

void handleCACHE(uint32_t opcode) {
    pc += 4;
}

bool interpret(uint32_t opcode) {
    int op = OPCODE();

    switch (op) {
    case 0x00:
    {
        int func = FUNC();
        switch (func) {
        case 0x00: handleSLL(opcode); break;
        case 0x02: handleSRL(opcode); break;
        case 0x03: handleSRA(opcode); break;
        case 0x04: handleSLLV(opcode); break;
        case 0x06: handleSRLV(opcode); break;
        case 0x07: handleSRAV(opcode); break;
        case 0x08: handleJR(opcode); break;
        case 0x09: handleJALR(opcode); break;
        case 0x0A: handleMOVZ(opcode); break;
        case 0x0B: handleMOVN(opcode); break;
        case 0x0C: handleSYSCALL(opcode); break;
        case 0x0F: pc += 4; break;
        case 0x10: handleMFHI(opcode); break;
        case 0x11: handleMTHI(opcode); break;
        case 0x12: handleMFLO(opcode); break;
        case 0x13: handleMTLO(opcode); break;
        case 0x16: handleCLZ(opcode); break;
        case 0x17: handleCLO(opcode); break;
        case 0x18: handleMULT(opcode); break;
        case 0x19: handleMULTU(opcode); break;
        case 0x1C: handleMADD(opcode); break;
        case 0x1D: handleMADDU(opcode); break;
        case 0x1A: handleDIV(opcode); break;
        case 0x1B: handleDIVU(opcode); break;
        case 0x20: handleADDU(opcode); break;
        case 0x21: handleADDU(opcode); break;
        case 0x22: handleSUB(opcode); break;
        case 0x23: handleSUBU(opcode); break;
        case 0x24: handleAND(opcode); break;
        case 0x25: handleOR(opcode); break;
        case 0x26: handleXOR(opcode); break;
        case 0x27: handleNOR(opcode); break;
        case 0x2A: handleSLT(opcode); break;
        case 0x2B: handleSLTU(opcode); break;
        case 0x2C: handleMAX(opcode); break;
        case 0x2D: handleMIN(opcode); break;
        case 0x2E: handleMSUB(opcode); break;
        case 0x2F: handleMSUBU(opcode); break;
        default:
            debugAllegrexState();
            LOG_ERROR(logType, "unknown functor 0x%02x @ 0x%08x", func, pc);
            return false;
        }
        break;
    }
    case 0x01: handleBranch(opcode); break;
    case 0x02: handleJ(opcode); break;
    case 0x03: handleJAL(opcode); break;
    case 0x04: handleBranch(opcode); break;
    case 0x05: handleBranch(opcode); break;
    case 0x06: handleBranch(opcode); break;
    case 0x07: handleBranch(opcode); break;
    case 0x09: handleADDIU(opcode); break;
    case 0x0A: handleSLTI(opcode); break;
    case 0x0B: handleSLTIU(opcode); break;
    case 0x0C: handleANDI(opcode); break;
    case 0x0D: handleORI(opcode); break;
    case 0x0E: handleXORI(opcode); break;
    case 0x0F: handleLUI(opcode); break;
    case 0x11: handleCOP1(opcode); break;
    case 0x14: handleBranch(opcode); break;
    case 0x15: handleBranch(opcode); break;
    case 0x1C: pc += 4; break; // unimpl
    case 0x16: handleBranch(opcode); break;
    case 0x17: handleBranch(opcode); break;
    case 0x1F: handleSPC3(opcode); break;
    case 0x20: handleMemoryAccess(opcode); break;
    case 0x21: handleMemoryAccess(opcode); break;
    case 0x22: case 0x26: case 0x2A: case 0x2E: handleMemoryAccess(opcode); break;
    case 0x23: handleMemoryAccess(opcode); break;
    case 0x24: handleMemoryAccess(opcode); break;
    case 0x25: handleMemoryAccess(opcode); break;
    case 0x28: handleMemoryAccess(opcode); break;
    case 0x29: handleMemoryAccess(opcode); break;
    case 0x2B: handleMemoryAccess(opcode); break;
    case 0x2F: handleCACHE(opcode); break;
    case 0x31: handleMemoryAccess(opcode); break;
    case 0x39: handleMemoryAccess(opcode); break;
    // VFPU
    case 0x12: _executeVFPU(opcode); break;
    case 0x18: _executeVFPU(opcode); break;
    case 0x19: _executeVFPU(opcode); break;
    case 0x1B: _executeVFPU(opcode); break;
    case 0x32: _executeVFPU(opcode); break;
    case 0x34: _executeVFPU(opcode); break;
    case 0x36: _executeVFPU(opcode); break;
    case 0x37: _executeVFPU(opcode); break;
    case 0x3A: _executeVFPU(opcode); break;
    case 0x3C: _executeVFPU(opcode); break;
    case 0x3E: _executeVFPU(opcode); break;
    case 0x3F: pc += 4; break;
    default:
        debugAllegrexState();
        LOG_ERROR(logType, "unknown opcode 0x%02x @ 0x%08x", op, pc);
        return false;
    }
    return true;
}
}