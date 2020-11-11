#ifndef __UTILS_H__
#define __UTILS_H__

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "config.h"

template <size_t N>
using sstring = char[N + 1];

namespace ZZT {
namespace Utils {
    class Random {
        uint32_t seed;

public:
        Random(): seed(0) { };
        Random(uint32_t s): seed(s) { };

        void SetSeed(uint32_t s);
        int16_t Next(int16_t max);
    };

    // Maths

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

    // Chars/Strings

    template<typename T> inline T UpCase(T c) {
        return (((c) >= 'a' && (c) <= 'z') ? ((c) - 0x20) : (c));
    }

    template<size_t i> constexpr int StrSize(char (&str)[i]) {
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
        strncpy(dst, src, i - 1);
        dst[i - 1] = '\0';
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

    // I/O

    class IOStream {
    protected:
        bool error_condition;

    public:
        virtual ~IOStream() { }

        virtual size_t read(uint8_t *ptr, size_t len) = 0;
        virtual size_t write(const uint8_t *ptr, size_t len) = 0;
        virtual size_t skip(size_t len) = 0;
        virtual size_t tell(void) = 0;
        virtual bool eof(void) = 0;
        virtual uint8_t *ptr(void);

        inline bool errored(void) {
            return error_condition;
        }
        
        uint8_t read8(void);
        uint16_t read16(void);
        uint32_t read32(void);
        bool read_bool(void);
        size_t read_cstring(char *ptr, size_t ptr_len, char terminator);
        bool read_pstring(char *ptr, size_t ptr_len, size_t str_len, bool packed);

        bool write8(uint8_t value);
        bool write16(uint16_t value);
        bool write32(uint32_t value);
        bool write_bool(bool value);
        bool write_cstring(const char *ptr, bool terminate);
        bool write_pstring(const char *ptr, size_t str_len, bool packed);
    };

    class ErroredIOStream : public IOStream {
    public:
        ErroredIOStream();

        size_t read(uint8_t *ptr, size_t len) override;
        size_t write(const uint8_t *ptr, size_t len) override;
        size_t skip(size_t len) override;
        size_t tell(void) override;
        bool eof(void) override;
    };

    class MemoryIOStream : public IOStream {
        uint8_t *memory;
        size_t mem_pos;
        size_t mem_len;
        bool is_write;

    public:
        MemoryIOStream(const uint8_t *memory, size_t mem_len)
            : MemoryIOStream((uint8_t *) memory, mem_len, false) {}
        MemoryIOStream(uint8_t *memory, size_t mem_len, bool write);
        ~MemoryIOStream();

        size_t read(uint8_t *ptr, size_t len) override;
        size_t write(const uint8_t *ptr, size_t len) override;
        size_t skip(size_t len) override;
        size_t tell(void) override;
        bool eof(void) override;
        virtual uint8_t *ptr(void) override;
    };

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
}

#endif