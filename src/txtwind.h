#ifndef __TXTWIND_H__
#define __TXTWIND_H__

#include <cstdint>
#include <string>
#include <vector>
#include "filesystem.h"
#include "driver.h"
#include "utils/stringutils.h"

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
		int16_t window_x, window_y, window_width, window_height;

        int PageMoveHeightLines(void);
        void DrawTitle(uint8_t color, const char *title);
        void DrawLine(int16_t lpos, bool withoutFormatting, bool viewingFile);
        int16_t Edit_DeleteCurrLine(void);

    public:
        bool selectable;
        uint8_t color;
        int16_t line_pos;
        std::vector<std::string> lines;
        sstring<20> hyperlink;
        sstring<50> title;
        sstring<50> loaded_filename;
        VideoCopy *screenCopy;
        bool rejected;

		int16_t line_count(void) const {
			return lines.size();
		}

        TextWindow(Driver *driver, FilesystemDriver *filesystem, int16_t w_x, int16_t w_y, int16_t w_width, int16_t w_height);
        virtual ~TextWindow();

        void Clear(void);
        virtual void DrawOpen(void);
        virtual void DrawClose(void);
        virtual void Draw(bool withoutFormatting, bool viewingFile);
        void Append(const char *line);
        void Append(const std::string line);
        void Select(bool hyperlinkAsSelect, bool viewingFile);
        void Edit(void);
        void OpenFile(const char *filename, bool errorIfMissing);
        void SaveFile(const char *filename);
        void Sort(int16_t start, int16_t count);
    };

    void TextWindowDrawPattern(Driver *driver, int16_t x, int16_t y, int16_t width, uint8_t color, WindowPatternType ptype);
}

#endif
