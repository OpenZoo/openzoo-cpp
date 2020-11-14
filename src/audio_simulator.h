#ifndef __AUDIO_SIMULATOR_H__
#define __AUDIO_SIMULATOR_H__

#include <stddef.h>
#include <stdint.h>
#include "sounds.h"

namespace ZZT {
    class AudioSimulator {
    private:
        int audio_frequency;
        int samples_per_pit;
        int samples_per_drum;

        SoundQueue *queue;
        int32_t current_note;
        uint32_t current_note_pos;
        uint32_t current_note_max;
    
        uint32_t calc_jump(uint32_t targetNotePos, int32_t streamPos, int32_t streamLen);
        void jump_by(uint32_t amount, int32_t &streamPos);
        void note_to(uint32_t targetNotePos, uint32_t frequency, uint8_t *stream, int32_t &streamPos, int32_t streamLen);
        void silence_to(uint32_t targetNotePos, uint8_t *stream, int32_t &streamPos, int32_t streamLen);

    public:
        bool allowed;

        AudioSimulator(SoundQueue *queue);
        void set_frequency(int frequency);
        void clear(void);
        void simulate(uint8_t *stream, size_t len);
    };
};

#endif