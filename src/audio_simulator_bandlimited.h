#ifndef __AUDIO_SIMULATOR_BANDLIMITED_H__
#define __AUDIO_SIMULATOR_BANDLIMITED_H__

#include "audio_simulator.h"

namespace ZZT {
    class AudioSimulatorBandlimited : public AudioSimulator {
    private:
        int16_t *cos_table;
        int16_t *coeff_table;

    protected:
        virtual void note_to(uint32_t targetNotePos, uint32_t frequency, uint8_t *stream, int32_t &streamPos, int32_t streamLen);

    public:
        AudioSimulatorBandlimited(SoundQueue *queue, int audio_frequency, bool audio_signed);
        virtual ~AudioSimulatorBandlimited();
    };
};

#endif