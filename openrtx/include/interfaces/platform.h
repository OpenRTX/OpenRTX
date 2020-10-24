/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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

#ifndef PLATFORM_H
#define PLATFORM_H

#include <stdint.h>
#include <stdbool.h>

/**
 * Standard interface for device-specific hardware.
 * This interface handles:
 * - LEDs
 * - Battery voltage read
 * - Microphone level
 * - Volume and channel selectors
 * - Screen backlight control
 */

typedef enum
{
        GREEN = 0,
        RED,
        YELLOW,
        WHITE,
} led_t;

/**
 * This function handles device hardware initialization.
 * Usually called at power-on
 */
void platform_init();

/**
 * This function handles device hardware de-initialization.
 * Usually called at power-down
 */
void platform_terminate();

/**
 * This function reads and returns the current battery voltage.
 */
float platform_getVbat();

/**
 * This function reads and returns the current microphone input level.
 */
float platform_getMicLevel();

/**
 * This function reads and returns the current volume selector level.
 */
float platform_getVolumeLevel();

/**
 * This function reads and returns the current channel selector level.
 */
uint8_t platform_getChSelector();

/**
 * This function reads and returns the current PTT status.
 */
bool platform_getPttStatus();

/**
 * This function turns on the selected led.
 * @param led: which led to control
 */
void platform_ledOn(led_t led);

/**
 * This function turns off the selected led.
 * @param led: which led to control
 */
void platform_ledOff(led_t led);

/**
 * This function emits a tone of the specified frequency from the speaker.
 * @param freq: desired frequency
 */
void platform_beepStart(uint16_t freq);

/**
 * This function stops emitting a tone.
 */
void platform_beepStop();

/**
 * This function sets the screen backlight to the specified level.
 * @param level: backlight level, from 0 (backlight off) to 255 (backlight at
 * full brightness).
 */
void platform_setBacklightLevel(uint8_t level);

#endif /* PLATFORM_H */
