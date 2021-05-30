#ifndef __UTILS_QUIRKSET_H__
#define __UTILS_QUIRKSET_H__

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <initializer_list>
#include <type_traits>

namespace ZZT {

    template<typename E, size_t Esize>
    class QuirkSet {
        static_assert(std::is_enum<E>::value, "QuirkSet type must be an enum.");
        uint8_t flags[(Esize + 7) >> 3];

    public:    
        QuirkSet() {
            clear();
        }

        QuirkSet(std::initializer_list<E> list) {
            clear();
            for (auto e: list) {
                set(e);
            }
        }

        inline bool is(E value) const {
            return (flags[value >> 3] & (1 << (value & 7))) != 0;
        }

        inline bool isNot(E value) const {
            return (flags[value >> 3] & (1 << (value & 7))) == 0;
        }

        inline void clear() {
            memset(flags, 0, sizeof(flags));
        }

        inline void unset(E value) {
            flags[value >> 3] &= ~(1 << (value & 7));
        }

        inline void set(E value) {
            flags[value >> 3] |= (1 << (value & 7));
        }

        inline bool first(E value) {
            if (isNot(value)) {
                set(value);
                return true;
            } else {
                return false;
            }
        }
    };
    
}

#endif