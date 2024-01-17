/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Silvano Seva IU2KWO                      *
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
#include <datetime.h>
#include <hwconfig.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Standard interface for device-specific hardware.
 * This interface handles:
 * - LEDs
 * - Battery voltage read
 * - Microphone level
 * - Volume and channel selectors
 * - Screen backlight control
 */

/**
 * \enum led_t Enumeration type for platform LED control. To allow for a wide
 * platform support, LEDs are identified by their color.
 */
typedef enum
{
        GREEN = 0,
        RED,
        YELLOW,
        WHITE,
} led_t;

/**
 * \struct hwInfo_t Data structure collecting all the hardware-dependent
 * information: UHF and VHF band support, manufacturer-assigned hardware name
 * and LCD driver type.
 */
typedef struct
{
    char     name[10];      /* Manufacturer-assigned hardware name.             */

    uint8_t  hw_version;    /* Hardware version number                          */

    uint8_t  _unused  : 6,
             uhf_band : 1,  /* Device allows UHF band operation.                */
             vhf_band : 1;  /* Device allows VHF band operation.                */

    uint16_t uhf_maxFreq;   /* Upper bound for UHF band, in MHz.                */
    uint16_t uhf_minFreq;   /* Lower bound for UHF band, in MHz.                */

    uint16_t vhf_maxFreq;   /* Upper bound for VHF band, in MHz.                */
    uint16_t vhf_minFreq;   /* Lower bound for VHF band, in MHz.                */

    void     *other;        /* Void pointer to device-specific data structure   */

} hwInfo_t;


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
 * This function reads and returns the current battery voltage in millivolt.
 */
uint16_t platform_getVbat();

/**
 * This function reads and returns the current microphone input level as a
 * normalised value between 0 and 255.
 */
uint8_t platform_getMicLevel();

/**
 * This function reads and returns the current volume selector level as a
 * normalised value between 0 and 255.
 */
uint8_t platform_getVolumeLevel();

/**
 * This function reads and returns the current channel selector level.
 */
int8_t platform_getChSelector();

/**
 * This function reads and returns the current PTT status.
 */
bool platform_getPttStatus();

/**
 * This function reads and returns the current status of the power on/power off
 * button or switch.
 * @return true if power is enabled, false otherwise.
 */
bool platform_pwrButtonStatus();

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

#ifdef CONFIG_RTC
/**
 * Get current UTC date and time.
 * @return structure of type datetime_t with current clock and calendar values.
 */
datetime_t platform_getCurrentTime();

/**
 * Set date and time to a given value.
 * @param t: struct of type datetime_t, holding the new time to be set.
 */
void platform_setTime(datetime_t t);
#endif

/**
 * This function returns a pointer to a data structure containing all the
 * hardware information.
 * WARNING: calling code must ensure that free() is never called on the returned
 * pointer!
 * @return pointer to device's hardware information.
 */
const hwInfo_t *platform_getHwInfo();

#ifdef __cplusplus
}
#endif

#endif /* PLATFORM_H */
