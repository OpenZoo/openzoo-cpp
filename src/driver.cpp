#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "driver.h"
#include "utils.h"

using namespace ZZT;
using namespace ZZT::Utils;

Driver::Driver(void) {
    /* INPUT */
    deltaX = 0;
    deltaY = 0;
    shiftPressed = false;
    shiftAccepted = false;
    joystickEnabled = false;
    keyPressed = 0;
    joystickMoved = false;

    joy_buttons_held = 0;
    joy_buttons_pressed = 0;

    /* SOUND/TIMER */

    /* VIDEO */
    monochrome = false;
}

/* INPUT */

void Driver::set_key_pressed(uint16_t k) {
    keyPressed = k;
    switch (keyPressed) {
        case KeyUp:
        case '8': {
            deltaX = 0;
            deltaY = -1;
        } break;
        case KeyLeft:
        case '4': {
            deltaX = -1;
            deltaY = 0;
        } break;
        case KeyRight:
        case '6': {
            deltaX = 1;
            deltaY = 0;
        } break;
        case KeyDown:
        case '2': {
            deltaX = 0;
            deltaY = 1;
        } break;
    }
}

bool Driver::set_dpad(bool up, bool down, bool left, bool right) {
    if (up) {
        deltaX = 0;
        deltaY = -1;
    } else if (left) {
        deltaX = -1;
        deltaY = 0;
    } else if (right) {
        deltaX = 1;
        deltaY = 0;
    } else if (down) {
        deltaX = 0;
        deltaY = 1;
    } else {
        return false;
    }

    joystickMoved = true;
    return true;
}

bool Driver::set_axis(int32_t axis_x, int32_t axis_y, int32_t axis_min, int32_t axis_max) {
    if (axis_x > axis_min && axis_x < axis_max) axis_x = 0;
    if (axis_y > axis_min && axis_y < axis_max) axis_y = 0;

    if (Abs(axis_x) > Abs(axis_y)) {
        deltaX = Signum(axis_x);
        deltaY = 0;
        joystickMoved = deltaX != 0;
    } else {
        deltaX = 0;
        deltaY = Signum(axis_y);
        joystickMoved = deltaY != 0;
    }

    return deltaX != 0 || deltaY != 0;
}

void Driver::set_joy_button_state(JoyButton button, bool value) {
    if (value) {
        joy_buttons_pressed |= button;
        joy_buttons_held |= button;
    } else {
        joy_buttons_pressed &= ~button;
        joy_buttons_held &= ~button;
    }
}

bool Driver::joy_button_pressed(JoyButton button, bool simulate) {
    bool result = (joy_buttons_pressed & button) != 0;
    if (!simulate) {
        joy_buttons_pressed &= ~button;
    }
    return result;
}

bool Driver::joy_button_held(JoyButton button) {
    bool result = (joy_buttons_held & button) != 0;
    return result;
}

/* SOUND/TIMER */

void Driver::sound_clear_queue(void) {
    sound_lock();
    _queue.clear();
    sound_unlock();
    sound_stop();
}

/* VIDEO */

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

void Driver::draw_string(int16_t x, int16_t y, uint8_t col, const char* text) {
    size_t len = strlen(text);
    for (size_t i = 0; i < len; i++) {
        // TODO: handle wrap?
        draw_char(x + i, y, col, text[i]);
    }
}

void Driver::clrscr(void) {
    for (int16_t y = 0; y < 25; y++) {
        for (int16_t x = 0; x < 80; x++) {
            draw_char(x, y, 0, 0);
        }
    }
}

void Driver::set_cursor(bool value) {
    // stub
}

void Driver::set_border_color(uint8_t value) {
    // stub
}

void Driver::copy_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY) {
    uint8_t col, chr;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            read_char(x + ix, y + iy, col, chr);
            copy.set(destX + ix, destY + iy, col, chr);
        }
    }
}

void Driver::paste_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY) {
uint8_t col, chr;
    for (int iy = 0; iy < height; iy++) {
        for (int ix = 0; ix < width; ix++) {
            copy.get(x + ix, y + iy, col, chr);
            draw_char(destX + ix, destY + iy, col, chr);
        }
    }
}
