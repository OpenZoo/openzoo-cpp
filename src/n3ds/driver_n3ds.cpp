#include <cstddef>
#include <cstring>
#include <3ds.h>
#include <citro3d.h>
#include "audio_simulator.h"
#include "driver_n3ds.h"
#include "filesystem_posix.h"
#include "gpu_n3ds.h"
#include "user_interface_slim.h"
#include "user_interface_super_zzt.h"
extern "C" {
	#include "6x8_bin.h"
	#include "8x8_bin.h"
	#include "shader_text_shbin.h"
}

using namespace ZZT;

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

static Game *game;
static bool gameRunning = true;

N3DSDriver::N3DSDriver(void) {
	this->soundSimulator = new AudioSimulator<uint16_t>(&this->_queue, 48000, false);
}

void ZZT::n3ds_audio_callback(int16_t *samples, int len, void *userdata) {
	if (!gameRunning) {
		memset(samples, 0, len * 2);
		return;
	}

	N3DSDriver *driver = (N3DSDriver*) userdata;
	driver->soundSimulator->simulate((uint16_t*) samples, len);
	for (int i = 0; i < len; i++) {
		samples[i] ^= 0x8000;
	}
}

void N3DSDriver::install(void) {
	// Video
	
	gfxSet3D(false);
	C3D_Init(0x40000);
	C3D_CullFace(GPU_CULL_NONE);

	tgt_top0 = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(tgt_top0, C3D_CLEAR_ALL, 0x000000, 0);
	C3D_RenderTargetSetOutput(tgt_top0, GFX_TOP, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8)
		| GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)
	);

	tgt_top1 = C3D_RenderTargetCreate(240, 400, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(tgt_top1, C3D_CLEAR_ALL, 0x000000, 0);
	C3D_RenderTargetSetOutput(tgt_top1, GFX_TOP, GFX_RIGHT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8)
		| GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)
	);

	tgt_bottom = C3D_RenderTargetCreate(240, 320, GPU_RB_RGB8, GPU_RB_DEPTH16);
	C3D_RenderTargetClear(tgt_bottom, C3D_CLEAR_ALL, 0x000000, 0);
	/* C3D_RenderTargetSetOutput(tgt_bottom, GFX_BOTTOM, GFX_LEFT,
		GX_TRANSFER_IN_FORMAT(GX_TRANSFER_FMT_RGB8)
		| GX_TRANSFER_OUT_FORMAT(GX_TRANSFER_FMT_RGB8)
	); */

	Charset charset = Charset(256, 6, 8, 1, _6x8_bin);
    this->font_6x8 = new N3DSCharset(charset);
	charset = Charset(256, 8, 8, 1, _8x8_bin);
	this->font_8x8 = new N3DSCharset(charset);
	this->textShader = new N3DSShader(shader_text_shbin, shader_text_shbin_size, 3);
	this->textShader->addType(GPU_SHORT, 4); // position
	this->textShader->addType(GPU_SHORT, 2); // UV
	this->textShader->addType(GPU_UNSIGNED_BYTE, 4); // color

	C3D_AlphaTest(false, GPU_GREATER, 0x80);
	C3D_DepthTest(false, GPU_GEQUAL, GPU_WRITE_ALL);

	this->mainLayer = new N3DSTextLayer(this->textShader, this->font_6x8, 60, 26, ega_palette, false);
	this->mainLayer40 = new N3DSTextLayer(this->textShader, this->font_8x8, 50, 30, ega_palette, false);
	this->currentLayer = this->mainLayer;

	LightLock_Init(&videoLock);

	// Sound

	ndspInit();
	ndspSetOutputMode(NDSP_OUTPUT_STEREO);
	ndspSetOutputCount(1);
	ndspSetMasterVol(1.0f);
	this->soundStream = new NDSPStreamWrapper(n3ds_audio_callback, (void*) this, 0, 1, 48000, 2048);

	// Timer

	svcCreateTimer(&pitTimer, RESET_PULSE);	
	s64 pitTime = 54924563;
	svcSetTimer(pitTimer, pitTime, pitTime);
}

void N3DSDriver::uninstall(void) {
	// Timer

	svcCancelTimer(pitTimer);

	// Sound

	ndspExit();
	delay(100);
	delete this->soundStream;

	// Video

	delete this->mainLayer40;
	delete this->mainLayer;

	C3D_RenderTargetDelete(this->tgt_top0);
	C3D_RenderTargetDelete(this->tgt_top1);
	C3D_RenderTargetDelete(this->tgt_bottom);

	delete this->font_8x8;
	delete this->font_6x8;
	delete this->textShader;

	C3D_Fini();
}

void N3DSDriver::set_text_input(bool enabled, InputPromptMode mode) {
	if (enabled) {
		keyboard.open(-1, 2, currentLayer->width(), mode);
	} else {
		keyboard.close();
	}
}

void N3DSDriver::update_input(void) {
    advance_input();
	if (keyboard.opened()) {
		keyboard.update();
		keyPressed = keyboard.key_pressed;
	}
}

