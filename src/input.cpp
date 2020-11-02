#include <cstddef>
#include <cstring>
#include "input.h"

using namespace ZZT;

InputDriver::InputDriver(void) {
    deltaX = 0;
    deltaY = 0;
    shiftPressed = false;
    shiftAccepted = false;
    joystickEnabled = false;
    mouseEnabled = false;
    keyPressed = 0;
    joystickMoved = false;
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