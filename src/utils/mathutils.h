#ifndef __UTILS_MATHUTILS_H__
#define __UTILS_MATHUTILS_H__

#include <cstdint>

namespace ZZT {

    class Random {
        uint32_t seed;

public:
        Random(): seed(0) { };
        Random(uint32_t s): seed(s) { };

        void SetSeed(uint32_t s);
        int16_t Next(int16_t max);
    };

	template<typename T> T NextPowerOfTwo(T value) {
		value--;
		value |= value >> 1;
		value |= value >> 2;
		value |= value >> 4;
		value |= value >> 8;
		value |= value >> 16;
		value |= value >> 32;
		return value + 1;
	}

    template<typename T> T Sqr(T value) {
        return value * value;
    }

    template<typename T> T Signum(T value) {
        if (value > 0) return 1;
        else if (value < 0) return -1;
        else return 0;
    }

    template<typename T> T Abs(T value) {
        return value < 0 ? (-value) : (value);
    }

    template<typename T> T Difference(T a, T b) {
        return (a >= b) ? (a - b) : (b - a);
    }
    
}

#endif
