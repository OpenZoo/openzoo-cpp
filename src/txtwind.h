#ifndef __TXTWIND_H__
#define __TXTWIND_H__

#include <cstdint>
#include <string>
#include "input.h"
#include "sounds.h"
#include "utils.h"
#include "video.h"

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
        InputDriver *input;
        VideoDriver *video;
        SoundDriver *sound;

        void DrawTitle(uint8_t color, const char *title);
        void DrawLine(int16_t lpos, bool withoutFormatting, bool viewingFile);
        int16_t Edit_DeleteCurrLine(void);

    public:
        bool selectable;
        int16_t line_count;
        int16_t line_pos;
        std::string *lines[MAX_TEXT_WINDOW_LINES];
        sstring<20> hyperlink;
        sstring<50> title;
        sstring<50> loaded_filename;
        VideoCopy *screenCopy;
        bool rejected;

        TextWindow(VideoDriver *video, InputDriver *input, SoundDriver *sound);
        ~TextWindow();

        void Clear(void);
        void DrawOpen(void);
        void DrawClose(void);
        void Draw(bool withoutFormatting, bool viewingFile);
        void Append(const char *line);
        void Append(const std::string line);
        void Select(bool hyperlinkAsSelect, bool viewingFile);
        void Edit(void);
        void OpenFile(const char *filename, bool errorIfMissing);
        void SaveFile(const char *filename);
    };

    void TextWindowDrawPattern(VideoDriver *video, int16_t x, int16_t y, int16_t width, uint8_t color, WindowPatternType ptype);
    void TextWindowDisplayFile(VideoDriver *video, InputDriver *input, SoundDriver *sound, const char *filename, const char *title);
    void TextWindowInit(int16_t x, int16_t y, int16_t width, int16_t height);
}

#endif