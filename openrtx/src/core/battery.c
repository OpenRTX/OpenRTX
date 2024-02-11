/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#include <battery.h>
#include <hwconfig.h>
#include <math.h>

/*
 * Minimum and maximum battery voltages expressed in fixed point Q8.8 format.
 * Obtained by multiplying the values in volt by 256.
 */

#if defined CONFIG_BAT_LIPO_1S
static const uint16_t bat_v_min = 0x039C;   // 3.61V
static const uint16_t bat_v_max = 0x0426;   // 4.15V
#elif defined CONFIG_BAT_LIPO_2S
static const uint16_t bat_v_min = 0x071A;   // 7.10V
static const uint16_t bat_v_max = 0x0819;   // 8.10V
#elif defined CONFIG_BAT_LIPO_3S
static const uint16_t bat_v_min = 0x0AD4;   // 10.83V
static const uint16_t bat_v_max = 0x0C73;   // 12.45V
#elif defined CONFIG_BAT_NONE
// Nothing to do, just avoid arising the compiler error
#else
#error Please define a battery type into platform/targets/.../hwconfig.h
#endif

uint8_t battery_getCharge(uint16_t vbat)
{
    #ifdef CONFIG_BAT_NONE
    /* Return full charge if no battery is present. */
    (void) vbat;
    return 100;
    #else

    /*
     * Compute battery percentage by linear interpolation between zero and full
     * charge voltage values using Q8.8 fixed-point math to avoid using both
     * floating point and 64 bit variables, for maximum portability.
     *
     * Given that battery voltage parameter is an unsigned 16 bit value expressing
     * the voltage in mV, we first have to convert it to Q8.8 before computing
     * the charge percentage.
     *
     * Comparison between battery percentage computed using fixed point and
     * floating point routines on a voltage range from 10.83V to 12.45V with
     * increments of 1mV resulted in an average error of -0.015%, maximum error
     * of 0.79% and minimum error of -0.78%
     */

    uint32_t vb = vbat << 16;
    vb          = vb / 1000;
    vb          = (vb + 256) >> 8;

    /*
     * If the voltage is below minimum we return 0 to prevent an underflow in
     * the following calculation
     */
    if (vb < bat_v_min) return 0;

    uint32_t diff   = vb - bat_v_min;
    uint32_t range  = bat_v_max - bat_v_min;
    uint32_t result = ((diff << 8) / range) * 100;
    result         += 128;
    result         >>= 8;
    if(result > 100) result = 100;

    return result;

    #endif
}
