#include <Core/Allegrex/Allegrex.h>
#include <Core/Allegrex/AllegrexState.h>
#include <Core/Allegrex/AllegrexInterpreter.h>

#include <Core/Timing.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Kernel/sceKernelThread.h>

#include <Core/Logger.h>

#include <Core/Allegrex/CPURegisterName.h>

extern bool kernelTrace;

namespace Core::Allegrex {
static const char *logType = "Allegrex";
static bool processorFailed = false;

static bool debugMode = false;

static void debugModeSession();

void setDebugMode(bool state) {
    debugMode = true;
}

CyclesTaken interpreterBlockStep() {
    uint64_t cycles = 0;
    uint32_t *op = (uint32_t *) Core::Memory::getPointer(cpu.getPC());

    if (!op) {
        LOG_ERROR(logType, "invalid opcode from memory address 0x%08x!", cpu.getPC());
        processorFailed = true;
        return 0;
    }

    while (!processorFailed) {
        debugModeSession();
        bool requiredToBranch = cpu.requiredBranching == true;
        bool requiredToJump = cpu.requiredJumping == true;
        uint32_t npc = cpu.pc + 4;

        switch (cpu.pc) {
        case 0x8ae0c30:
            //Core::Memory::write32(cpu.pc, 0x0A2B8318);
            break;
        }

        bool interpretState = Core::Allegrex::interpret(*op++);
        if (!interpretState) {
            LOG_ERROR(logType, "can't interpret instruction 0x%08x @ 0x%08x", *op, cpu.getPC());
            processorFailed = true;
            return 0;
        }

        cpu.reg[0] = 0;

        cycles += 1;
        if (requiredToBranch) {
            cpu.pc = cpu.branchPC;
            cpu.requiredBranching = false;
            op = (uint32_t *) Core::Memory::getPointer(cpu.pc);
        } else if (requiredToJump) {
            cpu.pc = cpu.branchPC;
            cpu.requiredJumping = false;
            break;
        } else if (npc != cpu.pc)
            op = (uint32_t *) Core::Memory::getPointer(cpu.pc);
    }

    return cycles;
}

static void __tracePC(uint32_t pc, int callCount) {
    static int calls;
    static int counter;

    if (cpu.pc != pc)
        return;

    ++counter;

    if (counter == callCount) {
        debugAllegrexState();
        printf("called %d times\n", counter);
        setProcessorFailed(true);
    }
}

static void __traceAllRegisters(uint32_t registerValue, int callCount, uint32_t pc = -1) {
    static int counter;
    static int pcCounter;

    if (cpu.pc == pc) {
        ++pcCounter;
        // debugAllegrexState();
        // LOG_TRACE(logType, "pc call counter %d", pcCounter);
        // LOG_TRACE(logType, "----------\n");
    }

    static const char *gprName[] = {
        "ze", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
        "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
    };
    for (int i = 0; i < 32; i++) {
        if (cpu.reg[i] == registerValue) {
            if (++counter == callCount && (pc == -1 || cpu.pc == pc)) {
                debugAllegrexState();
                printf("called %d times register %s found PC %d times\n", counter, gprName[i], pcCounter);
                setProcessorFailed(true);
            }
        }
    }
}

static void readMemory32(uint32_t address) {
    printf("address 0x%08x memory data 0x%08x", address, Memory::read32(address));
}

static void traceRegister(int registerIndex, uint32_t registerValue, int callCount) {
    static int counter;
    if (registerValue == cpu.reg[registerIndex]) {
        if (++counter >= callCount) {
            debugAllegrexState();
            setProcessorFailed(true);
        }
    }
}

static void tracePC(uint32_t value, int count) {
    static int counter;

    if (cpu.pc == value) {
        if (++counter == count) {
            debugAllegrexState();
            setProcessorFailed(true);
        }
    }
}

static void debugModeSession() {
}

void setProcessorFailed(bool state) {
    processorFailed = state;
}

bool& isProcessorFailed() {
    return processorFailed;
}

bool interpreterStep() {
    bool requiredToJump;
    uint32_t *op;

    if (processorFailed)
        return false;

    op = (uint32_t *) Core::Memory::getPointer(cpu.getPC());

    if (!op) {
        LOG_ERROR(logType, "invalid opcode from memory address 0x%08x!", cpu.getPC());
        processorFailed = true;
        return false;
    }

    debugModeSession();

    requiredToJump = cpu.requiredBranching == true;
    bool interpretState = Core::Allegrex::interpret(*op);
    if (!interpretState) {
        LOG_ERROR(logType, "can't interpret instruction 0x%08x @ 0x%08x", *op, cpu.getPC());
        processorFailed = true;
        return false;
    }

    cpu.reg[0] = 0;

    if (requiredToJump) {
        cpu.setPC(cpu.branchPC);
        cpu.requiredBranching = false;
    }
    return true;
}
}