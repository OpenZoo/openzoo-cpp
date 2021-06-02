#include <cstdint>
#include <cstdlib>
#include <cstring>
#include "iostream.h"

namespace ZZT {

    uint8_t IOStream::read8(void) {
        uint8_t result = 0;
        read(&result, 1);
        return result;
    }

    uint16_t IOStream::read16(void) {
        uint8_t low = read8();
        return low | (read8() << 8);
    }

    uint32_t IOStream::read32(void) {
        uint16_t low = read16();
        return low | (read16() << 16);
    }

    bool IOStream::read_bool(void) {
        return read8() != 0;
    }

    size_t IOStream::read_cstring(char *ptr, size_t ptr_len, char terminator) {
        int ch = -1;
        size_t pos = 0;
        while (true) {
            ch = read8();
            if (ch > 0 && ((char) ch) != terminator && !errored()) {
                if (pos < (ptr_len - 1)) {
                    ptr[pos++] = ch;
                }
            } else break;
        }
        ptr[pos] = 0;
        return pos;
    }

    bool IOStream::read_pstring(char *ptr, size_t ptr_len, size_t str_len, bool packed) {
        uint8_t size = read8();
        if (packed) str_len = size;
        uint8_t to_read = (ptr_len < str_len) ? ptr_len : str_len;
        uint8_t to_skip = str_len - to_read;
        if (to_read > 0) {
            if (read((uint8_t *) ptr, to_read) != to_read) return false;
        }
        ptr[to_read < size ? to_read : size] = 0;
        if (to_skip > 0) {
            if (skip(to_skip) != to_skip) return false;
        }
        return true;
    }

    bool IOStream::write8(uint8_t value) {
        return write(&value, 1) != 0;
    }

    bool IOStream::write16(uint16_t value) {
        uint8_t low = value & 0xFF;
        uint8_t high = value >> 8;
        if (!write8(low)) return false;
        return write8(high);
    }

    bool IOStream::write32(uint32_t value) {
        uint16_t low = value & 0xFFFF;
        uint16_t high = value >> 16;
        if (!write16(low)) return false;
        return write16(high);
    }

    bool IOStream::write_bool(bool value) {
        return write8(value ? 1 : 0);
    }

    bool IOStream::write_cstring(const char *ptr, bool terminate) {
        while (true) {
            if (!terminate && (*ptr) == 0) {
                break;
            }
            write8(*ptr);
            if (errored() || (*ptr) == 0) {
                break;
            }
            ptr++;
        }
        return !errored();
    }

    bool IOStream::write_pstring(const char *ptr, size_t str_len, bool packed) {
        size_t ptr_len = strlen(ptr);
        if (ptr_len > str_len) ptr_len = str_len;
        if (ptr_len > 255) return false;
        if (!write8(ptr_len)) return false;
        if (write((const uint8_t*) ptr, ptr_len) < ptr_len) return false;
        if (!packed && str_len > ptr_len) {
            if (skip(str_len - ptr_len) < (str_len - ptr_len)) return false;
        }
        return true;
    }

	size_t IOStream::remaining(void) {
		return -1;
	}

    uint8_t *IOStream::ptr(void) {
        return nullptr;
    }

    ErroredIOStream::ErroredIOStream() {
        this->error_condition = true;
    }

    size_t ErroredIOStream::read(uint8_t *ptr, size_t len) {
        return 0;
    }

    size_t ErroredIOStream::write(const uint8_t *ptr, size_t len) {
        return 0;
    }

    size_t ErroredIOStream::skip(size_t len) {
        return 0;
    }

    size_t ErroredIOStream::tell(void) {
        return 0;
    }

    bool ErroredIOStream::reset(void) {
        return false;
    }

    bool ErroredIOStream::eof(void) {
        return false;
    }

    MemoryIOStream::MemoryIOStream(uint8_t *buf, size_t len, bool write) {
        this->memory = buf;
        this->mem_pos = 0;
        this->mem_len = len;
        this->is_write = write;
        this->error_condition = this->memory == NULL;
    }

    MemoryIOStream::~MemoryIOStream() {
        if (this->memory != NULL) {
            this->memory = NULL;
        }
    }

    size_t MemoryIOStream::read(uint8_t *ptr, size_t len) {
        if (errored() || is_write) return 0;
        size_t mem_count = mem_len - mem_pos;
        if (mem_count > len) mem_count = len;
        if (mem_count > 0) {
            memcpy((void*) ptr, (void*) (this->memory + mem_pos), mem_count);
            mem_pos += mem_count;
        }
        this->error_condition |= (mem_count != len);
        return mem_count;
    }

    size_t MemoryIOStream::write(const uint8_t *ptr, size_t len) {
        if (errored() || !is_write) return 0;
        size_t mem_count = mem_len - mem_pos;
        if (mem_count > len) mem_count = len;
        if (mem_count > 0) {
            memcpy((void*) (this->memory + mem_pos), (const void*) ptr, mem_count);
            mem_pos += mem_count;
        }
        this->error_condition |= (mem_count != len);
        return mem_count;
    }

    size_t MemoryIOStream::skip(size_t len) {
        if (errored()) return 0;
        size_t mem_count = mem_len - mem_pos;
        if (mem_count > len) mem_count = len;
        if (mem_count > 0) {
            if (is_write) {
                memset((void*) (this->memory + mem_pos), 0, mem_count);
            }
            mem_pos += mem_count;
        }
        this->error_condition |= (mem_count != len);
        return mem_count;
    }

    size_t MemoryIOStream::tell(void) {
        if (errored()) return 0;
        return mem_pos;
    }

	size_t MemoryIOStream::remaining(void) {
		return mem_len <= mem_pos ? 0 : mem_len - mem_pos;
	}

    bool MemoryIOStream::reset(void) {
		mem_pos = 0;
        return true;
    }

    bool MemoryIOStream::eof(void) {
        return mem_pos >= mem_len;
    }

    uint8_t *MemoryIOStream::ptr(void) {
        return this->memory + mem_pos;
    }
    
}