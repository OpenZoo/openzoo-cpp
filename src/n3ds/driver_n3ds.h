#ifndef __DRIVER_N3DS_H__
#define __DRIVER_N3DS_H__

#include <cstdint>
#include "driver.h"
#include "audio_simulator.h"
#include "gpu_n3ds.h"
#include "sound_n3ds.h"
#include "user_interface.h"
#include "user_interface_osk.h"

namespace ZZT {
    class N3DSDriver: public Driver {
	private:
		friend void n3ds_audio_callback(int16_t *samples, int len, void *userdata);
		friend void timerThreadProc(void *userdata);

        uint16_t hsecs;
		AudioSimulator<uint16_t> *soundSimulator;
		NDSPStreamWrapper *soundStream;
		N3DSShader *textShader;
		N3DSCharset *font_6x8, *font_8x8;
        C3D_RenderTarget *tgt_top0, *tgt_top1, *tgt_bottom;
		N3DSTextLayer *mainLayer, *mainLayer40, *currentLayer;
		Handle pitTimer;
		LightLock videoLock;

    public:
        OnScreenKeyboard keyboard;

        N3DSDriver();

        void install(void);
        void uninstall(void);
		void draw_frame(void);
		void update_joy(void);
		void on_pit_tick(void);

		UserInterface *create_user_interface(Game &game);

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
    };
}

#endif
