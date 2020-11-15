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
#define AUDIO_MIN 64
#define AUDIO_NONE 128
#define AUDIO_MAX 192

// longest sample would be 2640 * 256 = 675840 samples
#define FIXED_SHIFT 9

uint32_t AudioSimulator::calc_jump(uint32_t targetNotePos, int32_t streamPos, int32_t streamLen) {
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

void AudioSimulator::jump_by(uint32_t amount, int32_t &streamPos) {
    current_note_pos += amount;
    streamPos += amount;
    if (current_note_pos >= current_note_max) {
        current_note = -1;
    }
}

void AudioSimulator::note_to(uint32_t targetNotePos, uint32_t frequency, uint8_t *stream, int32_t &streamPos, int32_t streamLen) {
    uint32_t iMax = calc_jump(targetNotePos, streamPos, streamLen);

    if (iMax > 0) {
#if 0
        // floating-point implementation
        double samplesPerChange = ((double) audio_frequency) / (((double) PIT_DIVISOR) / (uint32_t) (PIT_DIVISOR / frequency));

        uint32_t samplePos = current_note_pos;
        for (uint32_t i = 0; i < iMax; i++, samplePos++) {
            if (fmod(samplePos, samplesPerChange) < (samplesPerChange / 2.0)) {
                stream[streamPos + i] = AUDIO_MIN;
            } else {
                stream[streamPos + i] = AUDIO_MAX;
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
                    uint32_t trans_val = ((AUDIO_MAX * (stepSize - samplePos)) + (AUDIO_MIN * samplePos)) >> FIXED_SHIFT;
                    stream[streamPos + i] = trans_val;
                } else {
                    stream[streamPos + i] = AUDIO_MIN;
                }
            } else {
                int midPos = samplePos - (samplesPerChange >> 1);
                if (midPos < stepSize) {
                    // transition from min to max
                    uint32_t trans_val = ((AUDIO_MIN * (stepSize - midPos)) + (AUDIO_MAX * midPos)) >> FIXED_SHIFT;
                    stream[streamPos + i] = trans_val;
                } else {
                    stream[streamPos + i] = AUDIO_MAX;
                }
            }
        }
#endif
    }

    jump_by(iMax, streamPos);
}

void AudioSimulator::silence_to(uint32_t targetNotePos, uint8_t *stream, int32_t &streamPos, int32_t streamLen) {
    uint32_t iMax = calc_jump(targetNotePos, streamPos, streamLen);
    if (iMax > 0) {
        for (uint32_t i = 0; i < iMax; i++) {
            stream[streamPos + i] = AUDIO_NONE;
        }
    }
    jump_by(iMax, streamPos);
}

AudioSimulator::AudioSimulator(SoundQueue *queue) {
    this->queue = queue;
    this->allowed = false;
    clear();
    set_frequency(48000);
}

void AudioSimulator::clear(void) {
    this->current_note = -1;
}

void AudioSimulator::set_frequency(int frequency) {
    audio_frequency = frequency;
    samples_per_pit = frequency * 11 / 200;
    samples_per_drum = frequency / 1000;
}

void AudioSimulator::simulate(uint8_t *stream, size_t len) {
    if (!queue->enabled || !queue->is_playing || !allowed) {
        current_note = -1;
        memset(stream, AUDIO_NONE, len);
    } else {
        int32_t pos = 0;
        while (pos < len) {
            if (current_note < 0) {
                uint16_t note, duration;
                if (!queue->pop(note, duration)) {
                    queue->is_playing = false;
                    memset(stream + pos, AUDIO_NONE, len - pos);
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
