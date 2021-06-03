#ifndef __UTILS_STRINGUTILS_H__
#define __UTILS_STRINGUTILS_H__

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "config.h"

template <size_t N>
using sstring = char[N + 1];

namespace ZZT {

    template<typename T> inline T UpCase(T c) {
        return (((c) >= 'a' && (c) <= 'z') ? ((c) - 0x20) : (c));
    }

    template<size_t i> constexpr size_t StrSize(char (&str)[i]) {
        return i - 1;
    }

    template<size_t i> inline int StrLength(char (&str)[i]) {
#if defined(HAVE_STRNLEN)
        return strnlen(str, i);
#else
        return strlen(str);
#endif
    }

    template<size_t i> inline void StrCopy(char (&dst)[i], const char *src) {
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wstringop-truncation"
        strncpy(dst, src, i - 1);
        dst[i - 1] = '\0';
#pragma GCC diagnostic pop
    }

    template<size_t len> void StrUpCase(char (&dst)[len], const char *src) {
        size_t src_len = strlen(src);
        if (src_len > (len - 1)) src_len = len - 1;
        for (size_t i = 0; i < src_len; i++)
            dst[i] = UpCase(src[i]);
        dst[src_len] = 0;
    }

    template<size_t i> inline void StrJoin(char (&dst)[i], int count, ...) {
        va_list args;
        va_start(args, count);
        if (count > 0) {
            strncpy(dst, va_arg(args, const char *), i - 1);
            dst[i - 1] = 0;
            for (int k = 1; k < count; k++) {
                strncat(dst, va_arg(args, const char *), i - 1);
                dst[i - 1] = 0;
            }
        } else {
            dst[0] = 0;
        }
        va_end(args);
    }

    template<size_t i> inline void StrFormat(char (&dst)[i], const char *format, ...) {
        va_list args;
        va_start(args, format);
#if defined(HAVE_VSNIPRINTF)
        // Avoid linking in floating-point parsing code on newlib-based platforms.
        vsniprintf(dst, i, format, args);
#else
        vsnprintf(dst, i, format, args);
#endif
        va_end(args);
    }

    template<size_t i> inline void StrFromInt(char (&dst)[i], int value) {
        return StrFormat(dst, "%d", value);
    }

    inline bool StrEquals(const char *a, const char *b) {
        return !strcmp(a, b); 
    }

    inline void StrClear(char *s) {
        s[0] = 0;
    }

    inline bool StrEmpty(const char *s) {
        return s[0] == 0;
    }

    // Dynamic strings

    class DynString {
    private:
        uint8_t len;
        char *data;

        void set_length(size_t new_len);
        DynString(size_t length);

    public:
        DynString();
        DynString(const char *s);
        DynString(const DynString &other);

        ~DynString();

        DynString& operator=(const DynString& other);
        DynString& operator=(DynString&& other);
        DynString operator+(const DynString &rhs);
        DynString operator+(const char *rhs);
        DynString operator+(char rhs);
        DynString substr(size_t from, size_t length);

        inline const char* c_str() const {
            return data;
        }

        inline size_t length() const {
            return len;
        }

        inline char operator[](int idx) const {
            return data[idx];
        }
    };

}

#endif
