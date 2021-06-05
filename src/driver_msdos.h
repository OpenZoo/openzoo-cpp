#ifndef __DRIVER_MSDOS_H__
#define __DRIVER_MSDOS_H__

#include <cstdint>
#include "driver.h"

namespace ZZT {
    class MSDOSDriver: public Driver {
        friend void timerCallback(void);

    private:
        uint16_t hsecs;
        int16_t duration_counter;
        _go32_dpmi_seginfo old_timer_handler;
        _go32_dpmi_seginfo new_timer_handler;
        
    public:
        MSDOSDriver();

        void install(void);
        void uninstall(void);

		UserInterface *create_user_interface(Game &game, bool is_editor);

        // required (input)
        void update_input(void) override;

        // required (sound)
        uint16_t get_hsecs(void) override;
        void delay(int ms) override;
        void idle(IdleMode mode) override;
        void sound_stop(void) override;

        // required (video)
        void draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) override;
        void read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) override;
        void clrscr(void) override;
        void set_cursor(bool value) override;
        void set_border_color(uint8_t value) override;
    };
}

#endif