uint16_t N3DSDriver::get_hsecs(void) {
    return hsecs >> 1;
}

void N3DSDriver::delay(int ms) {
	svcSleepThread(((s64) ms) * 1000000);
}

void N3DSDriver::idle(IdleMode mode) {
	if (mode == IMYield) {
		svcSleepThread(1);
		return;
	}
	gspWaitForVBlank();
}

void N3DSDriver::sound_stop(void) {
    soundStream->lock();
	soundSimulator->clear();
	soundStream->unlock();
}

void N3DSDriver::draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) {
	LightLock_Lock(&videoLock);
	currentLayer->write(x, y, chr, col);
	LightLock_Unlock(&videoLock);
}

void N3DSDriver::read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) {
	currentLayer->read(x, y, chr, col);
}

void N3DSDriver::draw_frame(void) {
	C3D_Mtx projection;
	N3DSTextLayer *layer = currentLayer;

	Mtx_OrthoTilt(&projection, 0, 400, 240, 0, -1.0, 64.0, true);

	C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
	C3D_FrameDrawOn(tgt_top0);
	C3D_RenderTargetClear(tgt_top0, C3D_CLEAR_ALL, 0x000000, 0);

	LightLock_Lock(&videoLock);

	layer->draw((400 - layer->width_px()) / 2, (240 - layer->height_px()) / 2, 0.0f, projection);

	/* C3D_FrameDrawOn(tgt_top1);
	C3D_RenderTargetClear(tgt_top1, C3D_CLEAR_ALL, 0x000000, 0);

	layer->draw((400 - layer->width_px()) / 2, (240 - layer->height_px()) / 2, 0.0f, projection); */

	LightLock_Unlock(&videoLock);

	C3D_FrameEnd(0);
}

void N3DSDriver::update_joy(void) {
	hidScanInput();
	uint32_t ki = hidKeysHeld();

	set_joy_button_state(JoyButtonUp, (ki & KEY_UP) != 0, true);
	set_joy_button_state(JoyButtonDown, (ki & KEY_DOWN) != 0, true);
	set_joy_button_state(JoyButtonLeft, (ki & KEY_LEFT) != 0, true);
	set_joy_button_state(JoyButtonRight, (ki & KEY_RIGHT) != 0, true);
	set_joy_button_state(JoyButtonA, (ki & KEY_A) != 0, true);
	set_joy_button_state(JoyButtonB, (ki & KEY_B) != 0, true);
	set_joy_button_state(JoyButtonX, (ki & KEY_X) != 0, true);
	set_joy_button_state(JoyButtonY, (ki & KEY_Y) != 0, true);
	set_joy_button_state(JoyButtonL, (ki & KEY_L) != 0, true);
	set_joy_button_state(JoyButtonR, (ki & KEY_R) != 0, true);
	set_joy_button_state(JoyButtonSelect, (ki & KEY_SELECT) != 0, true);
	set_joy_button_state(JoyButtonStart, (ki & KEY_START) != 0, true);
}

#include "gamevars.h"

UserInterface *N3DSDriver::create_user_interface(Game &game) {
	if (game.engineDefinition.engineType == ENGINE_TYPE_SUPER_ZZT) {
		this->currentLayer = this->mainLayer40;
		return new UserInterfaceSuperZZT(this, 50, 30);
	} else {
		this->currentLayer = this->mainLayer;
		return new UserInterfaceSlim(this);
	}
}

void N3DSDriver::on_pit_tick() {
	hsecs += 11;
	soundSimulator->allowed = _queue.is_playing;

	// TODO: wake PIT listeners
}

void gameThreadProc(void *userdata) {
	game->GameTitleLoop();
	gameRunning = false;
}

void ZZT::timerThreadProc(void *userdata) {
	N3DSDriver *driver = (N3DSDriver*) userdata;
	while (gameRunning) {
		svcWaitSynchronization(driver->pitTimer, 549245630);
		svcClearTimer(driver->pitTimer);
		driver->on_pit_tick();
	}
}

int main(int argc, char** argv) {
	N3DSDriver driver = N3DSDriver();
	game = new Game();

	osSetSpeedupEnable(1);

	gfxInitDefault();
	consoleInit(GFX_BOTTOM, NULL);

	driver.install();
	driver.keyboard.driver = &driver;

	game->driver = &driver;
	game->filesystem = new PosixFilesystemDriver();

	s32 mainThreadPrio;
	svcGetThreadPriority(&mainThreadPrio, CUR_THREAD_HANDLE);
	Thread gameThread = threadCreate(gameThreadProc, nullptr, 0x10000, mainThreadPrio - 1, 0, true);
	Thread timerThread = threadCreate(timerThreadProc, &driver, 0x2000, mainThreadPrio - 2, 0, true);

	while (aptMainLoop() && gameRunning) {
		driver.update_joy();
		driver.draw_frame();
	}

	threadJoin(timerThread, 500000000);
	threadJoin(gameThread, 500000000);

	delete game->filesystem;
	delete game;

	driver.uninstall();

	return 0;
}
