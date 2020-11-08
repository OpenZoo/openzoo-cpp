#ifndef __FILESYSTEM_ROMFS_H__
#define __FILESYSTEM_ROMFS_H__

#include "filesystem.h"
#include "utils.h"

namespace ZZT {
    class RomfsFilesystemDriver: public PathFilesystemDriver {
    protected:
        const uint8_t *ptr, *root_ptr;
        bool valid;

    public:
        RomfsFilesystemDriver(const void *ptr);

        virtual Utils::IOStream *open_file_absolute(const char *filename, bool write) override;
        virtual bool list_files(std::function<bool(FileEntry&)> callback) override;
        virtual bool has_parent(void) override;
   };
}

#endif
