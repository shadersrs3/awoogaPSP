#define _CRT_SECURE_NO_WARNINGS

#include <SDL.h>

#include <Core/Emulator.h>
#include <Core/Logger.h>

#include <Core/Kernel/sceKernelThread.h>

#include <Core/Memory/MemoryState.h>

#include <Core/HLE/Modules/sceRtc.h>
#include <Core/HLE/Modules/IoFileMgrForUser.h>

#include <Core/Memory/MemoryAccess.h>
#include <Core/GPU/GE.h>

#include <Core/HostController.h>

#include <Core/GPU/GPU.h>
#include <Core/GPU/Renderer.h>
#include <Core/GPU/DisplayList.h>

#include <Core/HLE/Modules/sceAtrac3plus.h>

#include <Core/Timing.h>

#include <GL/glew.h>

#include <Core/Memory/MemoryAccess.h>

#include <thread>
#include <mutex>

using namespace Core::Kernel;

extern "C" __declspec(dllimport) unsigned int timeBeginPeriod(unsigned int res);
extern "C" bool SetConsoleTitleW(const wchar_t *name);

std::atomic<bool> quit = false;
SDL_Window *window = nullptr;
SDL_GLContext ctx = nullptr;

static void userLoadGame() {
    Core::HLE::setHostCurrentWorkingDirectory("...");
    Core::Emulator::setGamePath("...");
}

static void userLoadHomebrew() {
}

static bool __frontEndInit() {
    timeBeginPeriod(1);

    SDL_Init(SDL_INIT_VIDEO);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 6);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 16);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    SDL_GL_SetSwapInterval(0);

    window = SDL_CreateWindow("PSP emulator", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 480*2, 272*2, SDL_WINDOW_OPENGL);
    ctx = SDL_GL_CreateContext(window);
    SDL_GL_SetSwapInterval(0);

    SDL_GL_MakeCurrent(window, ctx);

    glewExperimental = true;
    if (auto err = glewInit(); err != GLEW_OK) {
        LOG_ERROR("main", "Can't process GLEW: %s", glewGetErrorString(err));
        return false;
    }
    return true;
}

void __frontendDestroy() {
    SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(window);
    SDL_Quit();
}

bool kernelTrace;

int main(int argc, char *argv[]) {
    SetConsoleTitleW(L"awooga log");
    bool state = Core::Emulator::initialize();

    if (!state) {
        LOG_ERROR("main", "can't initialize emulator");
        return -1;
    }

    userLoadGame();
    userLoadHomebrew();
    state = Core::Emulator::loadGameProgram();
    // state = false;
    if (state) {
#if 0
        Core::GPU::setRenderDevice(Core::GPU::createRenderDevice(Core::GPU::RENDERER_TYPE_NONE));
        for (int i = 0; i < 5000; i++) {
            Core::Emulator::frame();
        }

        auto *p = Core::Kernel::getKernelObjectList(Core::Kernel::KERNEL_OBJECT_THREAD);

        Core::GPU::destroyRenderDevice(Core::GPU::getRenderDevice());
        Core::GPU::setRenderDevice(nullptr);
#else
        bool state = __frontEndInit();
        LOG_DEBUG("main", "GPU Vendor Name: %s Renderer: %s", glGetString(GL_VENDOR), glGetString(GL_RENDERER));
        Core::GPU::setRenderDevice(Core::GPU::createRenderDevice(Core::GPU::RENDERER_TYPE_OPENGL));
        Core::GPU::__openglSetStreamingTextureDevice(Core::GPU::getRenderDevice(), false);

        bool speedhack = false;
        if (state) {
            while (!quit.load()) {
                Uint64 start = SDL_GetPerformanceCounter();
                
                SDL_Event ev;
                Core::GPU::GPUState *state = Core::GPU::getGPUState();
                while (SDL_PollEvent(&ev)) {
                    if (ev.type == SDL_QUIT) {
                        quit = true;
                        break;
                    }

                    switch (ev.type) {
                    case SDL_KEYDOWN:
                        if (ev.key.keysym.scancode == SDL_SCANCODE_X)
                            kernelTrace = !kernelTrace;
                        break;
                    }
                }

                const uint8_t *data = SDL_GetKeyboardState(nullptr);
                speedhack = !!data[SDL_SCANCODE_TAB];

                Core::Emulator::frame();
                Core::Controller::updateHostController();

                Core::GPU::__openglRenderScreen(Core::GPU::getRenderDevice());
                Uint64 end = SDL_GetPerformanceCounter();
                float elapsedMS = (end - start) / (float)SDL_GetPerformanceFrequency() * 1000.0f;
                constexpr const float refreshTime = 1000 / 60.f;
                SDL_GL_SwapWindow(window);

                if (elapsedMS < refreshTime) {
                    if (!speedhack)
                        std::this_thread::sleep_for(std::chrono::milliseconds(int(refreshTime - elapsedMS)));
                }
            }
        }
        Core::GPU::destroyRenderDevice(Core::GPU::getRenderDevice());
        Core::GPU::setRenderDevice(nullptr);
#endif
    } else {
        LOG_ERROR("main", "can't load game program!");
    }

    Core::Emulator::destroy();
    return 0;
}
