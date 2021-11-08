/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#ifndef SETTINGS_H
#define SETTINGS_H

#include <hwconfig.h>

typedef struct
{
    uint8_t brightness;     // Display brightness
    uint8_t contrast;       // Display contrast
    uint8_t sqlLevel;       // Squelch level
    uint8_t voxLevel;       // Vox level
    int8_t utc_timezone;    // Timezone
    bool gps_enabled;       // GPS active
    char callsign[10];      // Plaintext callsign, for future use
}
__attribute__((packed)) settings_t;


static const settings_t default_settings =
{
    255,              // Brightness
#ifdef SCREEN_CONTRAST
    DEFAULT_CONTRAST, // Contrast
#else
    255,              // Contrast
#endif
    4,                // Squelch level, 4 = S3
    0,                // Vox level
    0,                // UTC Timezone
    false,            // GPS enabled
    ""                // Empty callsign
};

#endif /* SETTINGS_H */
