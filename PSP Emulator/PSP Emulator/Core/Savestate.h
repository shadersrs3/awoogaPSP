#pragma once

#include <string>

#include <cstdint>

namespace Core::Savestate {
bool saveState(const std::string& state);
bool loadState(const std::string& state);
}