#include <cstddef>
#include <cstring>
#include <SDL.h>
#include "driver_sdl2.h"
#include "filesystem_posix.h"
#include "user_interface_slim.h"

#define PIT_SPEED_MS 55

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
#define TEXT_BLINK_RATE 534

static const uint8_t sdl_to_pc_scancode[] = {
/*  0*/ 0,
/*  1*/ 0, 0, 0,
/*  4*/ 0x1E, 0x30, 0x2E, 0x20, 0x12, 0x21, 0x22, 0x23, 0x17, /* A-I */
/* 13*/ 0x24, 0x25, 0x26, 0x32, 0x31, 0x18, 0x19, 0x10, 0x13, /* J-R */
/* 22*/ 0x1F, 0x14, 0x16, 0x2F, 0x11, 0x2D, 0x15, 0x2C,       /* S-Z */
/* 30*/ 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, /* 1-0 */
/* 40*/ 0x1C, 0x01, 0x0E, 0x0F, 0x39,
/* 45*/ 0x0C, 0x0D, 0x1A, 0x1B, 0x2B,
/* 50*/ 0x2B, 0x27, 0x28, 0x29,
/* 54*/ 0x33, 0x34, 0x35, 0x3A,
        0x3B, 0x3C, 0x3D, 0x3E, 0x3F, 0x40, 0x41, 0x42, 0x43, 0x44, 0x57, 0x58,
        0x37, 0x46, 0, 0x52, 0x47, 0x49, 0x53, 0x4F, 0x51,
        0x4D, 0x4B, 0x50, 0x48, 0x45
};

static const int sdl_to_pc_scancode_max = sizeof(sdl_to_pc_scancode) - 1;

using namespace ZZT;

CharsetTexture::CharsetTexture() {
    texture = nullptr;
}

CharsetTexture::~CharsetTexture() {
    if (texture != nullptr) {
        SDL_DestroyTexture(texture);
    }
}

