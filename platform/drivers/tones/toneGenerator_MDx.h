/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef TONE_GENERATOR_H
#define TONE_GENERATOR_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Tone generator for MDx family, used primarily for CTCSS tones and user
 * interface "beeps". It also provides an high-frequency PWM timebase to encode
 * AFSK/4FSK data and to reproduce arbitrary audio samples.
 *
 * WARNING: this driver implements a priority mechanism between "beeps", FSK
 * modulation and audio playback. A request for FSK modulation or audio playback
 * always interrupts the generation of a "beep" tone and generation of "beep"
 * tones is disabled when FSK modulation or audio playback is active.
 *
 * This driver uses the following peripherals of the STM32F405 MCU:
 * - TIM3 as high-frequency PWM timebase.
 * - TIM14 as timebase for CTCSS and "beeps" through a sine table.
 */

/**
 * Initialise tone generator.
 */
void toneGen_init();

/**
 * Terminate tone generator.
 */
void toneGen_terminate();

/**
 * Set frequency for CTCSS tone generation.
 *
 * @param toneFreq: CTCSS tone frequency.
 */
void toneGen_setToneFreq(const float toneFreq);

/**
 * Activate generation of CTCSS tone.
 */
void toneGen_toneOn();

/**
 * Terminate generation of CTCSS tone.
 */
void toneGen_toneOff();

/**
 * Activate generation of a "beep" tone, with a given duration.
 * Audio is sent both to the speaker and to the rtx baseband IC.
 *
 * @param beepFreq: frequency of "beep" tone in Hz.
 * @param volume: "beep" output volume, range 0 - 255.
 * @param duration: tone duration in milliseconds, zero for infinite duration.
 */
void toneGen_beepOn(const float beepFreq, const uint8_t volume,
                    const uint32_t duration);

/**
 * Terminate generation of "beep" tone, irrespectively of its duration.
 */
void toneGen_beepOff();

/**
 * Disable the generation of "beep" tones until the unlock function is called.
 * This function supports recursive call: multiple subsequent calls require an
 * equal number of calls of the unlock function to effectively unlock beeps.
 * This function can be called safely from an interrupt routine.
 */
void toneGen_lockBeep();

/**
 * Enable the generation of "beep" tones previously unlocked.
 * This function can be called safely from an interrupt routine.
 */
void toneGen_unlockBeep();

/**
 * Check if generation of "beep" tones is disabled.
 *
 * @return if generation of "beep" tones is disabled.
 */
bool toneGen_beepLocked();

/**
 * Get the current status of the "beep"/AFSK/audio generator stage.
 *
 * @return true if the tone generator is busy.
 */
bool toneGen_toneBusy();


#ifdef __cplusplus
}
#endif

#endif /* TONE_GENERATOR_H */
