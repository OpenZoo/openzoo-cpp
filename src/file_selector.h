#ifndef __FILE_SELECTOR_H__
#define __FILE_SELECTOR_H__

#include "txtwind.h"

namespace ZZT {
    class FileSelector {
        FilesystemDriver *filesystem;
        const char *title;
        const char *extension;
        TextWindow *window;
        sstring<64> filename;

    public:
        FileSelector(TextWindow *window, FilesystemDriver *filesystem, const char *title, const char *extension);
        ~FileSelector();

        inline const char *get_filename() {
            return filename;
        }

        bool select();
    };
}

#endif