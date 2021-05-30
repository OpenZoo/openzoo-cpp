#ifndef __USER_INTERFACE_OSK_H__
#define __USER_INTERFACE_OSK_H__

#include <cstdint>
#include "driver.h"
#include "user_interface.h"

namespace ZZT {
    struct OSKEntry {
        uint8_t x, y;
        uint16_t char_regular;
        uint16_t char_shifted;
    };

    struct OSKLayout {
        uint8_t width, height;
        const OSKEntry *entries;
        uint8_t entry_count;
    };

    class OnScreenKeyboard {
    private:
        int16_t x, y;
        const OSKLayout *layout;
        VideoCopy *backup;

        uint8_t e_pos;
        bool shifted;

        void draw(bool redraw);

    public:
        uint8_t key_pressed;
        Driver *driver;

        OnScreenKeyboard();
        ~OnScreenKeyboard();

        inline bool opened() const { return backup != nullptr; }
        void open(int16_t x, int16_t y, InputPromptMode mode);
        void update(void);
        void close(void);
    };
}

#endif