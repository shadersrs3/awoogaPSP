#pragma once

#include <cstdint>

namespace Core::Timing {
const uint64_t& getBaseCycles();
void setBaseCycles(uint64_t cycles);
void addIdleCycles(uint64_t cycles);
uint64_t getIdleCycles();
void consumeIdleCycles();
void setTimesliceQuantum(int quantum);
int getTimesliceQuantum();
void setClockFrequency(uint64_t hz);
const uint64_t& getClockFrequency();
void setDowncount(int64_t downcount);
int64_t& getDowncount();
void consumeCycles(int64_t cyclesElapsed);
const uint64_t& getCurrentCycles();
uint64_t getSystemTimeMilliseconds();
uint64_t getSystemTimeMicroseconds();
uint64_t usToCycles(double us);
uint64_t msToCycles(double ms);
uint64_t usToCycles(uint64_t time);
uint64_t msToCycles(uint64_t ms);
}