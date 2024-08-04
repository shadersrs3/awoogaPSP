#include <cstdio>

#include <Core/Timing.h>

#include <Core/Allegrex/AllegrexState.h>

using namespace Core::Allegrex;

namespace Core::Timing {
static int timesliceQuantum = 20000;
static uint64_t cpuClockFrequency = 222000000;
static int64_t cyclesDowncount;
static uint64_t cyclesElapsed;
static uint64_t baseCycles;

uint64_t idleCyclesTaken = 0;

const uint64_t& getBaseCycles() {
    return baseCycles;
}

void setBaseCycles(uint64_t cycles) {
    baseCycles = cycles;
}

void addIdleCycles(uint64_t cycles) {
    idleCyclesTaken += cycles;
}

uint64_t getIdleCycles() {
    return idleCyclesTaken;
}

void consumeIdleCycles() {
    idleCyclesTaken = 0;
}

void setTimesliceQuantum(int quantum) {
    timesliceQuantum = quantum;
}

int getTimesliceQuantum() {
    return timesliceQuantum;
}

void setClockFrequency(uint64_t hz) {
    cpuClockFrequency = hz;
}

void setDowncount(int64_t downcount) {
    cyclesDowncount = downcount;
}

int64_t& getDowncount() {
    return cyclesDowncount;
}

void consumeCycles(int64_t elapsed) {
    cyclesDowncount -= elapsed;
    cyclesElapsed += elapsed;
}

const uint64_t& getClockFrequency() {
    return cpuClockFrequency;
}

const uint64_t& getCurrentCycles() {
    return cyclesElapsed;
}

uint64_t getSystemTimeMilliseconds() {
    uint64_t systemTime = (uint64_t)((getCurrentCycles() * 1000uLL) / (double)getClockFrequency());
    return systemTime;
}

uint64_t getSystemTimeMicroseconds() {
    uint64_t systemTime = (uint64_t)((getCurrentCycles() * 1000000uLL) / (double)getClockFrequency());
    return systemTime;
}

uint64_t usToCycles(double time) {
    return (uint64_t)((time * getClockFrequency()) / 1000000.);
}

uint64_t msToCycles(double time) {
    return (uint64_t)((time * getClockFrequency()) / 1000.);
}

uint64_t usToCycles(uint64_t time) {
    return (uint64_t)((time * getClockFrequency()) / 1000000);
}

uint64_t msToCycles(uint64_t time) {
    return (uint64_t)((time * getClockFrequency()) / 1000);
}
}