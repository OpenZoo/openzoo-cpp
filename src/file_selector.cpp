#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>

#include "utils.h"
#include "file_selector.h"

using namespace ZZT;
using namespace ZZT::Utils;

FileSelector::FileSelector(VideoDriver *video, InputDriver *input, SoundDriver *sound, FilesystemDriver *filesystem, const char *title, const char *extension)
    : window(TextWindow(video, input, sound, filesystem)) {
    this->filesystem = filesystem;
    this->title = title;
    this->extension = extension;
    StrClear(filename);
}

FileSelector::~FileSelector() {

}

bool FileSelector::select() {
    size_t ext_len = strlen(this->extension);
    PathFilesystemDriver *path_fs = filesystem->is_path_driver() ? static_cast<PathFilesystemDriver*>(filesystem) : nullptr;
    char *past_path = nullptr;
    if (path_fs != nullptr) {
        past_path = (char*) malloc(path_fs->get_max_path_length() + 1);
        strcpy(past_path, path_fs->get_current_path());
    }

    while (true) {
        window.Clear();
        StrCopy(window.title, title);
        window.selectable = true;
        StrClear(window.hyperlink);

        if (path_fs != nullptr) {
            if (path_fs->has_parent()) {
                window.Append("!..;[..]");
            }
        }

        int listed_start = window.line_count;
        filesystem->list_files([this, ext_len](FileEntry &entry) -> bool {
            sstring<20> wname;
            int name_len = strlen(entry.filename);
            if (name_len > StrSize(wname)) return true;

            if (entry.is_dir) {
                if (name_len > 0 && !StrEquals(entry.filename, ".") && !StrEquals(entry.filename, "..")) {
                    window.Append(DynString("!") + entry.filename + ";[" + entry.filename + "]");
                }
            } else {
                if (name_len < ext_len) return true;

                if (strcasecmp(entry.filename + name_len - ext_len, extension) != 0) {
                    return true;
                }

                StrCopy(wname, entry.filename);
                wname[name_len - ext_len] = 0;
                window.Append(wname);
            }

            return true;
        });
        int listed_end = window.line_count;
        window.Sort(listed_start, listed_end - listed_start);

        window.Append("Exit");

        window.DrawOpen();
        window.Select(true, false);
        window.DrawClose();

        if (window.line_pos == (window.line_count - 1) || window.rejected) {
            if (past_path != nullptr) {
                path_fs->set_current_path(past_path);
                free(past_path);
            }
            return false;            
        } else if (!StrEmpty(window.hyperlink)) {
            // directory
            if (path_fs != nullptr) {
                path_fs->open_dir(window.hyperlink);
            }
        } else {
            // file
            StrCopy(filename, window.lines[window.line_pos]->c_str());
            if (past_path != nullptr) {
                free(past_path);
            }
            return true;
        }
    }
}