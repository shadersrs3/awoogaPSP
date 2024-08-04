#pragma once

#include <string>

namespace Core::Emulator::Config {
void loadEmulatorConfiguration();
void saveEmulatorConfiguration();
void loadControllerKeyBindings();
void saveControllerKeyBindings();
}