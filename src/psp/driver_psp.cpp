#include <cstddef>
#include <cstring>
#include <ctime>
#include <unistd.h>
#include "assets.h"
#include "driver_psp.h"
#include "filesystem_posix.h"

extern "C" {
	#include "6x10_psp.h"
}

#include <pspkernel.h>
#include <pspgu.h>
#include <pspgum.h>
#include <pspdisplay.h>
#include <pspctrl.h>
#include <pspaudio.h>
#include <pspaudiolib.h>
#include <psppower.h>
#include <psputility.h>

PSP_MODULE_INFO("OpenZoo", PSP_MODULE_USER, 1, 0);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU);
PSP_HEAP_SIZE_KB(-1024);

using namespace ZZT;

#include "gamevars.h"

#define BLINK_TOGGLE_DURATION_MS 266
#define FRAME_Y_OFFSET ((272 - 250) / 2)

static const uint32_t ega_palette[16] = {
    0x000000,
    0x0000AA,
    0x00AA00,
    0x00AAAA,
    0xAA0000,
    0xAA00AA,
    0xAA5500,
    0xAAAAAA,
    0x555555,
    0x5555FF,
    0x55FF55,
    0x55FFFF,
    0xFF5555,
    0xFF55FF,
    0xFFFF55,
    0xFFFFFF
};

typedef struct {
	u32 color;
	u16 x, y, z, v_pad;
} point_bg;

typedef struct {
	u16 u, v;
	u32 color;
	u16 x, y, z, v_pad;
} point_fg;

static Game *game;
static PSPDriver driver = PSPDriver();

static uint32_t __attribute__((aligned(16))) gu_clut4[16];
static uint32_t __attribute__((aligned(16))) gu_list[262144];
static uint8_t __attribute__((aligned(16))) gu_char_texture[256 * 128 / 2];

static long psp_time_ms(void) {
	clock_t c = clock();
	return c / (CLOCKS_PER_SEC/1000);
}

static int psp_exit_callback(int a1, int a2, void *a3) {
	sceKernelExitGame();
	driver.running = false;
	return 0;
}

static int psp_exit_thread(SceSize args, void *argp) {
	int cbid = sceKernelCreateCallback("exit callback", psp_exit_callback, NULL);
	sceKernelRegisterExitCallback(cbid);
	sceKernelSleepThreadCB();
	return 0;
}

static int start_thread(const char *name, SceKernelThreadEntry entry, int priority, int stack_size) {
	int thid = sceKernelCreateThread(name, entry, priority, stack_size, PSP_THREAD_ATTR_USER | PSP_THREAD_ATTR_VFPU, nullptr);
	if (thid >= 0) {
		sceKernelStartThread(thid, 0, 0);
	} else {
		sceKernelExitGame();
	}
	return thid;
}

void PSPDriver::update_joy(SceCtrlData &pad) {
	set_joy_button_state(JoyButtonUp, (pad.Buttons & PSP_CTRL_UP) != 0, true);
	set_joy_button_state(JoyButtonDown, (pad.Buttons & PSP_CTRL_DOWN) != 0, true);
	set_joy_button_state(JoyButtonLeft, (pad.Buttons & PSP_CTRL_LEFT) != 0, true);
	set_joy_button_state(JoyButtonRight, (pad.Buttons & PSP_CTRL_RIGHT) != 0, true);
	set_joy_button_state(JoyButtonA, (pad.Buttons & PSP_CTRL_CIRCLE) != 0, true);
	set_joy_button_state(JoyButtonB, (pad.Buttons & PSP_CTRL_CROSS) != 0, true);
	set_joy_button_state(JoyButtonX, (pad.Buttons & PSP_CTRL_TRIANGLE) != 0, true);
	set_joy_button_state(JoyButtonY, (pad.Buttons & PSP_CTRL_SQUARE) != 0, true);
	set_joy_button_state(JoyButtonL, (pad.Buttons & PSP_CTRL_LTRIGGER) != 0, true);
	set_joy_button_state(JoyButtonR, (pad.Buttons & PSP_CTRL_RTRIGGER) != 0, true);
	set_joy_button_state(JoyButtonSelect, (pad.Buttons & PSP_CTRL_SELECT) != 0, true);
	set_joy_button_state(JoyButtonStart, (pad.Buttons & PSP_CTRL_START) != 0, true);
}

void ZZT::psp_audio_callback(void *stream, unsigned int len, void *userdata) {
	/* if (psp_osk_open) {
		memset(stream, 0, len*sizeof(s16)*2);
		return;
	} */

	u8 *stream_u8 = ((u8*) stream) + (len * 3);
	s16 *stream_s16 = ((s16*) stream);

    driver.sound_lock();
    driver.soundSimulator->simulate(stream_u8, len);
    driver.sound_unlock();

	for (int i = 0; i < len; i++, stream_u8++, stream_s16+=2) {
		u16 val = (((u16) *stream_u8) << 8) ^ 0x8000;
		stream_s16[0] = (s16) val;
		stream_s16[1] = (s16) val;
	}
}

