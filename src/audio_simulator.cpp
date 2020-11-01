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
  - currently hardcoded to 48000Hz unsigned 8-bit samples
*/

#define PIT_DIVISOR 1193182.0
#define AUDIO_FREQUENCY 48000.0
#define SAMPLES_PER_PIT 2640
#define SAMPLES_PER_DRUM 48
#define SAMPLES_NOTE_DELAY 16
#define AUDIO_MIN 64
#define AUDIO_NONE 128
#define AUDIO_MAX 192
    
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

// TODO: fixed-point?
void AudioSimulator::note_to(uint32_t targetNotePos, uint32_t frequency, uint8_t *stream, int32_t &streamPos, int32_t streamLen) {
    uint32_t iMax = calc_jump(targetNotePos, streamPos, streamLen);
    double samplesPerChange = AUDIO_FREQUENCY / (PIT_DIVISOR / (uint32_t)(PIT_DIVISOR / frequency));

    if (iMax > 0) {
        for (uint32_t i = 0; i < iMax; i++) {
            uint32_t samplePos = current_note_pos + i;
            if ((fmod(samplePos, samplesPerChange)) < (samplesPerChange / 2.0)) {
                stream[streamPos + i] = AUDIO_MIN;                
            } else {
                stream[streamPos + i] = AUDIO_MAX;
            }
        }
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
}

void AudioSimulator::clear(void) {
    this->current_note = -1;
}

void AudioSimulator::simulate(uint8_t *stream, size_t len) {
    if (!queue->enabled || !queue->is_playing || !allowed) {
        current_note = -1;
        memset(stream, AUDIO_NONE, len);
    } else {
        int32_t pos = 0;
        while (pos < len) {
            if (current_note < 0) {
                uint8_t note, duration;
                if (!queue->pop(note, duration)) {
                    queue->is_playing = false;
                    memset(stream + pos, AUDIO_NONE, len - pos);
                    break;
                } else {
                    current_note = note;
                    current_note_pos = 0;
                    current_note_max = duration * SAMPLES_PER_PIT;
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
                uint32_t drum_pos = current_note_pos / SAMPLES_PER_DRUM;
                if (drum_pos < drum.len) {
                    note_to((drum_pos + 1) * SAMPLES_PER_DRUM, drum.data[drum_pos], stream, pos, len);
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