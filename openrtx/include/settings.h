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

typedef struct
{
    uint8_t valid[6];     // Should contain "OPNRTX" in a valid settings_t
    uint8_t brightness;
    uint8_t contrast;
    int8_t utc_timezone;
    bool gps_enabled;
    bool gps_set_time;
}
__attribute__((packed)) settings_t;


static const settings_t default_settings =
{
    "OPNRTX",  // Settings valid string
    255,       // Brightness
    71,        // Contrast
    0,         // UTC Timezone
    false,     // GPS enabled
    true       // GPS set time
};

#endif /* SETTINGS_H */
