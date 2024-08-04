#include <cstdio>
#include <SDL.h>
#include <Core/HostController.h>

#include <Core/HLE/Modules/sceCtrl.h>

namespace Core::Controller {

struct KeyMap {
    int scancode;
    int control;
};

static KeyMap map[12] = {
    { SDL_SCANCODE_P, SCE_CTRL_START }, // 0
    { SDL_SCANCODE_O, SCE_CTRL_SELECT }, // 1
    { SDL_SCANCODE_W, SCE_CTRL_TRIANGLE }, // 2
    { SDL_SCANCODE_A, SCE_CTRL_SQUARE }, // 3
    { SDL_SCANCODE_S, SCE_CTRL_CROSS }, // 4
    { SDL_SCANCODE_D, SCE_CTRL_CIRCLE }, // 5
    { SDL_SCANCODE_UP, SCE_CTRL_UP }, // 6
    { SDL_SCANCODE_DOWN, SCE_CTRL_DOWN }, // 7
    { SDL_SCANCODE_LEFT, SCE_CTRL_LEFT }, // 8
    { SDL_SCANCODE_RIGHT, SCE_CTRL_RIGHT }, // 9
    { SDL_SCANCODE_L, SCE_CTRL_L }, // 10
    { SDL_SCANCODE_R, SCE_CTRL_R }, // 11
};

static unsigned int buttons = 0;

static uint8_t analogAxisX = 128, analogAxisY = 128;

void updateHostController() {
    const uint8_t *data = SDL_GetKeyboardState(nullptr);
    for (int i = 0; i < 12; i++) {
        if (data[map[i].scancode]) {
            buttons |= map[i].control;
        } else {
            buttons &= ~map[i].control;
        }
    }

    analogAxisX = 128;
    analogAxisY = 128;

    if (data[map[8].scancode]) {
        analogAxisX = 0;
    }

    if (data[map[9].scancode]) {
        analogAxisX = 255;
    }

    if (data[map[6].scancode]) {
        analogAxisY = 0;
    }

    if (data[map[7].scancode]) {
        analogAxisY = 255;
    }

}

uint32_t pspGetButtons() {
    return buttons;
}

uint8_t pspGetAnalogAxisX() {
    return analogAxisX;
}

uint8_t pspGetAnalogAxisY() {
    return analogAxisY;
}
}