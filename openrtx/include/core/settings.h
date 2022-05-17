/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <hwconfig.h>
#include <stdbool.h>

typedef enum
{
    TIMER_OFF =  0,
    TIMER_5S  =  1,
    TIMER_10S =  2,
    TIMER_15S =  3,
    TIMER_20S =  4,
    TIMER_25S =  5,
    TIMER_30S =  6,
    TIMER_1M  =  7,
    TIMER_2M  =  8,
    TIMER_3M  =  9,
    TIMER_4M  = 10,
    TIMER_5M  = 11,
    TIMER_15M = 12,
    TIMER_30M = 13,
    TIMER_45M = 14,
    TIMER_1H  = 15
}
display_timer_t;

typedef struct
{
    uint8_t brightness;           // Display brightness
    uint8_t contrast;             // Display contrast
    uint8_t sqlLevel;             // Squelch level
    uint8_t voxLevel;             // Vox level
    int8_t  utc_timezone;         // Timezone
    bool    gps_enabled;          // GPS active
    char    callsign[10];         // Plaintext callsign, for future use
    uint8_t display_timer : 4,    // Standby timer
            not_in_use    : 4;
	uint8_t vpLevel 		: 3,
			vpPhoneticSpell	: 1,
			vpReserved    	: 4; // reserved for voice rate on the fly.

}
__attribute__((packed)) settings_t;


static const settings_t default_settings =
{
    100,              // Brightness
#ifdef SCREEN_CONTRAST
    DEFAULT_CONTRAST, // Contrast
#else
    255,              // Contrast
#endif
    4,                // Squelch level, 4 = S3
    0,                // Vox level
    0,                // UTC Timezone
    false,            // GPS enabled
    "",               // Empty callsign
    TIMER_30S,        // 30 seconds
    0,                 // not in use
	0, // vpOff,
	0, // phonetic spell off,
	0 // not in use.
};

#endif /* SETTINGS_H */
