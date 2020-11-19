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

/**
 * Tone generator for MDxx380 family, used for both CTCSS tones and user
 * interface "beeps".
 * This driver uses TIM3 of STM32F405 mcu in PWM mode to generate sinewaves
 * using a precomputed sine table.
 */

/**
 * Initialise tone generator.
 */
void toneGen_init();

/**
 * Terminate tone generator.
 */
void toneGen_shutdown();

/**
 * Set frequency for CTCSS tone generation.
 * @param toneFreq: CTCSS tone frequency.
 */
void toneGen_setToneFreq(float toneFreq);

/**
 * Activate generation of CTCSS tone.
 */
void toneGen_toneOn();

/**
 * Terminate generation of CTCSS tone.
 */
void toneGen_toneOff();

/**
 * Set frequency for user interface "beep".
 * @param beepFreq: frequency of "beep" tone.
 */
void toneGen_setBeepFreq(float beepFreq);

/**
 * Activate generation of "beep" tone.
 */
void toneGen_beepOn();

/**
 * Terminate generation of "beep" tone.
 */
void toneGen_beepOff();

/**
 * Activate generation of "beep" tone with automatic termination after a given
 * amount of time.
 * @param duration: duration of "beep" tone, in milliseconds.
 */
void toneGen_timedBeep(uint16_t duration);

#endif /* TONE_GENERATOR_H */
