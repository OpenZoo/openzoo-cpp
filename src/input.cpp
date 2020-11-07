#include <cstddef>
#include <cstdio>
#include <cstring>
#include "input.h"
#include "utils.h"

using namespace ZZT;
using namespace ZZT::Utils;

InputDriver::InputDriver(void) {
    deltaX = 0;
    deltaY = 0;
    shiftPressed = false;
    shiftAccepted = false;
    joystickEnabled = false;
    keyPressed = 0;
    joystickMoved = false;

    joy_buttons_held = 0;
    joy_buttons_pressed = 0;
}

void InputDriver::set_key_pressed(uint16_t k) {
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

bool InputDriver::set_dpad(bool up, bool down, bool left, bool right) {
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

bool InputDriver::set_axis(int32_t axis_x, int32_t axis_y, int32_t axis_min, int32_t axis_max) {
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

void InputDriver::set_joy_button_state(JoyButton button, bool value) {
    if (value) {
        joy_buttons_pressed |= button;
        joy_buttons_held |= button;
    } else {
        joy_buttons_pressed &= ~button;
        joy_buttons_held &= ~button;
    }
}

bool InputDriver::joy_button_pressed(JoyButton button, bool simulate) {
    bool result = (joy_buttons_pressed & button) != 0;
    if (!simulate) {
        joy_buttons_pressed &= ~button;
    }
    return result;
}

bool InputDriver::joy_button_held(JoyButton button) {
    bool result = (joy_buttons_held & button) != 0;
    return result;
}