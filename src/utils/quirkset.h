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

        template<E value>
        constexpr bool valid() const { return true; }

        template<E value>
        constexpr bool is() const {
            if (!valid<value>()) return false;
            return (flags[value >> 3] & (1 << (value & 7))) != 0;
        }

        template<E value>
        constexpr bool isNot() const {
            if (!valid<value>()) return true;
            return !is<value>();
        }

        inline void clear() {
            memset(flags, 0, sizeof(flags));
        }

        template<E value>
        constexpr void unset() {
            flags[value >> 3] &= ~(1 << (value & 7));
        }

        template<E value>
        constexpr void set() {
            if(!valid<value>()) std::abort();
            flags[value >> 3] |= (1 << (value & 7));
        }

        template<E value>
        inline bool first() {
            if (isNot<value>()) {
                set<value>();
                return true;
            } else {
                return false;
            }
        }
    };
    
}

#endif