void PSPDriver::set_text_input(bool enabled, InputPromptMode mode) {
	if (enabled) {
		keyboard.open(-1, 2, mode);
	} else {
		keyboard.close();
	}
}

void PSPDriver::update_input(void) {
    advance_input();
	if (keyboard.opened()) {
		keyboard.update();
		keyPressed = keyboard.key_pressed;
	}
}

SceUInt ZZT::psp_timer_callback(SceUID uid, SceInt64 requested, SceInt64 actual, void *args) {
	driver.hsecs += 11;
    driver.soundSimulator->allowed = driver._queue.is_playing;

	return !driver.running ? 0 : sceKernelUSec2SysClockWide(driver.pit_clock);
}

uint16_t PSPDriver::get_hsecs(void) {
    return hsecs >> 1;
}

void PSPDriver::delay(int ms) {
	sceKernelDelayThreadCB(ms * 1000);
}

void PSPDriver::idle(IdleMode mode) {
	if (mode == IMYield) {
		sceKernelDelayThreadCB(0);
		return;
	}
	sceDisplayWaitVblankCB();
}

void PSPDriver::sound_stop(void) {
    sound_lock();
    soundSimulator->clear();
    sound_unlock();
}

void PSPDriver::sound_lock(void) {
	sceKernelWaitSema(audio_mutex, 1, nullptr);
}

void PSPDriver::sound_unlock(void) {
	sceKernelSignalSema(audio_mutex, 1);
}

void PSPDriver::draw_frame(void) {
	sceGuStart(GU_DIRECT, gu_list);

	// draw 2000 BG cells
	point_bg *bg_cells = (point_bg *) sceGuGetMemory(sizeof(point_bg) * 4000);
	// draw 2000 FG cells
	point_fg *fg_cells = (point_fg *) sceGuGetMemory(sizeof(point_fg) * 4000);
	point_bg *bg_cells_origin = bg_cells;
	point_fg *fg_cells_origin = fg_cells;

	int i = 0;
	int should_blink = ((psp_time_ms() % (BLINK_TOGGLE_DURATION_MS*2)) >= BLINK_TOGGLE_DURATION_MS);

	uint8_t *chr_ptr = screen_chars;
	uint8_t *col_ptr = screen_colors;

	for (int y = 0; y < 25; y++) {
		uint16_t cy0 = y*10+FRAME_Y_OFFSET;
		uint16_t cy1 = (y+1)*10+FRAME_Y_OFFSET;
		for (int x = 0; x < 80; x++, i+=2, chr_ptr++, col_ptr++) {
			uint8_t chr = *chr_ptr;
			uint8_t col = *col_ptr;
			int should_blink_now = 0;
			if (col >= 0x80) {
				col &= 0x7F;
				should_blink_now = should_blink;
			}
			u32 bg_col = ega_palette[col >> 4] | 0xFF000000;
			u32 fg_col = ega_palette[col & 0xF] | 0xFF000000;

			// TODO: Replace with some kind of palette setting.
			bg_col = (bg_col & 0xFF00FF00) | ((bg_col & 0xFF) << 16) | ((bg_col & 0xFF0000) >> 16);
			fg_col = (fg_col & 0xFF00FF00) | ((fg_col & 0xFF) << 16) | ((fg_col & 0xFF0000) >> 16);

			u16 cx0 = x*6;
			u16 cx1 = (x+1)*6;
			u32 cu = (chr & 31)<<3;
			u32 cv = (chr >> 5)<<4;

			bg_cells[0].color = bg_col;
			bg_cells[0].x = cx0;
			bg_cells[0].y = cy0;
			bg_cells[0].z = 0;
			bg_cells[1].color = bg_col;
			bg_cells[1].x = cx1;
			bg_cells[1].y = cy1;
			bg_cells[1].z = 0;
			bg_cells += 2;

			if (!should_blink_now && ((col ^ (col >> 4)) & 0x0F) && chr != 0 && chr != 32) {
				fg_cells[0].u = cu;
				fg_cells[0].v = cv;
				fg_cells[0].color = fg_col;
				fg_cells[0].x = cx0;
				fg_cells[0].y = cy0;
				fg_cells[0].z = 0;
				fg_cells[1].u = cu+6;
				fg_cells[1].v = cv+10;
				fg_cells[1].color = fg_col;
				fg_cells[1].x = cx1;
				fg_cells[1].y = cy1;
				fg_cells[1].z = 0;
				fg_cells += 2;
			}
		}
	}

	sceGuDisable(GU_TEXTURE_2D);
	if (bg_cells != bg_cells_origin)
	 	sceGumDrawArray(GU_SPRITES, GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, (bg_cells - bg_cells_origin), 0, bg_cells_origin);
	sceGuEnable(GU_TEXTURE_2D);
	if (fg_cells != fg_cells_origin)
		sceGumDrawArray(GU_SPRITES, GU_TEXTURE_16BIT | GU_COLOR_8888 | GU_VERTEX_16BIT | GU_TRANSFORM_2D, (fg_cells - fg_cells_origin), 0, fg_cells_origin);

	sceGuFinish();
	sceGuSync(0, 0);

	sceDisplayWaitVblankStartCB();
	sceGuSwapBuffers();
}

