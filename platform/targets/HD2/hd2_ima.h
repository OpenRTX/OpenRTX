/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * IMA-ADPCM (4-bit) decode -- integer-only, realtime on the HD2's soft-float
 * CK803S.  Matches the host encoder in scripts/vpc_to_adpcm.py.  Used by the
 * voice-prompt player (voicePrompts_HD2.c) and the diag test (hd2_pcm_stream).
 *
 * Mono, 8 kHz, no per-block header: each clip is a flat nibble stream decoded
 * from a fixed start state {pred=0, index=0}; low nibble of each byte is the
 * earlier sample.
 */
#ifndef HD2_IMA_H
#define HD2_IMA_H

#include <stdint.h>

static const int HD2_IMA_STEP[89] = {
    7,8,9,10,11,12,13,14,16,17,19,21,23,25,28,31,34,37,41,45,50,55,60,66,73,80,
    88,97,107,118,130,143,157,173,190,209,230,253,279,307,337,371,408,449,494,
    544,598,658,724,796,876,963,1060,1166,1282,1411,1552,1707,1878,2066,2272,
    2499,2749,3024,3327,3660,4026,4428,4871,5358,5894,6484,7132,7845,8630,9493,
    10442,11487,12635,13899,15289,16818,18500,20350,22385,24623,27086,29794,32767};
static const int HD2_IMA_IDX[16] = {-1,-1,-1,-1,2,4,6,8,-1,-1,-1,-1,2,4,6,8};

typedef struct { int pred; int index; } hd2_ima_state;

static inline void hd2_ima_reset(hd2_ima_state *s) { s->pred = 0; s->index = 0; }

static inline int16_t hd2_ima_decode(hd2_ima_state *s, uint8_t code)
{
    int step = HD2_IMA_STEP[s->index];
    int diff = step >> 3;
    if(code & 4) diff += step;
    if(code & 2) diff += step >> 1;
    if(code & 1) diff += step >> 2;
    if(code & 8) s->pred -= diff; else s->pred += diff;
    if(s->pred >  32767) s->pred =  32767;
    if(s->pred < -32768) s->pred = -32768;
    s->index += HD2_IMA_IDX[code & 0xf];
    if(s->index < 0)  s->index = 0;
    if(s->index > 88) s->index = 88;
    return (int16_t)s->pred;
}

#endif /* HD2_IMA_H */
