/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

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
 * interface "beeps". It also provides a means to encode AFSK/4FSK data and to
 * reproduce arbitrary audio samples.
 *
 * WARNING: this driver implements a priority mechanism between "beeps", FSK
 * modulation and audio playback. A request for FSK modulation or audio playback
 * always interrupts the generation of a "beep" tone and generation of "beep"
 * tones is disabled when FSK modulation or audio playback is active.
 *
 * This driver uses the following peripherals of the STM32F405 MCU:
 * - TIM3 as high-frequency PWM timebase.
 * - TIM14 as timebase for CTCSS and "beeps" through a sine table.
 * - TIM7 and DMA1_Stream2 for AFSK, 4FSK and playback of audio streams.
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
 * Encode a given data stream using Bell 202 scheme at 1200 baud, sending the
 * audio stream to both the speaker and the rtx baseband IC.
 * This function blocks the execution flow until all data has been sent.
 *
 * @param buf: pointer to a buffer containing data to be encoded.
 * @param len: length of the data buffer.
 */
void toneGen_encodeAFSK1200(const uint8_t *buf, const size_t len);

/**
 * Reproduce an audio stream, sending audio stream to both the speaker and the
 * rtx baseband IC.
 * This function returns immediately and the stream is reproduced in background.
 * The calling thread can be made waiting for transfer completion by calling the
 * corresponding API function.
 *
 * WARNING: the underlying peripheral accepts ONLY 16 bit transfers, while the
 * PWM resolution is 8 bit. Thus, the sample buffer MUST be of uint16_t elements
 * and, when filling it with data, one must remember that the upper 8 bit are
 * DISCARDED.
 *
 * @param buf: pointer to a buffer containing the audio samples.
 * @param len: length of the data buffer.
 * @param sampleRate: sample rate of the audio stream in samples per second.
 * @param circMode: treat buf as a double circular buffer, continuously
 * reproducing its content.
 */
void toneGen_playAudioStream(const uint16_t *buf, const size_t len,
                             const uint32_t sampleRate, const bool circMode);

/**
 * When called, this function blocks the execution flow until the reproduction
 * of a previously started audio stream or AFSK modulation terminates.
 *
 * @return false if there is no ongoing stream or if another thread is already
 * pending, true otherwise.
 */
bool toneGen_waitForStreamEnd();

/**
 * Interrupt the ongoing reproduction of an audio stream, also making the
 * toneGen_waitForStreamEnd return to the caller.
 */
void toneGen_stopAudioStream();

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
