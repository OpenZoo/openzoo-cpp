#include <cstddef>
#include <cstring>
#include <pc.h>
#include <bios.h>
#include <conio.h>
#include <dos.h>
#include <dpmi.h>
#include <go32.h>
#include "driver_msdos.h"
#include "filesystem_msdos.h"
#include "gamevars.h"
#include "user_interface_super_zzt.h"

using namespace ZZT;

static MSDOSDriver *driver = nullptr;

void ZZT::timerCallback(void) {
	driver->hsecs += 11;

	if (!driver->_queue.enabled) {
		driver->_queue.is_playing = false;
		nosound();
	} else if (driver->_queue.is_playing) {
		if ((--driver->duration_counter) <= 0) {
			nosound();
			uint16_t note, duration;
			if (!driver->_queue.pop(note, duration)) {
				driver->_queue.is_playing = false;
			} else {
				if (note >= NOTE_MIN && note < NOTE_MAX) {
					sound(sound_notes[note - NOTE_MIN]);
				} else if (note >= DRUM_MIN && note < DRUM_MAX) {
					// TODO: drums
					nosound();
				} else {
					nosound();
				}

				driver->duration_counter = duration;
			}
		}
	}
}

void timerCallbackEnd(void) {

}

MSDOSDriver::MSDOSDriver(void) {

}

void MSDOSDriver::install(void) {
	if (driver == nullptr) {
    	driver = this;
		driver->hsecs = 0;
		driver->duration_counter = 0;

		_go32_dpmi_lock_code((void*) timerCallback, ((int)timerCallbackEnd - (int)timerCallback));
		_go32_dpmi_lock_data(this, sizeof(MSDOSDriver));

		_go32_dpmi_get_protected_mode_interrupt_vector(0x1C, &driver->old_timer_handler);
		driver->new_timer_handler = {
			.pm_offset = (int) timerCallback,
			.pm_selector = _go32_my_cs()
		};
		_go32_dpmi_chain_protected_mode_interrupt_vector(0x1C, &driver->new_timer_handler);

		// init video mode
		
		union REGS r;
		r.w.ax = 0x1201;
		r.h.bl = 0x30;
		int86(0x10, &r, &r);

		textmode(C80);
	}
}

void MSDOSDriver::uninstall(void) {
    if (driver == this) {
		_go32_dpmi_set_protected_mode_interrupt_vector(0x1C, &driver->old_timer_handler);
		driver = nullptr;
	}
}

void MSDOSDriver::update_input(void) {
	uint16_t k = 0;

	// get key
	while (true) {
		uint16_t tk = bioskey(1);
		if (tk != 0) {
			k = bioskey(0);
			if (((k & 0xFF) <= 2 || (k & 0xFF) >= 127) && (k & 0xFF00) != 0) {
				k = 0x80 | (k >> 8);
			} else {
				k &= 0x7F;
			}
		} else {
			break;
		}
	}

	// update keymod
	int kmod = bioskey(2);
	set_key_modifier_state(KeyModRightShift, (kmod & 0x01) != 0);
	set_key_modifier_state(KeyModLeftShift, (kmod & 0x02) != 0);
	set_key_modifier_state(KeyModCtrl, (kmod & 0x04) != 0);
	set_key_modifier_state(KeyModAlt, (kmod & 0x08) != 0);
	set_key_modifier_state(KeyModNumLock, (kmod & 0x20) != 0);

    if (k != 0) set_key_pressed(k, true, true);
	advance_input();
}

uint16_t MSDOSDriver::get_hsecs(void) {
	return driver->hsecs >> 1;
}

static inline void msdos_delay(int ms) {
	delay(ms);
}

void MSDOSDriver::delay(int ms) {
	msdos_delay(ms);
}

void MSDOSDriver::idle(IdleMode mode) {
	__dpmi_yield();
}

void MSDOSDriver::sound_stop(void) {
	nosound();
}

void MSDOSDriver::draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) {
	ScreenPutChar(chr, col, x, y);
}

void MSDOSDriver::read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) {
	int col_i, chr_i;
    ScreenGetChar(&chr_i, &col_i, x, y);
	col = col_i;
	chr = chr_i;
}

static inline void msdos_clrscr(void) {
	return clrscr();
}

void MSDOSDriver::clrscr(void) {
    msdos_clrscr();
}

void MSDOSDriver::set_cursor(bool value) { 	
	union REGS r;
	r.h.ah = 0x01;
	r.w.cx = value ? 0x0607 : 0x2000;
	int86(0x10, &r, &r);
}

void MSDOSDriver::set_border_color(uint8_t value) {
	outportb(0x3D9, value);
}

UserInterface *MSDOSDriver::create_user_interface(Game &game, bool is_editor) {
	union REGS r;
	if (game.engineDefinition.engineType == ENGINE_TYPE_SUPER_ZZT && !is_editor) {
		r.w.ax = 0x0001;
		int86(0x10, &r, &r);
		set_cursor(false);
		return new UserInterfaceSuperZZT(this, 40, 25);
	} else {
		r.w.ax = 0x0003;
		int86(0x10, &r, &r);
		set_cursor(false);
		return new UserInterface(this);
	}
}

#include "gamevars.h"

static Game *game;

int main(int argc, char** argv) {
	MSDOSDriver driver = MSDOSDriver();
	game = new Game();
	
	game->driver = &driver;
    game->filesystem = new MsdosFilesystemDriver();

	driver.install();
	driver.clrscr();

	game->GameTitleLoop();

	// TODO: set video mode back
	driver.clrscr();
	driver.set_cursor(true);

	driver.uninstall();
	
    delete game->filesystem;
	delete game;

	return 0;
}