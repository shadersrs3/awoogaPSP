#include <Core/Memory/MemoryState.h>
#include <Core/PSP/MemoryMap.h>

#include <Core/Logger.h>

#if defined(_WIN32) || (_WIN64)
#   include <Windows.h>
#   include <memoryapi.h>
#elif defined (__linux__)
#   include <sys/mmap.h>
#endif

namespace Core::Memory {
static const char *logType = "MemoryState";

static bool initialized = false;

uint8_t *scratchpad;
uint8_t *userMemory;
uint8_t *kernelMemory;
uint8_t *videoMemory;

bool initialize() {
    static auto deallocateRoutine = []() {
        if (scratchpad) {
            deallocateMemory(scratchpad, Core::PSP::SCRATCHPAD_MEMORY_SIZE);
            scratchpad = nullptr;
        }

        if (userMemory) {
            deallocateMemory(userMemory, Core::PSP::USERSPACE_MEMORY_SIZE);
            userMemory = nullptr;
        }

        if (kernelMemory) {
            deallocateMemory(kernelMemory, Core::PSP::KERNELSPACE_MEMORY_SIZE);
            kernelMemory = nullptr;
        }
        
        if (videoMemory) {
            deallocateMemory(videoMemory, Core::PSP::VRAM_MEMORY_SIZE);
            videoMemory = nullptr;
        }
    };

    if (initialized) {
        LOG_ERROR(logType, "trying to initialize PSP memory again!");
        return false;
    }

    LOG_INFO(logType, "initializing PSP memory map...");

    scratchpad = reinterpret_cast<uint8_t *>(allocateMemory(Core::PSP::SCRATCHPAD_MEMORY_SIZE));
    if (!scratchpad) {
        deallocateRoutine();
        return false;
    }

    userMemory = reinterpret_cast<uint8_t *>(allocateMemory(Core::PSP::USERSPACE_MEMORY_SIZE));
    if (!userMemory) {
        deallocateRoutine();
        return false;
    }

    kernelMemory = reinterpret_cast<uint8_t *>(allocateMemory(Core::PSP::KERNELSPACE_MEMORY_SIZE));
    if (!kernelMemory) {
        deallocateRoutine();
        return false;
    }


    videoMemory = reinterpret_cast<uint8_t *>(allocateMemory(Core::PSP::VRAM_MEMORY_SIZE));
    if (!videoMemory) {
        deallocateRoutine();
        return false;
    }

    initialized = true;
    return true;
}

void reset() {
    if (!initialized) {
        LOG_ERROR(logType, "can't reset PSP memory while not initialized!");
        return;
    }

    LOG_INFO(logType, "resetting PSP memory map...");
    std::memset(scratchpad, 0x00, Core::PSP::SCRATCHPAD_MEMORY_SIZE);
    std::memset(userMemory, 0x00, Core::PSP::USERSPACE_MEMORY_SIZE);
    std::memset(kernelMemory, 0x00, Core::PSP::KERNELSPACE_MEMORY_SIZE);
    std::memset(videoMemory, 0x00, Core::PSP::VRAM_MEMORY_SIZE);
}

void destroy() {
    if (!initialized) {
        LOG_ERROR(logType, "can't destroy PSP memory while not initialized!");
        return;
    }

    deallocateMemory(scratchpad, Core::PSP::SCRATCHPAD_MEMORY_SIZE);
    deallocateMemory(userMemory, Core::PSP::USERSPACE_MEMORY_SIZE);
    deallocateMemory(kernelMemory, Core::PSP::KERNELSPACE_MEMORY_SIZE);
    deallocateMemory(videoMemory, Core::PSP::VRAM_MEMORY_SIZE);
    scratchpad = userMemory = kernelMemory = videoMemory = nullptr;
    LOG_INFO(logType, "destroying PSP memory map...");
    initialized = false;
}

static size_t roundMemorySizeToPage(size_t memorySize) {
    return ((memorySize + 0xFFF) & ~0xFFF);
}

void *allocateMemory(size_t memorySize, bool executable) {
    memorySize = roundMemorySizeToPage(memorySize);
    void *data;

    if (!memorySize) {
        LOG_ERROR(logType, "can't allocate memory if memory size is zero");
        return nullptr;
    }

#if defined (_WIN32) || defined (_WIN64)
    DWORD pageProtectionFlags;

    if (executable) {
        pageProtectionFlags = PAGE_EXECUTE_READWRITE;
    } else {
        pageProtectionFlags = PAGE_READWRITE;
    }

    data = VirtualAlloc(NULL, memorySize, MEM_COMMIT, pageProtectionFlags);
    if (data)
        std::memset(data, 0, memorySize);
#else
#   error Unknown platform Windows supported at the moment.
#endif
    return data;
}

void deallocateMemory(void *ptr, size_t memorySize) {
    memorySize = roundMemorySizeToPage(memorySize);

    if (!memorySize) {
        LOG_ERROR(logType, "can't deallocate memory if memory size is zero");
        return;
    }

#if defined (_WIN32) || defined (_WIN64)
    if (!VirtualFree(ptr, memorySize, MEM_DECOMMIT))
        LOG_ERROR(logType, "can't VirtualFree memory address %p size 0x%llx!", ptr, memorySize);
#else
#   error Unknown platform Windows supported at the moment.
#endif
}
}