CharsetTexture* SDL2Driver::loadCharsetFromBMP(const char *path) {
    SDL_Surface *surface = SDL_LoadBMP(path);
    if (surface == NULL) {
        return nullptr;
    }

    SDL_Surface *surfaceOld = surface;
    surface = SDL_ConvertSurfaceFormat(surfaceOld, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(surfaceOld);
    if (surface == NULL) {
        return nullptr;
    }

    int32_t tex_width = surface->w;
    int32_t tex_height = surface->h;
    if (tex_width * tex_height < 256) {
        return nullptr;
    }

    SDL_SetColorKey(surface, 1, ((uint32_t*) surface->pixels)[0] & 0x00FFFFFF);

    CharsetTexture *tex = new CharsetTexture();
    if (tex == nullptr) return nullptr;

    tex->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (tex->texture == nullptr) {
        delete tex;
        return nullptr;
    }

    tex->charsetPitch = 1;
    while (tex->charsetPitch <= 256) {
        if ((tex_width % tex->charsetPitch) == 0 && (tex_height % (256 / tex->charsetPitch)) == 0) {
            // Proper divisors, 256 tiles
            // say, 8x16 will lead to 16x8 in the next iteration
            // so, clamping between 1:1 and 1:2 aspect ratios should work
            tex->charWidth = tex_width / tex->charsetPitch;
            tex->charHeight = tex_height / (256 / tex->charsetPitch);

            // We go from highest to lowest aspect ratio, so just pick the first one which works.
            if (tex->charWidth <= tex->charHeight) {
                return tex;
            }
        }
        tex->charsetPitch <<= 1;
    }

    delete tex;
    return nullptr;
}

CharsetTexture* SDL2Driver::loadCharsetFromBytes(const uint8_t *buf, size_t len) {
    CharsetTexture *tex = new CharsetTexture();
    if (tex == nullptr) return nullptr;

    tex->charWidth = 8;
    tex->charHeight = len >> 8;
    tex->charsetPitch = 32;
    if (tex->charHeight < 8 || tex->charHeight > 16) {
        delete tex;
        return nullptr;
    }

    uint8_t colors[256 * 128];

    for (int i = 0; i < 256; i++) {
        for (int iy = 0; iy < tex->charHeight; iy++) {
            uint8_t ib = buf[i * tex->charHeight + iy];
            int iyo = (((i >> 5) * tex->charHeight + iy) << 8) + ((i & 31) << 3);
            for (int ix = 0; ix < 8; ix++) {
                colors[iyo + ix] = (ib >> (7 - ix)) & 1;
            }
        }
    }

    SDL_Surface* surface = SDL_CreateRGBSurfaceFrom(colors, 256, tex->charHeight * 8, 8, 256, 0, 0, 0, 0);
    if (surface == nullptr) {
        delete tex;
        return nullptr;
    }

    SDL_Color palette[2] = {
        { .r = 0, .g = 0, .b = 0, .a = 0 },
        { .r = 255, .g = 255, .b = 255, .a = 255 }
    };

    SDL_SetPaletteColors(surface->format->palette, palette, 0, 2);
    SDL_SetColorKey(surface, 1, 0);

    tex->texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (tex->texture == nullptr) {
        delete tex;
        return nullptr;
    }

    return tex;
}

uint32_t ZZT::pitTimerCallback(uint32_t interval, SDL2Driver *driver) {
    driver->timer_hsecs += (PIT_SPEED_MS) / 5;
    driver->soundSimulator.allowed = driver->_queue.is_playing;

    driver->wake(IMUntilPit);
    return PIT_SPEED_MS;
}

void SDL2Driver::render_char_bg(int16_t x, int16_t y) {
    int offset = (y * width_chars + x) << 1;
    uint8_t col = screen_buffer[offset + 1];

    if (col < 0x80 && !screen_buffer_changed[offset]) return;

    col = (col >> 4) & 0x07;
    uint32_t bg_col = ega_palette[col];

    SDL_Rect out_rect = {
        .x = x * charsetTexture->charWidth,
        .y = y * charsetTexture->charHeight,
        .w = charsetTexture->charWidth,
        .h = charsetTexture->charHeight
    };

    SDL_SetRenderDrawColor(renderer, bg_col >> 16, bg_col >> 8, bg_col >> 0, SDL_ALPHA_OPAQUE);
    SDL_RenderFillRect(renderer, &out_rect);
}

void SDL2Driver::render_char_fg(int16_t x, int16_t y, bool blink) {
    int offset = (y * width_chars + x) << 1;
    uint8_t chr = screen_buffer[offset];
    uint8_t col = screen_buffer[offset + 1];

    if (chr == 0 || chr == 32) return;
    if (col < 0x80 && !screen_buffer_changed[offset]) return;

    blink &= (col >= 0x80);
    col &= 0x7F;
    if (blink || ((col >> 4) == (col & 0x0F))) return;

    uint32_t fg_col = ega_palette[col & 0x0F];

    SDL_Rect in_rect = {
        .x = (chr % charsetTexture->charsetPitch) * charsetTexture->charWidth,
        .y = (chr / charsetTexture->charsetPitch) * charsetTexture->charHeight,
        .w = charsetTexture->charWidth,
        .h = charsetTexture->charHeight
    };

    SDL_Rect out_rect = {
        .x = x * charsetTexture->charWidth,
        .y = y * charsetTexture->charHeight,
        .w = charsetTexture->charWidth,
        .h = charsetTexture->charHeight
    };

    SDL_SetTextureColorMod(charsetTexture->texture, fg_col >> 16, fg_col >> 8, fg_col >> 0);
    SDL_RenderCopy(renderer, charsetTexture->texture, &in_rect, &out_rect);
}

uint32_t ZZT::videoRenderThread(SDL2Driver *driver) {
    while (driver->renderThreadRunning) {
        SDL_RenderClear(driver->renderer);
        SDL_SetRenderTarget(driver->renderer, driver->playfieldTexture);

        SDL_LockMutex(driver->playfieldMutex);
        bool blink = (SDL_GetTicks() % TEXT_BLINK_RATE) >= (TEXT_BLINK_RATE / 2);

        for (int iy = 0; iy < driver->height_chars; iy++) {
            for (int ix = 0; ix < driver->width_chars; ix++) {
                driver->render_char_bg(ix, iy);
            }
        }

        for (int iy = 0; iy < driver->height_chars; iy++) {
            for (int ix = 0; ix < driver->width_chars; ix++) {
                driver->render_char_fg(ix, iy, blink);
            }
        }

        memset(driver->screen_buffer_changed, 0, driver->width_chars * driver->height_chars * sizeof(bool) * 2);
        SDL_UnlockMutex(driver->playfieldMutex);

        SDL_SetRenderTarget(driver->renderer, nullptr);
        SDL_RenderCopy(driver->renderer, driver->playfieldTexture, nullptr, nullptr);

        SDL_RenderPresent(driver->renderer);
        driver->wake(IMUntilFrame);
        SDL_Delay(1);
    }
    return 0;
}

void ZZT::audioCallback(SDL2Driver *driver, uint8_t *stream, int32_t len) {
    driver->sound_lock();
    driver->soundSimulator.simulate(stream, len);
    driver->sound_unlock();
}

SDL2Driver::SDL2Driver(int width_chars, int height_chars): soundSimulator(&_queue) {
    this->installed = false;
    this->width_chars = width_chars;
    this->height_chars = height_chars;
    this->screen_buffer = (uint8_t*) malloc(width_chars * height_chars * sizeof(uint8_t) * 2);
    this->screen_buffer_changed = (bool*) malloc(width_chars * height_chars * sizeof(bool) * 2);
    memset(this->screen_buffer_changed, 0, width_chars * height_chars * sizeof(bool) * 2);
}

SDL2Driver::~SDL2Driver() {
    free(this->screen_buffer_changed);
    free(this->screen_buffer);
}

void SDL2Driver::install(void) {
    if (!installed) {
        SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_GAMECONTROLLER);
        SDL_StartTextInput();
        pit_timer_id = SDL_AddTimer(PIT_SPEED_MS, (SDL_TimerCallback) pitTimerCallback, this);
        for (int i = 0; i < IdleModeCount; i++) {
            timer_mutexes[i] = SDL_CreateMutex();
            timer_conds[i] = SDL_CreateCond();
        }

        // input
        controller = nullptr;
        SDL_GameControllerAddMappingsFromFile("gamecontrollerdb.txt");
        for (int i = 0; i < SDL_NumJoysticks(); i++) {
            if (SDL_IsGameController(i)) {
                controller = SDL_GameControllerOpen(i);
                if (controller != nullptr) {
                    break;
                }
            }
        }

        // video
        window = SDL_CreateWindow("test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 640, 350, 0);
        renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "0");

        charsetTexture = loadCharsetFromBMP("ASCII.BMP");
        if (charsetTexture == nullptr) {
            exit(1);
        }

        playfieldTexture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET,
            width_chars * charsetTexture->charWidth, height_chars * charsetTexture->charHeight);
        SDL_SetWindowSize(window, width_chars * charsetTexture->charWidth, height_chars * charsetTexture->charHeight);

        playfieldMutex = SDL_CreateMutex();

        SDL_RenderClear(renderer);
        SDL_RenderPresent(renderer);

        renderThreadRunning = true;
        renderThread = SDL_CreateThread((SDL_ThreadFunction) videoRenderThread, "Video render thread", this);

        // audio
        soundBufferMutex = SDL_CreateMutex();
        SDL_AudioSpec requestedAudioSpec;
        memset(&requestedAudioSpec, 0, sizeof(SDL_AudioSpec));
        requestedAudioSpec = {
            .freq = 48000,
            .format = AUDIO_U8,
            .channels = 1,
            .samples = 2048,
            .callback = (SDL_AudioCallback) audioCallback,
            .userdata = this
        };
        audioDevice = SDL_OpenAudioDevice(nullptr, 0, &requestedAudioSpec, &audioSpec, 0);
        SDL_PauseAudioDevice(audioDevice, 0);

        installed = true;
    }
}

