#ifndef __TXTWIND_H__
#define __TXTWIND_H__

#include <cstdint>
#include "filesystem.h"
#include "driver.h"
#include "utils/strings.h"

#define MAX_TEXT_WINDOW_LINES 1024
#define TEXT_WINDOW_LINE_LENGTH 50

namespace ZZT {
    typedef enum {
        WinPatTop,
        WinPatBottom,
        WinPatInner,
        WinPatSeparator
    } WindowPatternType;

    class TextWindow {
    private:
        void DrawBorderLine(int16_t y, WindowPatternType ptype);

    protected:
        Driver *driver;
        FilesystemDriver *filesystem;

        int PageMoveHeightLines(void);
        void DrawTitle(uint8_t color, const char *title);
        void DrawLine(int16_t lpos, bool withoutFormatting, bool viewingFile);
        int16_t Edit_DeleteCurrLine(void);

    public:
        bool selectable;
        uint8_t color;
        int16_t line_count;
        int16_t line_pos;
        DynString **lines;
        sstring<20> hyperlink;
        sstring<50> title;
        sstring<50> loaded_filename;
        VideoCopy *screenCopy;
        bool rejected;

        TextWindow(Driver *driver, FilesystemDriver *filesystem);
        ~TextWindow();

        void Clear(void);
        void DrawOpen(void);
        void DrawClose(void);
        void Draw(bool withoutFormatting, bool viewingFile);
        void Append(const char *line);
        void Append(const DynString line);
        void Select(bool hyperlinkAsSelect, bool viewingFile);
        void Edit(void);
        void OpenFile(const char *filename, bool errorIfMissing);
        void SaveFile(const char *filename);
        void Sort(int16_t start, int16_t count);
    };

    void TextWindowDrawPattern(Driver *driver, int16_t x, int16_t y, int16_t width, uint8_t color, WindowPatternType ptype);
    void TextWindowDisplayFile(Driver *driver, FilesystemDriver *filesystem, const char *filename, const char *title);
    void TextWindowInit(int16_t x, int16_t y, int16_t width, int16_t height);
}

#endif