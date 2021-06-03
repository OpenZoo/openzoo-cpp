#include <cstddef>
#include <cstdlib>
#include <cstring>
#include "driver.h"
#include "gamevars.h"
#include "utils/math.h"

using namespace ZZT;

Driver::Driver(void) {
    /* INPUT */
    deltaX = 0;
    deltaY = 0;
    shiftPressed = false;
    shiftAccepted = false;
    joystickEnabled = false;

    keyPressed = 0;
    memset(keys_pressed, 0, sizeof(keys_pressed));
    keys_pressed_pos = 0;
    key_modifiers = 0;
    key_modifiers_new = 0;

    joy_buttons_held = 0;
    joy_buttons_held_new = 0;
    joy_buttons_pressed = 0;
    joy_buttons_pressed_new = 0;
    joy_repeat_hsecs_delay = 25;
    joy_repeat_hsecs_delay_next = 4;
}

/* INPUT */

#define KEY_PRESSED 1
#define KEY_HELD 2

void Driver::set_key_pressed(uint16_t key, bool value, bool one_time) {
    int kid = -1;
    for (int i = 0; i < MAX_KEYS_PRESSED; i++) {
        if (keys_pressed[i].value == key) {
           kid = i;
           break;
        }
    }
    if (kid == -1 && value) {
        for (int i = 0; i < MAX_KEYS_PRESSED; i++) {
            if (keys_pressed[i].flags == 0) {
                kid = i;
                break;
            }
        }
    }
    if (kid >= 0) {
        if (value) {
            keys_pressed[kid].value = key;
            keys_pressed[kid].hsecs = get_hsecs() + joy_repeat_hsecs_delay;
            keys_pressed[kid].flags |= KEY_PRESSED | (one_time ? 0 : KEY_HELD);
        } else {
            keys_pressed[kid].flags &= ~KEY_HELD;
        }
    }
}

void Driver::set_key_modifier_state(KeyModifier modifier, bool value) {
    if (value) {
        key_modifiers_new |= (uint16_t) modifier;
    } else {
        key_modifiers_new &= ~((uint16_t) modifier);
    }
}

bool Driver::set_dpad(bool up, bool down, bool left, bool right) {
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
        default: {
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
        } break;
    }

    return true;
}

bool Driver::set_axis(int32_t axis_x, int32_t axis_y, int32_t axis_min, int32_t axis_max) {
    if (axis_x > axis_min && axis_x < axis_max) axis_x = 0;
    if (axis_y > axis_min && axis_y < axis_max) axis_y = 0;

    if (Abs(axis_x) > Abs(axis_y)) {
        deltaX = Signum(axis_x);
        deltaY = 0;
    } else {
        deltaX = 0;
        deltaY = Signum(axis_y);
    }

    return deltaX != 0 || deltaY != 0;
}

void Driver::set_joy_button_state(JoyButton button, bool value, bool is_constant) {
    if (value) {
        if (is_constant) {
            bool old_value = (joy_buttons_held_new & (1 << button)) != 0;
            if (old_value) return;
        }
        joy_buttons_pressed_new |= (1 << button);
        joy_buttons_held_new |= (1 << button);
    } else {
        joy_buttons_held_new &= ~(1 << button);
    }
}