void SDL2Driver::uninstall(void) {
    if (installed) {
        installed = false;

        // audio
        if (audioDevice != 0) {
            SDL_CloseAudioDevice(audioDevice);
        }
        SDL_DestroyMutex(soundBufferMutex);

        // video
        renderThreadRunning = false;
        SDL_WaitThread(renderThread, nullptr);

        delete charsetTexture;
        SDL_DestroyMutex(playfieldMutex);
        SDL_DestroyTexture(playfieldTexture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);

        // input
        if (controller != nullptr) {
            SDL_GameControllerClose(controller);
        }

        for (int i = 0; i < IdleModeCount; i++) {
            SDL_DestroyMutex(timer_mutexes[i]);
            SDL_DestroyCond(timer_conds[i]);
        }
        SDL_RemoveTimer(pit_timer_id);
        SDL_StopTextInput();
        SDL_Quit();
    }
}

uint16_t SDL2Driver::get_hsecs(void) {
    return timer_hsecs >> 1;
}

void SDL2Driver::delay(int ms) {
    SDL_Delay(ms);
}

void SDL2Driver::wake(IdleMode mode) {
    SDL_LockMutex(timer_mutexes[mode]);
    SDL_CondBroadcast(timer_conds[mode]);
    SDL_UnlockMutex(timer_mutexes[mode]);
}

