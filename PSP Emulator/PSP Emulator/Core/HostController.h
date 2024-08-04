#pragma once

#include <cstdint>

namespace Core::Controller {
void updateHostController();
uint32_t pspGetButtons();
uint8_t pspGetAnalogAxisX();
uint8_t pspGetAnalogAxisY();
}