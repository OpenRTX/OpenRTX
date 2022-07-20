#ifndef NOISE_GATE_H
#define NOISE_GATE_H

#include <dsp.h>

typedef struct {
    float threshold; // cutoff threshold in dB
    float holdTime; // time to hold the gate open in seconds

    float attackCoef;
    float releaseCoef;

    float attackCounter; // attack counter, seconds

    float smoothedGain;
    float sampleTime; // sample time in seconds
} noise_gate_t;

void noise_gate_init(noise_gate_t *gate, float threshold, float attackTimeMs, float releaseTimeMs, float holdTimeMs, float sampleRateHz);

audio_sample_t noise_gate_update(noise_gate_t *gate, audio_sample_t inp);

void noise_gate_set_threshold(noise_gate_t *gate, float thresholddB);

void noise_gate_set_attack_release_time(noise_gate_t *gate, float attackTimeMs, float releaseTimeMs, float sampleRateHz);

#endif