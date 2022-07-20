#include <noise_gate.h>
#include <arm_math.h>
#include <dsp.h>


void noise_gate_init(noise_gate_t *gate, float thresholddB, float attackTimeMs, float releaseTimeMs, float holdTimeMs, float sampleRateHz) {
    noise_gate_set_threshold(gate, thresholddB);
    gate->holdTime = holdTimeMs / 1000.0f;

    noise_gate_set_attack_release_time(gate, attackTimeMs, releaseTimeMs, sampleRateHz);

    gate->sampleTime = 1.0f / sampleRateHz;

    gate->attackCounter = 0.0f;

    gate->smoothedGain = 0.0f;
}

audio_sample_t noise_gate_update(noise_gate_t *gate, audio_sample_t inp) {
    float inpAbs = fabsf((float) inp);
    float gain = inpAbs >= gate->threshold ? 1.0f : 0.0f;

    if (gate->attackCounter > gate->holdTime && gain <= gate->smoothedGain) { // Check if hold time met
        // Start decreasing gain
        gate->smoothedGain = gate->attackCoef * gate->smoothedGain + (1.0f - gate->attackCoef) * gain;
    } else if (gate->attackCounter <= gate->holdTime) { // Hold time not met, so increment counter
        gate->attackCounter += gate->sampleTime; // tick attack counter
    } else if (gain > gate->smoothedGain) {
        gate->smoothedGain = gate->releaseCoef * gate->smoothedGain + (1.0f - gate->releaseCoef) * gain;
        gate->attackCounter = 0.0f;
    }

    return (audio_sample_t) (inp * gate->smoothedGain);
}

void noise_gate_set_threshold(noise_gate_t *gate, float thresholddB) {
    // Bounding
    // if (thresholddB > 0.0f) {
    //     thresholddB = 0.0f;
    // } else if (thresholddB < -140.0f) {
    //     thresholddB = -140.0f;
    // }

    gate->threshold = thresholddB;
}

void noise_gate_set_attack_release_time(noise_gate_t *gate, float attackTimeMs, float releaseTimeMs, float sampleRateHz) {
    gate->attackCoef = expf(-logf(9.0f) / (sampleRateHz * (attackTimeMs/1000.0f)));
    gate->releaseCoef = expf(-logf(9.0f) / (sampleRateHz * (releaseTimeMs/1000.0f)));
}
