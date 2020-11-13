#ifndef __DRIVER_NULL_H__
#define __DRIVER_NULL_H__

#include <cstdint>
#include "driver.h"
#include "user_interface_osk.h"

namespace ZZT {
    class GBADriver: public Driver {
        friend void irq_vcount(void);
        friend void irq_vblank(void);
        friend void irq_timer_pit(void);

    public:
        OnScreenKeyboard keyboard;
        
        GBADriver();

        void install(void);
        void uninstall(void);

        // required (input)
        void update_input(void) override;
        void set_text_input(bool enabled, InputPromptMode mode) override;

        // required (sound)
        uint16_t get_hsecs(void) override;
        void delay(int ms) override;
        void idle(IdleMode mode) override;
        void sound_stop(void) override;

        // required (video)
        void draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) override;
        void read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) override;
        void get_video_size(int16_t &width, int16_t &height) override;

        // internal
        void update_joy();
    };
}

#endif