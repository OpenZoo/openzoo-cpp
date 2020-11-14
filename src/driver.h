#ifndef __DRIVER_H__
#define __DRIVER_H__

#include <cstdint>
#include "sounds.h"

namespace ZZT {
    typedef enum {
        InputPMNumbers = 0,
        InputPMAlphanumeric = 1,
        InputPMAnyText = 2,
        InputPMAnyKey = 3
    } InputPromptMode;

    typedef enum {
        KeyModLeftShift = 1,
        KeyModRightShift = 2,
        KeyModShift = 3,
        KeyModCtrl = 4,
        KeyModAlt = 8,
        KeyModNumLock = 16
    } KeyModifier;

    typedef enum {
        JoyButtonNone = -1,
        JoyButtonUp = 0,
        JoyButtonDown = 1,
        JoyButtonLeft = 2,
        JoyButtonRight = 3,
        JoyButtonA = 4,
        JoyButtonB = 5,
        JoyButtonX = 6,
        JoyButtonY = 7,
        JoyButtonSelect = 8,
        JoyButtonStart = 9,
        JoyButtonL = 10,
        JoyButtonR = 11,
        JoyButtonMax = 12
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

    class VideoCopy {
    private:
        uint8_t _width, _height;
        uint8_t *data;
    public:
        VideoCopy(uint8_t width, uint8_t height);
        ~VideoCopy();

        uint8_t width(void) { return _width; }
        uint8_t height(void) { return _height; }

        void get(uint8_t x, uint8_t y, uint8_t &col, uint8_t &chr);
        void set(uint8_t x, uint8_t y, uint8_t col, uint8_t chr);
    };
 
    class Driver {
    private:
        uint16_t joy_buttons_hsecs[JoyButtonMax];
        uint32_t joy_buttons_pressed_new;
        uint32_t joy_buttons_held_new;
        uint32_t joy_buttons_pressed;
        uint32_t joy_buttons_held;
        uint16_t key_pressed_new;
        uint16_t key_modifiers_new;
        uint16_t key_modifiers;

    protected:
        Driver(void);
        
        /* INPUT */

        void set_key_pressed(uint16_t value);
        void set_key_modifier_state(KeyModifier modifier, bool value);
        bool set_dpad(bool up, bool down, bool left, bool right);
        bool set_axis(int32_t axis_x, int32_t axis_y, int32_t axis_min, int32_t axis_max);
        void set_joy_button_state(JoyButton button, bool value, bool is_constant);
        void advance_input();

        /* SOUND/TIMER */

        SoundQueue _queue;

        /* VIDEO */

    public:

        /* INPUT */

        uint16_t joy_repeat_hsecs_delay;
        uint16_t joy_repeat_hsecs_delay_next;

        int16_t deltaX, deltaY;
        bool shiftPressed;
        bool shiftAccepted;
        bool joystickEnabled;

        uint16_t keyPressed;

        // required
        virtual void update_input(void) = 0;

        // optional
        virtual void set_text_input(bool enabled, InputPromptMode mode); // intended for OSK toggling
        virtual void read_wait_key(void);
        
        bool key_modifier_held(KeyModifier modifier);
        bool joy_button_pressed(JoyButton button, bool simulate);
        bool joy_button_held(JoyButton button, bool simulate);

        /* SOUND/TIMER */

        // required
        virtual uint16_t get_hsecs(void) = 0;
        virtual void delay(int ms) = 0;
        virtual void idle(IdleMode mode) = 0;
        virtual void sound_stop(void) = 0;

        // optional
        virtual void sound_lock(void) { };
        virtual void sound_unlock(void) { };
        virtual void sound_queue(int16_t priority, const uint8_t *pattern, int len) {
            sound_lock();
            _queue.queue(priority, pattern, len);
            sound_unlock();
        }

        void sound_clear_queue(void);
        inline bool sound_is_enabled(void) { return _queue.enabled; }
        inline void sound_set_enabled(bool value) { _queue.enabled = value; }
        inline bool sound_is_block_queueing(void) { return _queue.block_queueing; }
        inline void sound_set_block_queueing(bool value) { _queue.block_queueing = value; }

        // extra
        template<size_t i> inline void sound_queue(int16_t priority, const char (&str)[i]) {
            sound_queue(priority, (const uint8_t*) str, i - 1);
        }

        /* VIDEO */
        bool monochrome;

        // required
        virtual void draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) = 0;
        virtual void read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) = 0;
        virtual void get_video_size(int16_t &width, int16_t &height) = 0;

        // optional
        virtual void draw_string(int16_t x, int16_t y, uint8_t col, const char* text);
        virtual void clrscr(void);
        virtual void set_cursor(bool value);
        virtual void set_border_color(uint8_t value);
        virtual void copy_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY);
        virtual void paste_chars(VideoCopy &copy, int x, int y, int width, int height, int destX, int destY);
        virtual bool set_video_size(int16_t width, int16_t height, bool simulate);
    };
}

#endif