#pragma once

#include <string>

#include <cstdint>

namespace Core::Emulator {
enum class EmulatorState {
    STOPPED, RUNNING,
};

bool initialize();
bool reset();
bool destroy();

void start();
void resume();
void pause();

void frame();
void step(uint64_t steps);

void setGamePath(const std::string& path);
const std::string& getGamePath();
bool loadGameProgram();

void setDebugMode(bool state);
bool isDebugModeEnabled();
const EmulatorState& getEmulatorState();
}