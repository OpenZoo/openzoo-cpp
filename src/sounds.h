#ifndef __SOUNDS_H__
#define __SOUNDS_H__

#include <cstdint>

#define NOTE_MIN 16
#define NOTE_MAX 112
#define DRUM_MIN 240
#define DRUM_MAX 250

namespace ZZT {
    typedef enum {
        IMUntilPit,
        IMUntilFrame,
        IdleModeCount
    } IdleMode;

    struct SoundDrum {
        uint8_t len;
        const uint16_t data[16];
    };

    int16_t SoundParse(const char *input, uint8_t *output, int16_t outlen);

    class SoundQueue {
        int16_t bufpos;
        int16_t buflen;
        int16_t current_priority;
        uint8_t buffer[255];

    public:
        bool enabled;
        bool is_playing;
        bool block_queueing;

        SoundQueue();

        void queue(int16_t priority, const uint8_t *pattern, int len);
        bool pop(uint16_t &note, uint16_t &duration);
        void clear(void);
    };

    class SoundDriver {
    protected:
        SoundQueue _queue;

    public:
        // required
        virtual uint16_t get_hsecs(void) = 0;
        virtual void delay(int ms) = 0;
        virtual void idle(IdleMode mode) = 0;
        virtual void sound_stop(void) = 0;

        // optional
        virtual void sound_lock(void) { };
        virtual void sound_unlock(void) { };
        virtual void sound_queue(int16_t priority, const uint8_t *pattern, int len) {
            sound_lock();
            _queue.queue(priority, pattern, len);
            sound_unlock();
        }

        void sound_clear_queue(void);
        inline bool sound_is_enabled(void) { return _queue.enabled; }
        inline void sound_set_enabled(bool value) { _queue.enabled = value; }
        inline bool sound_is_block_queueing(void) { return _queue.block_queueing; }
        inline void sound_set_block_queueing(bool value) { _queue.block_queueing = value; }

        // extra
        template<size_t i> inline void sound_queue(int16_t priority, const char (&str)[i]) {
            sound_queue(priority, (const uint8_t*) str, i - 1);
        }
    };

    extern const uint16_t sound_notes[NOTE_MAX - NOTE_MIN];
    extern const SoundDrum sound_drums[DRUM_MAX - DRUM_MIN];
}

#endif
