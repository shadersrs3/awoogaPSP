#pragma once

#include <cstdint>
#include <limits>

#include <random>

namespace Core::Utility {
struct RandomGenerator {
private:
    std::random_device rd;
    std::mt19937 generator32;
    std::uniform_int_distribution<int> distribution;
public:
    void operator=(const RandomGenerator& o);
    void setRandomSeed();
    void setSeed(uint32_t seed = std::mt19937::default_seed);
    void setRange(uint32_t min, uint32_t max);
    uint32_t operator()();
};

void initRNG(bool randomize = false, uint32_t seed = std::mt19937::default_seed);

void setSeedAndRange32(uint32_t seed = -1,
    uint32_t min = std::numeric_limits<uint32_t>::min(),
    uint32_t max = std::numeric_limits<uint32_t>::max());
void setSeedAndRange32Random(uint32_t min = std::numeric_limits<uint32_t>::min(),
                        uint32_t max = std::numeric_limits<uint32_t>::max());
uint32_t getRandomInt();
}