#pragma once

#include <deque>

namespace Core::HLE {
int scePowerSetClockFrequency(int pllfreq, int cpufreq, int busfreq);
std::deque<int> *getReadyPowerCallbacks();
int scePowerRegisterCallback(int slot, int cbid);
}