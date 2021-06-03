#include <cstdint>
#include "mathutils.h"

namespace ZZT {

    void Random::SetSeed(uint32_t s) {
        seed = s;
    }

    int16_t Random::Next(int16_t max) {
        seed = (seed * 134775813) + 1;
        return (seed >> 16) % max;
    }

}
