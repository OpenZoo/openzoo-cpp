#ifndef __ASSETS_H__
#define __ASSETS_H__

#include <cstdint>

namespace ZZT {
    class Charset {
    private:
        uint16_t _glyph_count;
        uint8_t _width;
        uint8_t _height;
        uint8_t _bitdepth;
        const void *_data;

    public:
        class Iterator {
        private:
            Charset *charset;
            const uint8_t *data;
            uint8_t data_pos;

        public:
            int16_t glyph;
            uint8_t x, y;
            uint8_t value;

            Iterator(Charset *charset);
            bool next();
        };
        friend class Iterator;

        Charset(uint16_t glyph_count, uint8_t width, uint8_t height, uint8_t bitdepth, const void *data);
        int width() const { return _width; }
        int height() const { return _height; }
        int bitdepth() const { return _bitdepth; }
        Iterator iterate();
    };
}

#endif