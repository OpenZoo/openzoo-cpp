#ifndef __INPUT_H__
#define __INPUT_H__

#include <cstdint>

namespace ZZT {
    typedef enum {
        KeyBackspace = 8,
        KeyTab = 9,
        KeyEnter = 13,
        KeyCtrlY = 25,
        KeyEscape = 27,
        KeyAltP = 153,
        KeyF1 = 187,
        KeyF2 = 188,
        KeyF3 = 189,
        KeyF4 = 190,
        KeyF5 = 191,
        KeyF6 = 192,
        KeyF7 = 193,
        KeyF8 = 194,
        KeyF9 = 195,
        KeyF10 = 196,
        KeyUp = 200,
        KeyPageUp = 201,
        KeyLeft = 203,
        KeyRight = 205,
        KeyDown = 208,
        KeyPageDown = 209,
        KeyInsert = 210,
        KeyDelete = 211,
        KeyHome = 212,
        KeyEnd = 213
    } Keys;

    class InputDriver {
    protected:
        InputDriver(void);

    public:
        int16_t deltaX, deltaY;
        bool shiftPressed;
        bool shiftAccepted;
        bool joystickEnabled;
        bool mouseEnabled;
        bool joystickMoved;

        uint16_t keyPressed;
        bool keyLeftShiftHeld;
        bool keyRightShiftHeld;
        bool keyShiftHeld;
        bool keyCtrlHeld;
        bool keyAltHeld;
        bool keyNumLockHeld;

        // required
        virtual bool configure(void) = 0;
        virtual void install(void) = 0;
        virtual void uninstall(void) = 0;
        virtual void update_input(void) = 0;
        virtual void read_wait_key(void) = 0;
    };
}

#endif