#ifndef __DRIVER_SDL2_H__
#define __DRIVER_SDL2_H__

#include <cstdint>
#include "input.h"
#include "sounds.h"
#include "video.h"
#include "audio_simulator.h"
#include <SDL.h>

namespace ZZT {
    class SDL2Driver;

    class CharsetTexture {
        friend SDL2Driver;

    private:
        int charWidth, charHeight;
        int charsetPitch;
        SDL_Texture *texture;
        CharsetTexture();

    public:
        ~CharsetTexture();
    };

    class SDL2Driver: public InputDriver, public SoundDriver, public VideoDriver {
        friend uint32_t pitTimerCallback(uint32_t interval, SDL2Driver *driver);
        friend uint32_t videoRenderThread(SDL2Driver *driver);
        friend void audioCallback(SDL2Driver *driver, uint8_t *stream, int32_t len);

    private:
        bool installed;
        SDL_TimerID pit_timer_id;
        uint16_t timer_hsecs;
        SDL_mutex *timer_mutexes[IdleModeCount];
        SDL_cond *timer_conds[IdleModeCount];

        void wake(IdleMode mode);
        void update_keymod(uint16_t kmod);

        // video
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_mutex *playfieldMutex;
        SDL_Texture *playfieldTexture;
        CharsetTexture *charsetTexture;
        SDL_Thread *renderThread;
        bool renderThreadRunning;

        uint8_t screen_buffer[4000];
        bool screen_buffer_changed[4000];

        CharsetTexture* loadCharsetFromBMP(const char *path);
        CharsetTexture* loadCharsetFromBytes(const uint8_t *buf, size_t len);

        void render_char_bg(int16_t x, int16_t y);
        void render_char_fg(int16_t x, int16_t y, bool blink);

        // audio
        AudioSimulator soundSimulator;
        SDL_mutex *soundBufferMutex;
        SDL_AudioDeviceID audioDevice;
        SDL_AudioSpec audioSpec;

    public:
        SDL2Driver();

        // required (global)
        bool configure(void) override;
        void install(void) override;
        void uninstall(void) override;

        // required (input)
        void update_input(void) override;
        void read_wait_key(void) override;

        // required (sound)
        uint16_t get_hsecs(void) override;
        void delay(int ms) override;
        void idle(IdleMode mode) override;
        void sound_stop(void) override;
        void sound_lock(void) override;
        void sound_unlock(void) override;

        // required (video)
        void draw_char(int16_t x, int16_t y, uint8_t col, uint8_t chr) override;
        void read_char(int16_t x, int16_t y, uint8_t &col, uint8_t &chr) override;
        void draw_string(int16_t x, int16_t y, uint8_t col, const char *str) override;
        void clrscr(void) override;
    };
}

#endif