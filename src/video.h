#ifndef __VIDEO_H__
#define __VIDEO_H__

#include <cstdint>

namespace ZZT {
    class VideoCopy {
    private:
        uint8_t _width, _height;
        uint8_t *data;
    public:
        VideoCopy(uint8_t width, uint8_t height);
        ~VideoCopy();

        uint8_t width(void) { return _width; }
        uint8_t height(void) { return _height; }

        void get(uint8_t x, uint8_t y, uint8_t &col, uint8_t &chr);
        void set(uint8_t x, uint8_t y, uint8_t col, uint8_t chr);
    };

    class VideoDriver {
    public:
        bool monochrome;

        // required
        virtual bool configure(void) = 0;
        virtual void install(void) = 0;
        virtual void uninstall(void) = 0;
        virtual void draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) = 0;
        virtual void read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) = 0;

        // optional
        virtual void draw_string(int16_t x, int16_t y, uint8_t col, const char* text);
        virtual void clrscr(void);
        virtual void set_cursor(bool value);
        virtual void set_border_color(uint8_t value);
        virtual void copy_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY);
        virtual void paste_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY);
    };
}

#endif