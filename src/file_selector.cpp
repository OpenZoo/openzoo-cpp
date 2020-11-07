#include <cstdlib>
#include <sys/stat.h>
#include <dirent.h>

#include "utils.h"
#include "file_selector.h"

using namespace ZZT;
using namespace ZZT::Utils;

FileSelector::FileSelector(VideoDriver *video, InputDriver *input, SoundDriver *sound, const char *title, const char *extension)
    : window(TextWindow(video, input, sound)) {
    this->title = title;
    this->extension = extension;
    StrClear(filename);
}

FileSelector::~FileSelector() {

}

bool FileSelector::select() {
    size_t ext_len = strlen(this->extension);

    while (true) {
        window.Clear();
        StrCopy(window.title, title);
        window.selectable = true;
        StrClear(window.hyperlink);

        DIR *dir;
        struct dirent *entry;
        struct stat st;

        if ((dir = opendir(".")) == NULL) {
            return false;
        }

        while ((entry = readdir(dir)) != NULL) {
            sstring<20> wname;

            stat(entry->d_name, &st);
            if (S_ISDIR(st.st_mode)) continue;

            int name_len = strlen(entry->d_name);
            if (name_len < ext_len) continue;
            if (name_len > StrSize(wname)) continue;

            if (strcasecmp(entry->d_name + name_len - ext_len, extension) != 0) {
                continue;
            }

            StrCopy(wname, entry->d_name);
            wname[name_len - ext_len] = 0;
            window.Append(wname);
        }
        window.Append("Exit");

        window.DrawOpen();
        window.Select(false, false);
        window.DrawClose();

        if (window.line_pos < (window.line_count - 1) && !window.rejected) {
            StrCopy(filename, window.lines[window.line_pos]->c_str());
            return true;
        } else {
            return false;
        }
    }
}

