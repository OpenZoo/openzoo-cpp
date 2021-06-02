#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "audio_simulator_bandlimited.h"
#include "sounds.h"

using namespace ZZT;

// TODO: redundant - copypasted from audio_simulator.cpp
#define PIT_DIVISOR 1193182

#ifndef M_PI
#define M_PI 3.14159265358979
#endif

#define FIXED_SHIFT 6
#define FIXED_SHIFT_STEP 8
#define TRIG_SHIFT 14
#define COEFF_MAX 512

template<typename SampleFormat>
AudioSimulatorBandlimited<SampleFormat>::AudioSimulatorBandlimited(SoundQueue *queue, int audio_frequency, bool audio_signed)
    : AudioSimulator<SampleFormat>(queue, audio_frequency, audio_signed) {
    this->cos_table = (int16_t*) malloc(sizeof(int16_t) * (1 << TRIG_SHIFT));
    this->coeff_table = (int16_t*) malloc(sizeof(int16_t) * COEFF_MAX);

    for (int i = 0; i < (1 << (TRIG_SHIFT)); i++) {
        cos_table[i] = cosf(2 * M_PI * (i / (float) (1 << TRIG_SHIFT))) * 16384;
    }
    coeff_table[0] = 0;
    for (int i = 1; i < COEFF_MAX; i++) {
        coeff_table[i] = 32768 * sinf(0.5 * i * M_PI) / (i * M_PI);
    }
}

template<typename SampleFormat>
AudioSimulatorBandlimited<SampleFormat>::~AudioSimulatorBandlimited() {
    free(this->coeff_table);
    free(this->cos_table);
}

template<typename SampleFormat>
void AudioSimulatorBandlimited<SampleFormat>::note_to(uint32_t targetNotePos, uint32_t frequency, SampleFormat *stream, int32_t &streamPos, int32_t streamLen) {
    uint32_t iMax = this->calc_jump(targetNotePos, streamPos, streamLen);

    if (iMax > 0) {
        uint32_t pit_ticks = PIT_DIVISOR / frequency;
        uint32_t freq_real_fixed = (PIT_DIVISOR << FIXED_SHIFT) / pit_ticks;

        uint32_t stepSize = (1 << FIXED_SHIFT_STEP);
        uint32_t samplePos = (this->current_note_pos << FIXED_SHIFT_STEP);

        for (uint32_t i = 0; i < iMax; i++, samplePos += stepSize) {
            int32_t sample = 0;
            int coeff_pos = 1;
            uint64_t cos_indice = ((uint64_t) freq_real_fixed * samplePos / this->audio_frequency);
            int pos = freq_real_fixed;
            
            while (pos < (this->audio_frequency << (FIXED_SHIFT - 1)) && coeff_pos < COEFF_MAX) {
                // -(1 << 28) = 0
                // (1 << 28) = 1
                sample += ((int64_t) coeff_table[coeff_pos] * cos_table[(cos_indice * coeff_pos) & ((1 << TRIG_SHIFT) - 1)]);

                pos += freq_real_fixed;
                coeff_pos++;
            }

            sample = ((((sample >> TRIG_SHIFT) + (1 << TRIG_SHIFT)) * (this->sample_max - this->sample_min)) >> (TRIG_SHIFT + 1)) + this->sample_min;
            if (sample < this->sample_min) sample = this->sample_min;
            else if (sample > this->sample_max) sample = this->sample_max;

            stream[streamPos + i] = sample;
        }
    }

    this->jump_by(iMax, streamPos);
}

template class ZZT::AudioSimulatorBandlimited<uint16_t>;