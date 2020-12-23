#include "Utility.h"
#include <random>

namespace {

static std::random_device device;
static std::minstd_rand generator(device());

};

float RandomFloat(float a) {
    std::uniform_real_distribution<float> d(0, a);
    return d(generator);
}

uint32_t RandomUInt(uint32_t a) {
    std::uniform_int_distribution<uint32_t> d(0, a - 1);
    return d(generator);
}