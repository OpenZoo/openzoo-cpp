#ifndef __FILESYSTEM_POSIX_H__
#define __FILESYSTEM_POSIX_H__

#include <cstdio>
#include "filesystem.h"

namespace ZZT {
    class PosixIOStream : public IOStream {
    protected:
        FILE *file;
        bool is_write;

    public:
        PosixIOStream(const char *name, bool write);
        ~PosixIOStream();

        size_t read(uint8_t *ptr, size_t len) override;
        size_t write(const uint8_t *ptr, size_t len) override;
        size_t skip(size_t len) override;
        size_t tell(void) override;
        bool eof(void) override;
    };

    class PosixFilesystemDriver: public PathFilesystemDriver {
    public:
        PosixFilesystemDriver();

        virtual IOStream *open_file_absolute(const char *filename, bool write) override;
        virtual bool list_files(std::function<bool(FileEntry&)> callback) override;
        virtual bool has_parent(void) override;
   };
}

#endif
