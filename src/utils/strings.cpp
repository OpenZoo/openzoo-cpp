#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "strings.h"

namespace ZZT {

    DynString::DynString() {
        set_length(0);
    }

    void DynString::set_length(size_t l) {
        if (l > 255) {
            len = 255;
        } else {
            len = l;
        }
        data = (char*) malloc(sizeof(char) * (len + 1));
        data[len] = 0;
    }

    DynString::DynString(const char *s) {
        size_t l = strlen(s);
        set_length(l);
        strncpy(data, s, len);
    }

    DynString::DynString(const DynString &other) {
        set_length(other.len);
        memcpy(data, other.data, len);
    }

    DynString::DynString(size_t length) {
        set_length(length);
    }

    DynString::~DynString() {
        free(data);
        data = nullptr;
    }

    DynString& DynString::operator=(const DynString& other) {
        // copy
        if (this != &other) {
            if (len != other.len) {
                free(data);
                set_length(other.len);
            }
            memcpy(data, other.data, len);
        }
        return *this;
    }

    DynString& DynString::operator=(DynString&& other) {
        // move
        if (this != &other) {
            if (len != other.len) {
                free(data);
                set_length(other.len);
                memcpy(data, other.data, len);
                free(other.data);
                other.data = nullptr;
            } else {
                data = other.data;
                other.data = nullptr;
            }
        }
        return *this;
    }

    DynString DynString::operator+(const DynString &rhs) {
        DynString result = DynString(len + rhs.len);
        memcpy(result.data, data, len);
        memcpy(result.data + len, rhs.data, result.len - len);
        return result;
    }

    DynString DynString::operator+(const char *rhs) {
        size_t rhs_len = strlen(rhs);
        DynString result = DynString(len + rhs_len);
        memcpy(result.data, data, len);
        memcpy(result.data + len, rhs, result.len - len);
        return result;
    }

    DynString DynString::operator+(char rhs) {
        if (len >= 255) return *this;

        DynString result = DynString(len + 1);
        memcpy(result.data, data, len);
        result.data[len] = rhs;
        return result;
    }

    DynString DynString::substr(size_t from, size_t length) {
        if (length <= 0 || from >= len) {
            return DynString();
        }

        if (from < 0) from = 0;
        if (length > (len - from)) length = len - from;
        DynString result = DynString(*this);
        memcpy(result.data, data + from, result.len);
        return result;
    }

}