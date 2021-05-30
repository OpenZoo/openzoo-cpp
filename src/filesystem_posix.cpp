#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "utils/strings.h"
#include "filesystem_posix.h"

using namespace ZZT;

#if defined(WIN32)
#include <windows.h>
#endif

#if defined(WIN32) || defined(MSDOS)
#define CASE_SENSITIVE false
#define HAS_PARENT_LENGTH 3
#define PATH_SEPARATOR '\\'
#else
#define CASE_SENSITIVE true
#define HAS_PARENT_LENGTH 1
#define PATH_SEPARATOR '/'
#endif

PosixIOStream::PosixIOStream(const char *name, bool write) {
    this->file = fopen(name, write ? "wb" : "rb");
    this->is_write = write;
    this->error_condition = this->file == NULL;
}

size_t PosixIOStream::read(uint8_t *ptr, size_t len) {
    if (errored() || is_write) return 0;
    size_t result = fread((void*) ptr, 1, len, file);
    this->error_condition |= (result != len);
    return result;
}

size_t PosixIOStream::write(const uint8_t *ptr, size_t len) {
    if (errored() || !is_write) return 0;
    size_t result = fwrite((void*) ptr, 1, len, file);
    this->error_condition |= (result != len);
    return result;
}

size_t PosixIOStream::skip(size_t len) {
    if (errored()) return 0;
    if (fseek(file, len, SEEK_CUR) != 0) {
        this->error_condition = true;
    }
    // TODO: return correct value
    return len;
}

size_t PosixIOStream::tell(void) {
    if (errored()) return 0;
    return ftell(file);
}

bool PosixIOStream::eof(void) {
    if (file == NULL) return false;
    return feof(file) != 0;
}

PosixIOStream::~PosixIOStream() {
    if (this->file != NULL) {
        fclose(file);
        this->file = NULL;
    }
}

PosixFilesystemDriver::PosixFilesystemDriver()
    : PathFilesystemDriver(nullptr, FILENAME_MAX, PATH_SEPARATOR, false) {
    char *cwd_path = (char*) malloc(max_path_length + 1);
    getcwd(cwd_path, max_path_length);
    if (StrEmpty(cwd_path)) {
        strcpy(cwd_path, ".");
    }
#if defined(WIN32)
    GetFullPathNameA(cwd_path, max_path_length + 1, this->current_path, NULL);
#elif defined(HAVE_REALPATH)
    realpath(cwd_path, this->current_path);
#else
    strncpy(this->current_path, cwd_path, max_path_length);
    this->current_path[max_path_length] = 0;
#endif
    free(cwd_path);
}

IOStream *PosixFilesystemDriver::open_file_absolute(const char *filename, bool write) {
    IOStream *stream = new PosixIOStream(filename, write);
#ifdef CASE_SENSITIVE
    // emulate case-insensitiveness
    if (stream->errored()) {
        char *path_parent = (char*) malloc(max_path_length);
        char *path_filename = (char*) malloc(max_path_length);
        if (split_path(filename, path_parent, max_path_length, path_filename, max_path_length)) {
            delete stream;

            list_files([path_filename](FileEntry &entry) -> bool {
                if (strcasecmp(entry.filename, path_filename) == 0) {
                    strcpy(path_filename, entry.filename);
                    return false;
                } else {
                    return true;
                }
            });

            join_path(path_parent, max_path_length, path_parent, path_filename);    
            stream = new PosixIOStream(path_parent, write);
        }
        free(path_filename);
        free(path_parent);
    }
#endif
    return stream;
}

bool PosixFilesystemDriver::has_parent(void) {
    return strlen(current_path) > HAS_PARENT_LENGTH;
}

bool PosixFilesystemDriver::list_files(std::function<bool(FileEntry&)> callback) {
    FileEntry fsentry;

    DIR *dir;

    if ((dir = opendir(this->current_path)) == NULL) {
        return false;
    }

    char *joined_path = (char*) malloc(max_path_length + 1);
    joined_path[max_path_length] = 0;

    struct dirent *entry;
    struct stat st;

    while ((entry = readdir(dir)) != NULL) {
        join_path(joined_path, max_path_length, this->current_path, entry->d_name);
        stat(joined_path, &st);

        fsentry.is_dir = S_ISDIR(st.st_mode) != 0;
        fsentry.filename = entry->d_name;

        if (!callback(fsentry)) {
            break;
        }
    }

    free(joined_path);
    closedir(dir);

    return true;
}