void SDL2Driver::idle(IdleMode mode) {
    SDL_LockMutex(timer_mutexes[mode]);
    SDL_CondWait(timer_conds[mode], timer_mutexes[mode]);
    SDL_UnlockMutex(timer_mutexes[mode]);
}

void SDL2Driver::draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) {
    int offset = (y * width_chars + x) << 1;
    SDL_LockMutex(playfieldMutex);
    screen_buffer[offset] = chr;
    screen_buffer_changed[offset++] = true;
    screen_buffer[offset] = col;
    SDL_UnlockMutex(playfieldMutex);
}

void SDL2Driver::read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) {
    int offset = (y * width_chars + x) << 1;
    chr = screen_buffer[offset++];
    col = screen_buffer[offset];
}

void SDL2Driver::draw_string(int16_t x, int16_t y, uint8_t col, const char *str) {
    int offset = (y * width_chars + x) << 1;
    SDL_LockMutex(playfieldMutex);
    while (*str != 0) {
        screen_buffer[offset] = *(str++);
        screen_buffer_changed[offset++] = true;
        screen_buffer[offset++] = col;
    }
    SDL_UnlockMutex(playfieldMutex);
}

void SDL2Driver::clrscr(void) {
    SDL_LockMutex(playfieldMutex);
    memset(screen_buffer, 0, width_chars * height_chars * 2);
    memset(screen_buffer_changed, 1, width_chars * height_chars * 2);
    SDL_UnlockMutex(playfieldMutex);
}

void SDL2Driver::update_keymod(uint16_t kmod) {
    keyLeftShiftHeld = (kmod & KMOD_LSHIFT) != 0;
    keyRightShiftHeld = (kmod & KMOD_RSHIFT) != 0;
    keyShiftHeld = (kmod & KMOD_SHIFT) != 0;
    keyCtrlHeld = (kmod & KMOD_CTRL) != 0;
    keyAltHeld = (kmod & KMOD_ALT) != 0;
    keyNumLockHeld = (kmod & KMOD_NUM) != 0;
}

