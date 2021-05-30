#ifndef __IOSTREAM_H__
#define __IOSTREAM_H__

#include <cstdint>

namespace ZZT {

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

}

#endif