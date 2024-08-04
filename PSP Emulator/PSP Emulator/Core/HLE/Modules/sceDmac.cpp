#include <Core/HLE/Modules/sceDmac.h>

#include <Core/Memory/MemoryAccess.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

namespace Core::HLE {
static const char *logType = "sceDmac";

static int __dmacMemcpy(uint32_t dst, uint32_t src, SceSize n) {
    Core::Timing::consumeCycles(2000);
    Memory::Utility::copyMemoryGuestToGuest(dst, src, n);
    LOG_WARN(logType, "__dmacMemcpy(dst: 0x%08x, src: 0x%08x, size: 0x%08x)", dst, src, n);
    return 0;
}

int sceDmacMemcpy(uint32_t dst, uint32_t src, SceSize n) {
    return __dmacMemcpy(dst, src, n);
}

int sceDmacTryMemcpy(uint32_t dst, uint32_t src, SceSize n) {
    return __dmacMemcpy(dst, src, n);
}
}