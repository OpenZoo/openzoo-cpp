#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "video.h"

using namespace ZZT;

VideoCopy::VideoCopy(uint8_t width, uint8_t height) {
    this->_width = width;
    this->_height = height;
    this->data = (uint8_t*) malloc((int) width * (int) height * 2 * sizeof(uint8_t));
}

VideoCopy::~VideoCopy() {
    free(this->data);
}

void VideoCopy::get(uint8_t x, uint8_t y, uint8_t &col, uint8_t &chr) {
    if (x >= 0 && y >= 0 && x < _width && y < _height) {
        int offset = (y * _width + x) << 1;
        chr = this->data[offset++];
        col = this->data[offset];
    } else {
        col = 0;
        chr = 0;
    }
}

void VideoCopy::set(uint8_t x, uint8_t y, uint8_t col, uint8_t chr) {
    if (x >= 0 && y >= 0 && x < _width && y < _height) {
        int offset = (y * _width + x) << 1;
        this->data[offset++] = chr;
        this->data[offset] = col;
    }
}

void VideoDriver::draw_string(int16_t x, int16_t y, uint8_t col, const char* text) {
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        // TODO: handle wrap?
        draw_char(x + i, y, col, text[i]);
    }
}

void VideoDriver::clrscr(void) {
    for (int16_t y = 0; y < 25; y++) {
        for (int16_t x = 0; x < 80; x++) {
            draw_char(x, y, 0, 0);
        }
    }
}

void VideoDriver::set_cursor(bool value) {
    // stub
}

void VideoDriver::set_border_color(uint8_t value) {
    // stub
}

void VideoDriver::copy_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY) {
    uint8_t col, chr;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            read_char(x + ix, y + iy, col, chr);
            copy.set(destX + ix, destY + iy, col, chr);
        }
    }
}

void VideoDriver::paste_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY) {
uint8_t col, chr;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            copy.get(x + ix, y + iy, col, chr);
            draw_char(destX + ix, destY + iy, col, chr);
        }
    }
}
