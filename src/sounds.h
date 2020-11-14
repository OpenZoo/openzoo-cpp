#ifndef __SOUNDS_H__
#define __SOUNDS_H__

#include <cstdint>

#define NOTE_MIN 16
#define NOTE_MAX 112
#define DRUM_MIN 240
#define DRUM_MAX 250

namespace ZZT {
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

    extern const uint16_t sound_notes[NOTE_MAX - NOTE_MIN];
    extern const SoundDrum sound_drums[DRUM_MAX - DRUM_MIN];
}

#endif
