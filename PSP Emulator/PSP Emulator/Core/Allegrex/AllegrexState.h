#pragma once

#include <cstdint>

#include <Core/Allegrex/CPURegisterName.h>

namespace Core::Kernel { struct PSPThread; }

namespace Core::Allegrex {
#pragma pack(push, 1)
struct AllegrexState {
    uint32_t pc;
    uint32_t reg[32];

    union {
        float f[32];
        uint32_t fi[32];
        int fs[32];
    };

    union {
        float vprf[128];
        uint32_t vpr[128];
    };

    uint32_t lo;
    uint32_t hi;
    uint32_t fcr31;
    uint32_t fpcond;
    uint32_t vfpuCtrl[16];
    uint32_t branchPC;
    bool llBit;
    bool requiredBranching;
    bool requiredJumping;
    int64_t cyclesDowncount;

    uint8_t VfpuWriteMask() const {
        return (vfpuCtrl[VFPU_CTRL_DPREFIX] >> 8) & 0xF;
    }

    bool VfpuWriteMask(int i) const {
        return (vfpuCtrl[VFPU_CTRL_DPREFIX] >> (8 + i)) & 1;
    }


    void setGPR(int index, uint32_t value);
    void jal(uint32_t address);
    void jr(uint32_t address);
    void jump(uint32_t address);
    uint32_t getPC() { return pc; }
    void setPC(uint32_t address) { pc = address; }
};

#pragma pack(pop)

extern AllegrexState cpu;

void initState();
void resetState();
void debugAllegrexState();
void loadState(Core::Kernel::PSPThread *thread);
void saveCurrentState();
bool isProcessorInDelaySlot();
}