#include <Core/Utility/RandomNumberGenerator.h>

#include <random>

namespace Core::Utility {
void RandomGenerator::operator=(const RandomGenerator& o) {
    this->generator32 = o.generator32;
    this->distribution = o.distribution;
}

void RandomGenerator::setRandomSeed() {
    generator32.seed(rd());
}

void RandomGenerator::setSeed(uint32_t seed) {
    generator32.seed(seed);
}

void RandomGenerator::setRange(uint32_t min, uint32_t max) {
    distribution = std::uniform_int_distribution<int>(min, max);
}

uint32_t RandomGenerator::operator()() {
    return distribution(generator32);
}

static RandomGenerator globalGen;

constexpr const static uint32_t min = 1048575;
constexpr const static uint32_t max = 16607650;

void initRNG(bool randomize, uint32_t seed) {
    RandomGenerator g;

    if (randomize)
        g.setRandomSeed();
    else
        g.setSeed(seed);

    g.setRange(min, max);
    globalGen = g;
}

uint32_t getRandomInt() {
    return globalGen();
}
}