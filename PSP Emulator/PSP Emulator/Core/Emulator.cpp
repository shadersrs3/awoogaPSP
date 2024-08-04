#include <Core/Emulator.h>
#include <Core/EmulatorConfig.h>

#include <Core/Allegrex/AllegrexState.h>
#include <Core/Memory/MemoryState.h>
#include <Core/Kernel/Objects/Module.h>
#include <Core/Kernel/sceKernelFactory.h>
#include <Core/Kernel/sceKernelThread.h>
#include <Core/Utility/RandomNumberGenerator.h>
#include <Core/Loaders/AbstractLoader.h>

#include <Core/Timing.h>

#include <Core/Logger.h>

#include <Core/GPU/GPU.h>

#include <Core/GPU/Renderer.h>

#include <chrono>
#include <thread>

namespace Core::Emulator {
static const char *logType = "emulator";

static uint64_t frameCounter;

static bool debugModeState = false;

static bool emulatorInitialized = false;

static std::string gamePath;

void setGamePath(const std::string& gamePath) { Core::Emulator::gamePath = gamePath; }
const std::string& getGamePath() { return gamePath; }

bool initialize() {
    if (emulatorInitialized == true) {
        LOG_ERROR(logType, "trying to initialize emulator twice");
        return false;
    }

    LOG_INFO(logType, "initializing emulator state...");

    // emulator state variable initialization
    emulatorInitialized = false;
    frameCounter = 0;

    // Random Number Generator has to be first
    Core::Utility::initRNG();
    LOG_INFO(logType, "created fixed random number generator for object uid creation");

    Core::GPU::initialize();

    if (!Core::Memory::initialize()) {
        LOG_ERROR(logType, "can't initialize emulator");
        return false;
    }

    // kernel initialization
    if (!Core::Kernel::initialize()) {
        LOG_ERROR(logType, "can't initialize kernel (check sceKernel logs)");
        return false;
    }

    Core::Allegrex::initState();
    LOG_SUCCESS(logType, "successfully initialized emulator state");
    emulatorInitialized = true;
    return emulatorInitialized;
}

bool reset() {
    if (emulatorInitialized == false) {
        LOG_ERROR(logType, "can't reset emulator while not initialized!");
        return false;
    }

    // resetting states
    frameCounter = 0;

    // resetting behaviours
    Core::GPU::reset();
    Core::Allegrex::resetState();
    Core::Utility::initRNG();
    Core::Kernel::reset();
    Core::Memory::reset();

    LOG_SUCCESS(logType, "resetted emulator state");
    return emulatorInitialized;
}

bool destroy() {
    bool destroyed = false;

    if (emulatorInitialized == false) {
        LOG_INFO(logType, "attempting to destroy emulator while not initialized");
        return false;
    }

    LOG_INFO(logType, "destroying emulator state...");

    Core::GPU::destroy();
    Core::Kernel::destroy();
    Core::Memory::destroy();

    if (Core::Loader::getGameLoaderHandle()) {
        delete Core::Loader::getGameLoaderHandle();
        Core::Loader::setGameLoaderHandle(nullptr);
    }

    emulatorInitialized = false;
    LOG_SUCCESS(logType, "destroyed emulator state");
    return true;
}

void start() {
    LOG_INFO(logType, "started psp emulation");
}

void resume() {
    LOG_INFO(logType, "resumed psp emulation");
}

void pause() {
    LOG_INFO(logType, "paused psp emulation");
}

void step(uint64_t steps) {
    Core::Kernel::hleThreadRunFor(steps);
}

void frame() {
    step((uint64_t)Core::Timing::msToCycles(16.67));
    // LOG_DEBUG(logType, "current frame %llu", ++frameCounter);
}

bool loadGameProgram() {
    if (!emulatorInitialized) {
        LOG_ERROR(logType, "can't load game program while emulator is not initialized!");
        return false;
    }

    if (Core::Loader::AbstractLoader *p = Core::Loader::getGameLoaderHandle(); p != nullptr) {
        delete p;
        Core::Loader::setGameLoaderHandle(nullptr);
    }

    Core::Loader::AbstractLoader *loader = Core::Loader::load(getGamePath());

    if (!loader) {
        LOG_ERROR(logType, "can't initialize loader!");
        return false;
    }

    if (loader->loadProgram() != 0) {
        LOG_ERROR(logType, "can't load ELF program!");
        delete loader;
        return false;
    }

    Core::Kernel::PSPModule *pspModule = Core::Loader::getModule(loader);
    if (!pspModule) {
        LOG_ERROR(logType, "can't get game module!");
        delete loader;
        return false;
    }

    LOG_TRACE(logType, "loading game module \"%s\" entry address 0x%08X", pspModule->moduleName, pspModule->entryAddress);
    Core::Kernel::createThreadFromModule(pspModule);
    Core::Loader::setGameLoaderHandle(loader);
    return true;
}

void setDebugMode(bool state) {
    debugModeState = true;
}

bool isDebugModeEnabled() {
    return debugModeState;
}
}