#ifndef __AUDIO_SIMULATOR_H__
#define __AUDIO_SIMULATOR_H__

#include <stddef.h>
#include <stdint.h>
#include "sounds.h"

namespace ZZT {
    class AudioSimulator {
    protected:
        uint8_t sample_min;
        uint8_t sample_none;
        uint8_t sample_max;

        bool audio_signed;
        int audio_frequency;
        int samples_per_pit;
        int samples_per_drum;

        SoundQueue *queue;
        int32_t current_note;
        uint32_t current_note_pos;
        uint32_t current_note_max;
    
        uint32_t calc_jump(uint32_t targetNotePos, int32_t streamPos, int32_t streamLen);
        void jump_by(uint32_t amount, int32_t &streamPos);
        virtual void note_to(uint32_t targetNotePos, uint32_t frequency, uint8_t *stream, int32_t &streamPos, int32_t streamLen);
        void silence_to(uint32_t targetNotePos, uint8_t *stream, int32_t &streamPos, int32_t streamLen);

    public:
        bool allowed;

        AudioSimulator(SoundQueue *queue, int audio_frequency, bool audio_signed);
        virtual ~AudioSimulator() { }
        int volume() const;
        void set_volume(int volume); /* 0 .. 127 */
        void set_frequency(int frequency);
        void clear(void);
        void simulate(uint8_t *stream, size_t len);
    };
};

#endif