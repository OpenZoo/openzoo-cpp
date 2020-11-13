#include "assets.h"

using namespace ZZT;

Charset::Charset(uint16_t glyph_count, uint8_t width, uint8_t height, uint8_t bitdepth, const void *data) {
    this->_glyph_count = glyph_count;
    this->_width = width;
    this->_height = height;
    this->_bitdepth = bitdepth;
    this->_data = data;
}

Charset::Iterator Charset::iterate() {
    return Charset::Iterator(this);
}

Charset::Iterator::Iterator(Charset *charset) {
    this->charset = charset;
    this->data = (const uint8_t*) charset->_data;
    this->data_pos = 0;
    this->glyph = -1;
    this->x = 0;
    this->y = 0;
}

bool Charset::Iterator::next() {
    if (glyph < charset->_glyph_count) {
        if (glyph >= 0) {
            x++;
            if (x >= charset->_width) {
                x = 0;
                y++;
                if (y >= charset->_height) {
                    y = 0;
                    glyph++;
                    if (glyph >= charset->_glyph_count) {
                        return false;
                    }
                }
            }
        } else {
            glyph = 0;
        }

        uint8_t shift = charset->_bitdepth;
        uint32_t mask = (1 << shift) - 1;
        value = ((*(this->data)) >> this->data_pos) & mask;
        value = (value << (8 / shift)) - value; // 1, 4 => 15...
        this->data_pos += shift;
        if (this->data_pos >= 8) {
            this->data_pos = 0;
            this->data++;
        }

        return true;
    } else {
        return false;
    }
}