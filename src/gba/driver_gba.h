#ifndef __DRIVER_GBA_H__
#define __DRIVER_GBA_H__

#include <cstdint>
#include "driver.h"
#include "user_interface_osk.h"

typedef enum : uint8_t {
	FORCE_4X8_MODE_NONE,
	FORCE_4X8_MODE_ALWAYS,
	FORCE_4X8_MODE_READ
} Force4x8ModeType;

void zoo_set_force_4x8_mode(Force4x8ModeType value);
void zoo_gba_sleep();

namespace ZZT {
    class GBADriver: public Driver {
        friend void irq_vblank(void);
        friend void irq_timer_pit(void);

    public:
        OnScreenKeyboard keyboard;
        
        GBADriver();

        void install(void);
        void uninstall(void);

		UserInterface *create_user_interface(Game &game) override;
		void move_chars(int srcX, int srcY, int width, int height, int destX, int destY) override;

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

        // internal
        void update_joy();
    };
}

#endif