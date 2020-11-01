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