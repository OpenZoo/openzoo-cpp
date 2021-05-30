#include <cstdlib>
#include "utils/strings.h"
#include "filesystem.h"

using namespace ZZT;

FilesystemDriver::FilesystemDriver(bool read_only) {
    this->read_only = read_only;
}

bool FilesystemDriver::is_path_driver() {
    return false;
}

NullFilesystemDriver::NullFilesystemDriver()
: FilesystemDriver(false) {
    
}

IOStream *NullFilesystemDriver::open_file(const char *filename, bool write) {
    return new ErroredIOStream();
}

bool NullFilesystemDriver::list_files(std::function<bool(FileEntry&)> callback) {
    return true;
}

static void zoo_path_cat(char *dest, const char *src, size_t n, char separator) {
	size_t len = strlen(dest);
	if (len < n && dest[len - 1] != separator) {
		dest[len++] = separator;
		dest[len] = '\0';
	}
	strncpy(dest + len, src, n - len);
}

PathFilesystemDriver::PathFilesystemDriver(const char *starting_path, size_t max_path_length, char separator, bool read_only)
    : FilesystemDriver(read_only) {
    this->max_path_length = max_path_length;
    this->separator = separator;
    this->current_path = (char*) malloc(max_path_length + 1);
    if (starting_path != nullptr) {
        strcpy(this->current_path, starting_path);
    } else {
        StrClear(this->current_path);
    }
}

PathFilesystemDriver::~PathFilesystemDriver() {
    free(this->current_path);
}

bool PathFilesystemDriver::is_path_driver() {
    return true;
}

IOStream *PathFilesystemDriver::open_file(const char *filename, bool write) {
    char *path = (char*) malloc(max_path_length + 1);
    join_path(path, max_path_length, current_path, filename);
    auto stream = open_file_absolute(path, write);
    free(path);
    return stream;
}

bool PathFilesystemDriver::open_dir(const char *name) {
    return join_path(this->current_path, max_path_length, this->current_path, name);
}

bool PathFilesystemDriver::join_path(char *dest, size_t len, const char *curr, const char *next) {
    const char *pathsep_pos;
    size_t pathsep_len;

    if (curr == nullptr) {
        return false;
    } else if (next == nullptr || !strcmp(next, ".")) {
        if (dest != curr) {
            strncpy(dest, curr, len);
        }
        return true;
    } else if (!strcmp(next, "..")) {
        pathsep_pos = strrchr(curr, separator);
        if (pathsep_pos == nullptr) {
            return false;
        }
        pathsep_len = pathsep_pos - curr;
        if (pathsep_len >= len) {
            return false;
        }
        if (dest != curr) {
            memcpy(dest, curr, pathsep_len);
        }
        dest[pathsep_len] = '\0';
        return true;
    } else {
        if (dest != curr) {
            strncpy(dest, curr, len);
        }
        zoo_path_cat(dest, next, len, separator);
        return true;
    }
}

bool PathFilesystemDriver::split_path(const char *path, char *parent, size_t parent_len, char *filename, size_t filename_len) {
    if (path == filename) {
        // Unsupported edge case
        return false;
    }

    const char *divide = strrchr(path, separator);
    if (divide == nullptr) {
        // only filename
        if (filename != nullptr) {
            strncpy(filename, path, filename_len);
            filename[filename_len - 1] = 0;
        }
        return true;        
    }

    size_t pre_len = (divide - path);
    size_t post_len = strlen(path) - (pre_len + 1);

    if (filename != nullptr) {
        if (post_len > 0) {
            // copy filename
            if (post_len > (filename_len - 1)) post_len = filename_len - 1;
            memcpy(filename, divide + 1, post_len);
            filename[post_len] = 0; 
        } else {
            filename[0] = 0;
        }
    }

    if (parent != nullptr) { 
        if (parent != path) {
            if (pre_len > (parent_len - 1)) pre_len = parent_len - 1;
            memcpy(parent, path, pre_len);
        }
        parent[pre_len] = 0; 
    }

    return true;
}