static JoyButton sdl_to_pc_joybutton(SDL_GameControllerButton button) {
    switch (button) {
        case SDL_CONTROLLER_BUTTON_A: return JoyButtonB;
        case SDL_CONTROLLER_BUTTON_B: return JoyButtonA;
        case SDL_CONTROLLER_BUTTON_X: return JoyButtonY;
        case SDL_CONTROLLER_BUTTON_Y: return JoyButtonX;
        case SDL_CONTROLLER_BUTTON_START: return JoyButtonStart;
        case SDL_CONTROLLER_BUTTON_BACK: return JoyButtonSelect;
        case SDL_CONTROLLER_BUTTON_LEFTSHOULDER: return JoyButtonL;
        case SDL_CONTROLLER_BUTTON_RIGHTSHOULDER: return JoyButtonR;
        case SDL_CONTROLLER_BUTTON_DPAD_UP: return JoyButtonUp;
        case SDL_CONTROLLER_BUTTON_DPAD_LEFT: return JoyButtonLeft;
        case SDL_CONTROLLER_BUTTON_DPAD_RIGHT: return JoyButtonRight;
        case SDL_CONTROLLER_BUTTON_DPAD_DOWN: return JoyButtonDown;
        default: return JoyButtonNone;
    }
}

void SDL2Driver::update_input(void) {
    deltaX = 0;
    deltaY = 0;
    shiftPressed = false;
    joystickMoved = false;
    joy_buttons_pressed = 0;

    SDL_Event event;
    uint32_t scode, kcode;
    uint8_t k = 0;

    while (SDL_PollEvent(&event) != 0) {
        switch (event.type) {
            case SDL_TEXTINPUT: {
                k = event.text.text[0];
                if (!(k >= 32 && k < 127)) {
                    k = 0;
                }
            } break;
            case SDL_KEYDOWN: {
                update_keymod(event.key.keysym.mod);
                scode = event.key.keysym.scancode;
                kcode = event.key.keysym.sym;
                if (kcode > 0 && kcode < 127) {
                    k = kcode;
                } else if (scode <= sdl_to_pc_scancode_max) {
                    k = sdl_to_pc_scancode[scode] | 0x80;
                } else {
                    k = 0;
                }
            } break;
            case SDL_KEYUP: {
                update_keymod(event.key.keysym.mod);
            } break;
            case SDL_CONTROLLERBUTTONDOWN: {
                JoyButton button = sdl_to_pc_joybutton((SDL_GameControllerButton) event.cbutton.button);
                if (button != JoyButtonNone) {
                    set_joy_button_state(button, true);
                }
            } break;
            case SDL_CONTROLLERBUTTONUP: {
                JoyButton button = sdl_to_pc_joybutton((SDL_GameControllerButton) event.cbutton.button);
                if (button != JoyButtonNone) {
                    set_joy_button_state(button, false);
                }
            } break;
            case SDL_QUIT: {
                // TODO
                exit(1);
            } break;
        }
    }

    set_key_pressed(k);
    update_joy_buttons();
    shiftPressed = keyShiftHeld;

    if (controller != nullptr) {
        // emulate delta/shift behaviour on game controller
        // TODO
        /* int16_t axis_x = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX);
        int16_t axis_y = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY);
        set_axis(axis_x, axis_y, -16384, 16384); */

        if (SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_B)) {
            if (!shiftAccepted) {
                shiftPressed = true;
            }
        } else {
            shiftAccepted = false;
        }
    }
}

void SDL2Driver::read_wait_key(void) {
    update_input();
    if (keyPressed != 0) return;
 
    do {
        idle(IMUntilFrame);
        update_input();
    } while (keyPressed == 0);
}

void SDL2Driver::sound_stop(void) {
    sound_lock();
    soundSimulator.clear();
    sound_unlock();
}

void SDL2Driver::sound_lock(void) {
    SDL_LockMutex(soundBufferMutex);
}

void SDL2Driver::sound_unlock(void) {
    SDL_UnlockMutex(soundBufferMutex);
}

#include "gamevars.h"

static Game game = Game();

int main(int argc, char** argv) {
	SDL2Driver driver = SDL2Driver(80, 25);

	game.driver = &driver;
    game.filesystem = new PosixFilesystemDriver();
    game.interface = new UserInterface(&driver);
//    game.interface = new UserInterfaceSlim(&driver);

	driver.install();

	driver.clrscr();
	game.GameTitleLoop();

	driver.uninstall();

    delete game.interface;
    delete game.filesystem;

	return 0;
}
