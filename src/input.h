#ifndef __INPUT_H__
#define __INPUT_H__

#include <cstdint>

namespace ZZT {
    typedef enum {
        JoyButtonNone = 0,
        JoyButtonUp = 1 << 0,
        JoyButtonDown = 1 << 1,
        JoyButtonLeft = 1 << 2,
        JoyButtonRight = 1 << 3,
        JoyButtonA = 1 << 4,
        JoyButtonB = 1 << 5,
        JoyButtonX = 1 << 6,
        JoyButtonY = 1 << 7,
        JoyButtonSelect = 1 << 8,
        JoyButtonStart = 1 << 9,
        JoyButtonL = 1 << 10,
        JoyButtonR = 1 << 11
    } JoyButton;

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
        
        void set_key_pressed(uint16_t value);
        bool set_dpad(bool up, bool down, bool left, bool right);
        bool set_axis(int32_t axis_x, int32_t axis_y, int32_t axis_min, int32_t axis_max);
        void set_joy_button_state(JoyButton button, bool value);

        uint32_t joy_buttons_pressed;
        uint32_t joy_buttons_held;

    public:
        int16_t deltaX, deltaY;
        bool shiftPressed;
        bool shiftAccepted;
        bool joystickEnabled;
        bool joystickMoved;

        uint16_t keyPressed;
        bool keyLeftShiftHeld;
        bool keyRightShiftHeld;
        bool keyShiftHeld;
        bool keyCtrlHeld;
        bool keyAltHeld;
        bool keyNumLockHeld;

        // required
        virtual void update_input(void) = 0;
        virtual void read_wait_key(void) = 0;

        // optional
        virtual bool joy_button_pressed(JoyButton button, bool simulate);
        virtual bool joy_button_held(JoyButton button);
    };
}

#endif