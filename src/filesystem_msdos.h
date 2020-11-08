#ifndef __FILESYSTEM_MSDOS_H__
#define __FILESYSTEM_MSDOS_H__

#include "filesystem.h"
#include "filesystem_posix.h"
#include "utils.h"

namespace ZZT {
    class MsdosFilesystemDriver: public PathFilesystemDriver {
    public:
        MsdosFilesystemDriver();

        virtual Utils::IOStream *open_file_absolute(const char *filename, bool write) override;
        virtual bool list_files(std::function<bool(FileEntry&)> callback) override;
        virtual bool has_parent(void) override;
   };
}

#endif
