#ifndef __FILESYSTEM_MSDOS_H__
#define __FILESYSTEM_MSDOS_H__

#include "filesystem.h"
#include "filesystem_posix.h"
#include "utils/iostream.h"

namespace ZZT {
    class MsdosFilesystemDriver: public PathFilesystemDriver {
    public:
        MsdosFilesystemDriver();

        virtual IOStream *open_file_absolute(const char *filename, bool write) override;
        virtual bool list_files(std::function<bool(FileEntry&)> callback) override;
        virtual bool has_parent(void) override;
   };
}

#endif