void Driver::advance_input() {
    uint16_t hsecs = get_hsecs();

    deltaX = 0;
    deltaY = 0;
    shiftPressed = false;

    // keyPressed
    keyPressed = 0;
    for (int i = 0; i < MAX_KEYS_PRESSED; i++, keys_pressed_pos++) {
        int kid = (keys_pressed_pos & (MAX_KEYS_PRESSED - 1));
        if (keys_pressed[kid].flags != 0) {
            if ((keys_pressed[kid].flags & (KEY_HELD | KEY_PRESSED)) == KEY_HELD) {
                // hsecs > joy_button_hsecs when hsecs_diff near max limit
                uint16_t hsecs_diff = keys_pressed[kid].hsecs - hsecs;
                if (hsecs_diff >= 32768) {
                    keys_pressed[kid].hsecs = hsecs + joy_repeat_hsecs_delay_next;
                    keys_pressed[kid].flags |= KEY_PRESSED;
                }
            }
            if (keys_pressed[kid].flags & KEY_PRESSED) {
                keys_pressed[kid].flags &= ~KEY_PRESSED;
                keyPressed = keys_pressed[kid].value;
                keys_pressed_pos++; // try next key on next update_input
                break;
            }
        }
    }
    keys_pressed_pos &= (MAX_KEYS_PRESSED - 1);
    key_modifiers = key_modifiers_new;

    // joy_buttons_pressed_new -> joy_buttons_pressed
    joy_buttons_held = joy_buttons_held_new;
    joy_buttons_pressed = joy_buttons_pressed_new;
    joy_buttons_pressed_new = 0;
    auto pn = joy_buttons_pressed;
    for (int i = 0; i < JoyButtonMax && pn != 0; i++, pn>>=1) {
        if (pn&1) {
            joy_buttons_hsecs[i] = hsecs + joy_repeat_hsecs_delay;
        }
    }

    // only autorepeat the D-pad
    for (int button = 0; button <= JoyButtonRight; button++) {
        if ((joy_buttons_held & (1 << button)) != 0) {
            // hsecs > joy_button_hsecs when hsecs_diff near max limit
            uint16_t hsecs_diff = joy_buttons_hsecs[button] - hsecs;
            if (hsecs_diff >= 32768) {
                joy_buttons_hsecs[button] = hsecs + joy_repeat_hsecs_delay_next;
                joy_buttons_pressed |= (1 << button);
            }
        }
    }

    // deltaX/Y
    set_dpad(
        joy_button_pressed(JoyButtonUp, false),
        joy_button_pressed(JoyButtonDown, false),
        joy_button_pressed(JoyButtonLeft, false),
        joy_button_pressed(JoyButtonRight, false)
    );

    // shiftPressed
    shiftPressed |= key_modifier_held(KeyModShift);
	if (joy_button_held(JoyButtonA, true)) {
		if (!shiftAccepted) {
			shiftPressed = true;
		}
	} else {
		shiftAccepted = false;
	}
}

bool Driver::key_modifier_held(KeyModifier modifier) {
    return (key_modifiers & modifier) != 0;
}

bool Driver::joy_button_pressed(JoyButton button, bool simulate) {
    bool result = (joy_buttons_pressed & (1 << button)) != 0;
    if (!simulate) {
        joy_buttons_pressed &= ~(1 << button);
    }
    return result;
}

bool Driver::joy_button_held(JoyButton button, bool simulate) {
    bool result = (joy_buttons_held & (1 << button)) != 0;
    return result || joy_button_pressed(button, simulate);
}

void Driver::set_text_input(bool enabled, InputPromptMode mode) {
    
}

void Driver::read_wait_key(void) {
    update_input();
    if (keyPressed != 0) return;
 
    do {
        idle(IMUntilFrame);
        update_input();
    } while (keyPressed == 0);
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
	// TOO: de-hardcode
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

void Driver::move_chars(int srcX, int srcY, int width, int height, int destX, int destY) {
	uint8_t col, chr;
	int ix_min = (srcX > destX) ? 0 : width - 1;
	int ix_max = (srcX > destX) ? width : -1;
	int ix_step = (srcX > destX) ? 1 : -1;
	int iy_min = (srcY > destY) ? 0 : height - 1;
	int iy_max = (srcY > destY) ? height : -1;
	int iy_step = (srcY > destY) ? 1 : -1;
    for (int iy = iy_min; iy != iy_max; iy += iy_step) {
        for (int ix = ix_min; ix != ix_max; ix += ix_step) {
            read_char(srcX + ix, srcY + iy, col, chr);
            draw_char(destX + ix, destY + iy, col, chr);
        }
    }
}

void Driver::scroll_chars(int x, int y, int width, int height, int deltaX, int deltaY) {
    if (Abs(deltaX) >= width || Abs(deltaY) >= height) {
        return;
    }
    int copy_width = width - Abs(deltaX);
    int copy_height = height - Abs(deltaY);
    int from_x = (deltaX < 0) ? (x - deltaX) : x;
    int from_y = (deltaY < 0) ? (y - deltaY) : y;
    int to_x = (deltaX < 0) ? x : (x + deltaX);
    int to_y = (deltaY < 0) ? y : (y + deltaY);
	move_chars(from_x, from_y, copy_width, copy_height, to_x, to_y);
}

bool Driver::set_video_size(int16_t width, int16_t height, bool simulate) {
    return false;
}

UserInterface *Driver::create_user_interface(Game &game) {
	return new UserInterface(this);
}
