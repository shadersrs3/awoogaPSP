#include <Core/Allegrex/AllegrexState.h>
#include <Core/Allegrex/AllegrexSyscallHandler.h>

#include <Core/Kernel/sceKernelThread.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/HLE/CPUAssembler.h>
#include <Core/HLE/CustomSyscall.h>

#include <Core/Logger.h>

using namespace Core::Kernel;

extern bool kernelTrace;

namespace Core::Allegrex {
static const char *logType = "AllegrexState";
AllegrexState cpu;

void AllegrexState::setGPR(int index, uint32_t value) {
    if (index != 0)
        reg[index] = value;
}

void AllegrexState::jal(uint32_t address) {
    branchPC = address;
    pc += 4;
    requiredJumping = true;
}

void AllegrexState::jr(uint32_t address) {
    branchPC = address;
    pc += 4;
    requiredJumping = true;
}

void AllegrexState::jump(uint32_t address) {
    branchPC = address;
    pc += 4;
    requiredBranching = true;
}

static const char *gprName[] = {
    "ze", "at", "v0", "v1", "a0", "a1", "a2", "a3", "t0", "t1", "t2", "t3", "t4", "t5", "t6", "t7",
    "s0", "s1", "s2", "s3", "s4", "s5", "s6", "s7", "t8", "t9", "k0", "k1", "gp", "sp", "fp", "ra"
};

static const char *vfpuRegName[] = {
    "s000", "s001", "s002", "s003", "s010", "s011", "s012", "s013", "s020", "s021", "s022", "s023", "s030", "s031", "s032", "s033",
    "s100", "s101", "s102", "s103", "s110", "s111", "s112", "s113", "s120", "s121", "s122", "s123", "s130", "s131", "s132", "s133",
    "s200", "s201", "s202", "s203", "s210", "s211", "s212", "s213", "s220", "s221", "s222", "s223", "s230", "s231", "s232", "s233",
    "s300", "s301", "s302", "s303", "s310", "s311", "s312", "s313", "s320", "s321", "s322", "s323", "s330", "s331", "s332", "s333",
    "s400", "s401", "s402", "s403", "s410", "s411", "s412", "s413", "s420", "s421", "s422", "s423", "s430", "s431", "s432", "s433",
    "s500", "s501", "s502", "s503", "s510", "s511", "s512", "s513", "s520", "s521", "s522", "s523", "s530", "s531", "s532", "s533",
    "s600", "s601", "s602", "s603", "s610", "s611", "s612", "s613", "s620", "s621", "s622", "s623", "s630", "s631", "s632", "s633",
    "s700", "s701", "s702", "s703", "s710", "s711", "s712", "s713", "s720", "s721", "s722", "s723", "s730", "s731", "s732", "s733",
};

static void debugFPU() {
    LOG_DEBUG(logType, "FPU registers dump trace");

    for (int i = 0; i < 32; i++)
        printf("f%d: 0x%08X (%f)\n", i, cpu.fi[i], cpu.f[i]);
}

static void debugVFPU() {
    LOG_DEBUG(logType, "VFPU registers dump trace");

    for (int i = 0; i < 8; i++) {
        int mb = i * 0x10;
        for (int j = 0; j < 4; j++) {
            int cb = mb + j * 4;
            printf("%s: ", vfpuRegName[cb]);
            for (int k = 0; k < 4; k++) {
                int rb = cb + k;
                printf("0x%08x ", cpu.vpr[rb]);
            }
            printf("\n");
        }
        printf("\n");
    }
}

static void debugGPR() {
    LOG_TRACE(logType, "GPR dump trace");

    for (int i = 0; i < 32; i++)
        LOG_TRACE(logType, "%s: 0x%08X", gprName[i], cpu.reg[i]);

    LOG_TRACE(logType, "lo: 0x%08x hi:0x%08x", cpu.lo, cpu.hi);
}

void debugAllegrexState() {
    debugFPU();
    debugVFPU();
    
    LOG_DEBUG(logType, "stack frame bottom to top %d", current->stackFrameIndex);
    for (int i = 0; i < current->stackFrameIndex; i++) {
        LOG_DEBUG(logType, "%d 0x%08x", i, current->stackFrame[i]);
    }

    debugGPR();
    LOG_TRACE(logType, "pc 0x%08x thread (%s)\n", cpu.pc, Core::Kernel::current->name);
}

void initState() {
    cpu.branchPC = 0;
    cpu.requiredBranching = false;

    resetSyscallAddressHandler();

    uint32_t address = Core::Kernel::hleGetReturnFromAddress(Core::Kernel::THREAD_RETURN_ADDRESS_IDLE);
    Core::HLE::CustomNID nid = Core::HLE::CUSTOM_NID_IDLE;
    Core::Memory::write32(address, Core::HLE::Assembly::makeJumpReturnAddress());
    Core::Memory::write32(address + 0x4, Core::HLE::Assembly::makeSyscallInstruction(nid));
    setSyscallAddressHandler(address + 0x4, nid);

    address = Core::Kernel::hleGetReturnFromAddress(Core::Kernel::THREAD_RETURN_ADDRESS_RETURN_FROM_THREAD);
    nid = Core::HLE::CUSTOM_NID_RETURN_FROM_THREAD;
    Core::Memory::write32(address, Core::HLE::Assembly::makeJumpReturnAddress());
    Core::Memory::write32(address + 0x4, Core::HLE::Assembly::makeSyscallInstruction(nid));
    setSyscallAddressHandler(address + 0x4, nid);

    address = Core::Kernel::hleGetReturnFromAddress(Core::Kernel::THREAD_RETURN_ADDRESS_RETURN_FROM_CALLBACK);
    nid = Core::HLE::CUSTOM_NID_RETURN_FROM_CALLBACK;
    Core::Memory::write32(address, Core::HLE::Assembly::makeJumpReturnAddress());
    Core::Memory::write32(address + 0x4, Core::HLE::Assembly::makeSyscallInstruction(nid));
    setSyscallAddressHandler(address + 0x4, nid);

    address = Core::Kernel::hleGetReturnFromAddress(Core::Kernel::THREAD_RETURN_ADDRESS_RETURN_FROM_INTERRUPT);
    nid = Core::HLE::CUSTOM_NID_RETURN_FROM_INTERRUPT;
    Core::Memory::write32(address, Core::HLE::Assembly::makeJumpReturnAddress());
    Core::Memory::write32(address + 0x4, Core::HLE::Assembly::makeSyscallInstruction(nid));
    setSyscallAddressHandler(address + 0x4, nid);
}

void resetState() {
    resetSyscallAddressHandler();
    cpu.requiredBranching = false;
}

void loadState(Core::Kernel::PSPThread *state) {
    const Core::Kernel::PSPThread::Context *ctx = &state->context;

    cpu.pc = ctx->pc;
    std::memcpy(cpu.reg, ctx->gpr, sizeof ctx->gpr);
    std::memcpy(cpu.fi, ctx->fpr, sizeof ctx->fpr);
    std::memcpy(cpu.vpr, ctx->vpr, sizeof ctx->vpr);
    std::memcpy(cpu.vfpuCtrl, ctx->vfpuCtrl, sizeof ctx->vfpuCtrl);

    cpu.lo = ctx->lo;
    cpu.hi = ctx->hi;
    cpu.fcr31 = ctx->fcr31;
    cpu.llBit = ctx->llBit;
    cpu.fpcond = ctx->fpcond;
}

void saveCurrentState() {
    Core::Kernel::PSPThread::Context *ctx = &Core::Kernel::current->context;

    ctx->pc = cpu.pc;
    std::memcpy(ctx->gpr, cpu.reg, sizeof ctx->gpr);
    std::memcpy(ctx->fpr, cpu.fi, sizeof ctx->fpr);
    std::memcpy(ctx->vpr, cpu.vpr, sizeof ctx->vpr);
    std::memcpy(ctx->vfpuCtrl, cpu.vfpuCtrl, sizeof ctx->vfpuCtrl);

    ctx->lo = cpu.lo;
    ctx->hi = cpu.hi;
    ctx->fcr31 = cpu.fcr31;
    ctx->llBit = cpu.llBit;
    ctx->fpcond = cpu.fpcond;
}

bool isProcessorInDelaySlot() {
    return cpu.requiredBranching;
}
}