void PSPDriver::draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) {
	int offset = (y * 80 + x);
	screen_colors[offset] = col;
	screen_chars[offset] = chr;
}

void PSPDriver::read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) {
	int offset = (y * 80 + x);
	col = screen_colors[offset];
	chr = screen_chars[offset];
}

void PSPDriver::get_video_size(int16_t &width, int16_t &height) {
	width = 80;
	height = 25;
}

PSPDriver::PSPDriver(void) {
	this->soundSimulator = new AudioSimulator<uint8_t>(&_queue, 44100, false);
}

PSPDriver::~PSPDriver() {
	delete this->soundSimulator;
}

void PSPDriver::install(void) {
	keyboard.driver = this;

	running = true;

	exit_thread_id = start_thread("Exit handler", psp_exit_thread, 0x1E, 0x800);

	pit_clock = sceKernelUSec2SysClockWide(55000);
	pit_timer_id = sceKernelCreateVTimer("PIT timer", nullptr);
	sceKernelSetVTimerHandlerWide(pit_timer_id, pit_clock, psp_timer_callback, nullptr);

	audio_mutex = sceKernelCreateSema("Audio mutex", 0, 1, 1, 0);
	pspAudioInit();
	pspAudioSetChannelCallback(0, psp_audio_callback, NULL);

	memset(gu_clut4, 0, 15 * sizeof(uint32_t));
	gu_clut4[15] = 0xFFFFFFFF;

	memset(gu_char_texture, 0, sizeof(gu_char_texture));
	Charset charset = Charset(256, 6, 10, 1, _6x10_psp_bin);
	Charset::Iterator charsetIterator = charset.iterate();
	while (charsetIterator.next()) {
		if (charsetIterator.value >= 128) {
			int x = (charsetIterator.glyph & 31) * 8 + charsetIterator.x;
			int y = (charsetIterator.glyph >> 5) * 16 + charsetIterator.y;
			uint8_t &line = *(gu_char_texture + (x >> 1) + (y << 7));
			line |= (15 << ((x & 1) << 2));
		}
	}

	sceGuInit();
	sceGuStart(GU_DIRECT, gu_list);
	sceGuDrawBuffer(GU_PSM_8888, NULL, 512);
	sceGuDispBuffer(480, 272, (void*) 0x88000, 512);
	sceGuDepthBuffer((void*) 0x110000, 512);
	sceGuOffset(2048 - 240, 2048 - 136);
	sceGuViewport(2048, 2048, 480, 272);
	sceGuDepthRange(0xC350, 0x2710);
	sceGuScissor(0, 0, 480, 272);
	sceGuEnable(GU_SCISSOR_TEST);
	sceGuDisable(GU_DEPTH_TEST);
	sceGuShadeModel(GU_FLAT);
	sceGuAlphaFunc(GU_GREATER, 0, 0xFF);
	sceGuEnable(GU_ALPHA_TEST);
	sceGuEnable(GU_TEXTURE_2D);
	sceGuClutMode(GU_PSM_8888, 0, 0x0F, 0);
	sceGuClutLoad(2, gu_clut4);
	sceGuTexMode(GU_PSM_T4, 0, 0, 0);
	sceGuTexImage(0, 256, 128, 256, gu_char_texture);
	sceGuTexFunc(GU_TFX_MODULATE, GU_TCC_RGBA);
	sceGuTexEnvColor(0x0);
	sceGuTexOffset(0.0f, 0.0f);
	sceGuTexScale(1.0f / 256.0f, 1.0f / 128.0f);
	sceGuTexWrap(GU_REPEAT, GU_REPEAT);
	sceGuTexFilter(GU_NEAREST, GU_NEAREST);
	sceGuFinish();
	sceGuSync(0, 0);
	sceGuDisplay(GU_TRUE);

	sceKernelStartVTimer(pit_timer_id);
}

void PSPDriver::uninstall(void) {
	sceGuDisplay(GU_FALSE);
	sceGuTerm();

	pspAudioEnd();
	sceKernelDeleteSema(audio_mutex);

	running = false;

	sceKernelStopVTimer(pit_timer_id);
}

static int psp_game_thread(SceSize args, void *argp) {
	game->GameTitleLoop();
	return 0;
}

int main(int argc, char** argv) {
	game = new Game();

	game->driver = &driver;
	game->filesystem = new PosixFilesystemDriver();

	driver.install();

	int game_thread_id = start_thread("Game logic", psp_game_thread, 0x1C, 0x10000);

	// input/render
	SceCtrlData pad;
	while (driver.running) {
		sceCtrlReadBufferPositive(&pad, 1);

		driver.update_joy(pad);
		driver.draw_frame();
	}

	driver.uninstall();

	delete game->filesystem;
	delete game;

	return 0;
}
