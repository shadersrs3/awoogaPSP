#include <Core/HLE/CPUAssembler.h>

#include <Core/Logger.h>

namespace Core::HLE::Assembly {
static const char *logType = "cpuasm";

uint32_t makeJumpReturnAddress() {
    return 0x3e00008;
}

uint32_t makeBranch(uint16_t offset) {
    return 0x10000000 | offset;
}

uint32_t makeSyscallInstruction(uint32_t nid) {
    uint32_t syscallCode = 0x0;

    LOG_ERROR(logType, "can't craft NID 0x%08x to syscall (unknown NID)", nid);
    syscallCode &= 0xFFFFF;
    return (syscallCode << 6) | 0xC;
}

uint32_t makeNop() {
    return 0x0;
}
}