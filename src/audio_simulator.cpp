#include <cmath>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "audio_simulator.h"
#include "sounds.h"

using namespace ZZT;

/*
  Known issues:
  - currently hardcoded to unsigned 8-bit samples
*/

#define PIT_DIVISOR 1193182
#define SAMPLES_NOTE_DELAY 16

// longest sample would be 2640 * 256 = 675840 samples
#define FIXED_SHIFT 9

template<typename SampleFormat>
uint32_t AudioSimulator<SampleFormat>::calc_jump(uint32_t targetNotePos, int32_t streamPos, int32_t streamLen) {
    int32_t maxTargetChange = targetNotePos - current_note_pos;
    if (maxTargetChange < 0) {
        return 0;
    }
    int32_t maxStreamChange = streamLen - streamPos;
    if (maxTargetChange < maxStreamChange) {
        return maxTargetChange;
    } else {
        return maxStreamChange;
    }
}

template<typename SampleFormat>
void AudioSimulator<SampleFormat>::jump_by(uint32_t amount, int32_t &streamPos) {
    current_note_pos += amount;
    streamPos += amount;
    if (current_note_pos >= current_note_max) {
        current_note = -1;
    }
}

template<typename SampleFormat>
void AudioSimulator<SampleFormat>::note_to(uint32_t targetNotePos, uint32_t frequency, SampleFormat *stream, int32_t &streamPos, int32_t streamLen) {
    uint32_t iMax = calc_jump(targetNotePos, streamPos, streamLen);

    if (iMax > 0) {
#if 0
        // floating-point implementation
        double samplesPerChange = ((double) audio_frequency) / (((double) PIT_DIVISOR) / (uint32_t) (PIT_DIVISOR / frequency));

        uint32_t samplePos = current_note_pos;
        for (uint32_t i = 0; i < iMax; i++, samplePos++) {
            if (fmod(samplePos, samplesPerChange) < (samplesPerChange / 2.0)) {
                stream[streamPos + i] = sample_min;
            } else {
                stream[streamPos + i] = sample_max;
            }
        }
#else
        // fixed-point implementation
        uint32_t pit_ticks = PIT_DIVISOR / frequency;
        uint32_t samplesPerChange = ((uint64_t) (audio_frequency * pit_ticks) << FIXED_SHIFT) / PIT_DIVISOR;

        uint32_t stepSize = (1 << FIXED_SHIFT);
        uint32_t samplePos = (current_note_pos << FIXED_SHIFT) % samplesPerChange;
        for (uint32_t i = 0; i < iMax; i++, samplePos += stepSize) {
            while (samplePos >= samplesPerChange) samplePos -= samplesPerChange;
            if (samplePos < (samplesPerChange >> 1)) {
                if (samplePos < stepSize) {
                    // transition from max to min
                    SampleFormat trans_val = ((sample_max * (stepSize - samplePos)) + (sample_min * samplePos)) >> FIXED_SHIFT;
                    stream[streamPos + i] = trans_val;
                } else {
                    stream[streamPos + i] = sample_min;
                }
            } else {
                int midPos = samplePos - (samplesPerChange >> 1);
                if (midPos < stepSize) {
                    // transition from min to max
                    SampleFormat trans_val = ((sample_min * (stepSize - midPos)) + (sample_max * midPos)) >> FIXED_SHIFT;
                    stream[streamPos + i] = trans_val;
                } else {
                    stream[streamPos + i] = sample_max;
                }
            }
        }
#endif
    }

    jump_by(iMax, streamPos);
}

template<typename SampleFormat>
void AudioSimulator<SampleFormat>::silence_to(uint32_t targetNotePos, SampleFormat *stream, int32_t &streamPos, int32_t streamLen) {
    uint32_t iMax = calc_jump(targetNotePos, streamPos, streamLen);
    if (iMax > 0) {
        for (uint32_t i = 0; i < iMax; i++) {
            stream[streamPos + i] = sample_none;
        }
    }
    jump_by(iMax, streamPos);
}

template<typename SampleFormat>
AudioSimulator<SampleFormat>::AudioSimulator(SoundQueue *queue, int audio_frequency, bool audio_signed) {
    this->queue = queue;
    this->allowed = false;
    this->audio_signed = audio_signed;
    clear();
    set_volume(64);
    set_frequency(audio_frequency);
}

template<typename SampleFormat>
void AudioSimulator<SampleFormat>::clear(void) {
    this->current_note = -1;
}

template<typename SampleFormat>
int AudioSimulator<SampleFormat>::volume(void) const {
    return this->sample_none - this->sample_min;
}

template<>
void AudioSimulator<uint8_t>::set_volume(int volume) {
    this->sample_none = 128;
    this->sample_min = 128 - volume;
    this->sample_max = 128 + volume;

    if (audio_signed) {
        this->sample_min ^= 0x80;
        this->sample_none ^= 0x80;
        this->sample_max ^= 0x80;
    }
}

template<>
void AudioSimulator<uint16_t>::set_volume(int volume) {
    this->sample_none = 32768;
    this->sample_min = 32768 - (volume << 8);
    this->sample_max = 32768 + (volume << 8);

    if (audio_signed) {
        this->sample_min ^= 0x8000;
        this->sample_none ^= 0x8000;
        this->sample_max ^= 0x8000;
    }
}

template<typename SampleFormat>
void AudioSimulator<SampleFormat>::set_frequency(int frequency) {
    audio_frequency = frequency;
    samples_per_pit = frequency * 11 / 200;
    samples_per_drum = frequency / 1000;
}

template<typename SampleFormat>
void AudioSimulator<SampleFormat>::simulate(SampleFormat *stream, size_t len) {
    if (!queue->enabled || !queue->is_playing || !allowed) {
        current_note = -1;
		for (int i = 0; i < len; i++) {
			stream[i] = sample_none;
		}
    } else {
        int32_t pos = 0;
        while (pos < len) {
            if (current_note < 0) {
                uint16_t note, duration;
                if (!queue->pop(note, duration)) {
                    queue->is_playing = false;
					for (int i = pos; i < len; i++) {
						stream[i] = sample_none;
					}
                    break;
                } else {
                    current_note = note;
                    current_note_pos = 0;
                    current_note_max = duration * samples_per_pit;
                }
            }

            if (current_note >= NOTE_MIN && current_note < NOTE_MAX) {
                // note
                if (current_note_pos < SAMPLES_NOTE_DELAY) {
                    silence_to(SAMPLES_NOTE_DELAY, stream, pos, len);
                } else {
                    uint16_t freq = sound_notes[current_note - NOTE_MIN];
                    if (freq > 0) {
                        note_to(current_note_max, freq, stream, pos, len);
                    } else {
                        silence_to(current_note_max, stream, pos, len);
                    }
                }
            } else if (current_note >= DRUM_MIN && current_note < DRUM_MAX) {
                const SoundDrum &drum = sound_drums[current_note - DRUM_MIN];
                uint32_t drum_pos = current_note_pos / samples_per_drum;
                if (drum_pos < drum.len) {
                    note_to((drum_pos + 1) * samples_per_drum, drum.data[drum_pos], stream, pos, len);
                } else {
                    silence_to(current_note_max, stream, pos, len);
                }
            } else {
                // silence
                silence_to(current_note_max, stream, pos, len);                
            }
        }
    }
}

template class ZZT::AudioSimulator<uint8_t>;
template class ZZT::AudioSimulator<uint16_t>;