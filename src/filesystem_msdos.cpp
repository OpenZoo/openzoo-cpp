#include <cstdio>
#include <cstdlib>
#include <dir.h>
#include <unistd.h>
#include "filesystem_msdos.h"
#include "utils/stringutils.h"

using namespace ZZT;

#define HAS_PARENT_LENGTH 3
#define PATH_SEPARATOR '/'

MsdosFilesystemDriver::MsdosFilesystemDriver()
    : PathFilesystemDriver(nullptr, FILENAME_MAX, PATH_SEPARATOR, false) {
    char *cwd_path = (char*) malloc(max_path_length + 1);
    getcwd(cwd_path, max_path_length);
    if (StrEmpty(cwd_path)) {
        strcpy(cwd_path, ".");
    }
    realpath(cwd_path, this->current_path);
    free(cwd_path);
}

IOStream *MsdosFilesystemDriver::open_file_absolute(const char *filename, bool write) {
    return new PosixIOStream(filename, write);
}

bool MsdosFilesystemDriver::has_parent(void) {
    return strlen(current_path) > HAS_PARENT_LENGTH;
}

bool MsdosFilesystemDriver::list_files(std::function<bool(FileEntry&)> callback) {
    FileEntry fsentry;

    struct ffblk ffblk;

    char *joined_path = (char*) malloc(max_path_length + 1);
    join_path(joined_path, max_path_length, current_path, "*.*");

    int done = findfirst(joined_path, &ffblk, FA_DIREC);
    while (!done) {
        fsentry.is_dir = (ffblk.ff_attrib & FA_DIREC) != 0;
        fsentry.filename = ffblk.ff_name;

        if (!callback(fsentry)) {
            break;
        }

        done = findnext(&ffblk);
    }

    free(joined_path);

    return true;
}
