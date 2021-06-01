#include <cstdint>
#include <cstring>
#include "gamevars.h"
#include "sounds.h"

using namespace ZZT;

static const uint8_t letter_to_tone[7] = {9, 11, 0, 2, 4, 5, 7};

const uint16_t ZZT::sound_notes[NOTE_MAX - NOTE_MIN] = {
	64,	67,	71,	76,	80,	85,	90,	95,	101,	107,	114,	120,	0,	0,	0,	0,
	128,	135,	143,	152,	161,	170,	181,	191,	203,	215,	228,	241,	0,	0,	0,	0,
	256,	271,	287,	304,	322,	341,	362,	383,	406,	430,	456,	483,	0,	0,	0,	0,
	512,	542,	574,	608,	645,	683,	724,	767,	812,	861,	912,	966,	0,	0,	0,	0,
	1024,	1084,	1149,	1217,	1290,	1366,	1448,	1534,	1625,	1722,	1824,	1933,	0,	0,	0,	0,
	2048,	2169,	2298,	2435,	2580,	2733,	2896,	3068,	3250,	3444,	3649,	3866,	0,	0,	0,	0
};

const SoundDrum ZZT::sound_drums[DRUM_MAX - DRUM_MIN] = {
	{1,  {3200,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0}},
	{14, {1100,	1200,	1300,	1400,	1500,	1600,	1700,	1800,	1900,	2000,	2100,	2200,	2300,	2400,	0}},
	{14, {4800,	4800,	8000,	1600,	4800,	4800,	8000,	1600,	4800,	4800,	8000,	1600,	4800,	4800,	8000}},
	{14, {0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0}},
	{14, {500,	2556,	1929,	3776,	3386,	4517,	1385,	1103,	4895,	3396,	874,	1616,	5124,	606,	0}},
	{14, {1600,	1514,	1600,	821,	1600,	1715,	1600,	911,	1600,	1968,	1600,	1490,	1600,	1722,	1600}},
	{14, {2200,	1760,	1760,	1320,	2640,	880,	2200,	1760,	1760,	1320,	2640,	880,	2200,	1760,	0}},
	{14, {688,	676,	664,	652,	640,	628,	616,	604,	592,	580,	568,	556,	544,	532,	0}},
	{14, {1207,	1224,	1163,	1127,	1159,	1236,	1269,	1314,	1127,	1224,	1320,	1332,	1257,	1327,	0}},
	{14, {378,	331,	316,	230,	224,	384,	480,	320,	358,	412,	376,	621,	554,	426,	0}}
};

SoundQueue::SoundQueue() {
    enabled = true;
    block_queueing = false;
    clear();
}

void SoundQueue::queue(int16_t priority, const uint8_t *pattern, int len) {
    if (!block_queueing &&
        (!is_playing || (
                ((priority >= current_priority) && (current_priority != -1))
                || (priority == -1)
            )
        )
    ) {
         if (priority >= 0 || !is_playing) {
            current_priority = priority;
            memcpy(buffer, pattern, len);
            bufpos = 0;
            buflen = len;
        } else {
            if (bufpos > 0) {
                memmove(buffer, buffer + bufpos, buflen - bufpos);
                buflen -= bufpos;
                bufpos = 0;
            }
            if ((buflen + len) < 255) {
                memcpy(buffer + buflen, pattern, len);
                buflen += len;
            }
        }
        is_playing = true;
    }
}

void SoundQueue::clear(void) {
    bufpos = 0;
    buflen = 0;
    is_playing = false;
}

bool SoundQueue::pop(uint16_t &note, uint16_t &duration) {
    if (bufpos >= buflen) {
        return false;
    } else {
        note = buffer[bufpos++];
        duration = buffer[bufpos++];
        if (duration == 0) {
            // emulate ZZT overflow
            duration = 256;
        }
        return true;
    }
}

size_t ZZT::SoundParse(const char *input, uint8_t *output, size_t outlen) {
    uint8_t note_octave = 3;
    uint8_t note_duration = 1;
    uint8_t note_tone;
    size_t inpos;
    size_t outpos = 0;

    for (inpos = 0; inpos < strlen(input); inpos++) {
        // check for overflow
        if ((outpos + 2) >= outlen) break;

        note_tone = -1;
        switch (UpCase(input[inpos])) {
            case 'T': note_duration = 1; break;
            case 'S': note_duration = 2; break;
            case 'I': note_duration = 4; break;
            case 'Q': note_duration = 8; break;
            case 'H': note_duration = 16; break;
            case 'W': note_duration = 32; break;
            case '.': note_duration = note_duration * 3 / 2; break;
            case '3': note_duration = note_duration / 3; break;
            case '+': if (note_octave < 6) note_octave += 1; break;
            case '-': if (note_octave > 1) note_octave -= 1; break;

            case 'A':
            case 'B':
            case 'C':
            case 'D':
            case 'E':
            case 'F':
            case 'G': {
                note_tone = letter_to_tone[(input[inpos] - 'A') & 0x07];

                switch (input[inpos + 1]) {
                    case '!': note_tone -= 1; inpos++; break;
                    case '#': note_tone += 1; inpos++; break;
                }

                output[outpos++] = (note_octave << 4) | note_tone;
                output[outpos++] = note_duration;
            } break;

            case 'X': {
                output[outpos++] = 0;
                output[outpos++] = note_duration;
            } break;

            case '0':
            case '1':
            case '2':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                output[outpos++] = input[inpos] + 0xF0 - '0';
                output[outpos++] = note_duration;
            } break;
        }
    }

    return outpos;
}

bool Game::HasTimeElapsed(int16_t &counter, int16_t duration) {
    int16_t hSecsTotal = driver->get_hsecs();
    uint16_t hSecsDiff = ((hSecsTotal - counter) + 6000) % 6000;

    if (hSecsDiff >= duration) {
        counter = hSecsTotal;
        return true;
    } else {
        return false;
    }
}