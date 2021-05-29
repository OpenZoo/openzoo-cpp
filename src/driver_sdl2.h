#ifndef __DRIVER_SDL2_H__
#define __DRIVER_SDL2_H__

#include <cstdint>
#include "driver.h"
#include "audio_simulator.h"
#include "audio_simulator_bandlimited.h"

namespace ZZT {
    class Game;
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

    class SDL2Driver: public Driver {
        friend uint32_t pitTimerCallback(uint32_t interval, SDL2Driver *driver);
        friend uint32_t videoInputThread(SDL2Driver *driver);
        friend uint32_t gameThread(Game *game);
        friend void audioCallback(SDL2Driver *driver, uint8_t *stream, int32_t len);

    private:
        int16_t width_chars, height_chars;
        bool installed;
        SDL_TimerID pit_timer_id;
        uint16_t timer_hsecs;
        SDL_mutex *timer_mutexes[IdleModeCount];
        SDL_cond *timer_conds[IdleModeCount];

        void wake(IdleMode mode);
        void update_keymod(uint16_t kmod);

        // input
        SDL_GameController *controller;

        // video
        SDL_Window *window;
        SDL_Renderer *renderer;
        SDL_mutex *inputMutex;
        SDL_mutex *playfieldMutex;
        SDL_Texture *playfieldTexture;
        CharsetTexture *charsetTexture;
        SDL_Thread *gameThread;
        // SDL_Thread *renderThread;
        bool renderThreadRunning;

        uint8_t *screen_buffer;
        bool *screen_buffer_changed;

        CharsetTexture* loadCharsetFromBMP(const char *path);
        CharsetTexture* loadCharsetFromBytes(const uint8_t *buf, size_t len);

        void render_char_bg(int16_t x, int16_t y);
        void render_char_fg(int16_t x, int16_t y, bool blink);

        // audio
        AudioSimulator *soundSimulator;
        SDL_mutex *soundBufferMutex;
        SDL_AudioDeviceID audioDevice;
        SDL_AudioSpec audioSpec;

    public:
        SDL2Driver(int width_chars, int height_chars);
        ~SDL2Driver();

        void install(void);
        void uninstall(void);

        // required (input)
        void update_input(void) override;

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
        void get_video_size(int16_t &width, int16_t &height) override;
        bool set_video_size(int16_t width, int16_t height, bool simulate) override;
        void draw_string(int16_t x, int16_t y, uint8_t col, const char *str) override;
        void clrscr(void) override;
    };
}

#endif