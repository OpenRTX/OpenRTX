/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/battery.h"
#include "hwconfig.h"
#include <math.h>

/*
 * Minimum and maximum battery voltages expressed in fixed point Q8.8 format.
 * Obtained by multiplying the values in volt by 256.
 */

#if defined(CONFIG_BAT_LIPO)
static const uint16_t vcell_max = 0x0414;   // 4.08V
static const uint16_t vcell_min = 0x039C;   // 3.61V
#elif defined (CONFIG_BAT_LIION)
static const uint16_t vcell_max = 0x0433;   // 4.15V
static const uint16_t vcell_min = 0x0333;   // 3.2V
#elif defined(CONFIG_BAT_NONE)
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

    const uint32_t vbat_max = vcell_max * CONFIG_BAT_NCELLS;
    const uint32_t vbat_min = vcell_min * CONFIG_BAT_NCELLS;

    uint32_t vb = vbat << 16;
    vb          = vb / 1000;
    vb          = (vb + 256) >> 8;

    /*
     * If the voltage is below minimum we return 0 to prevent an underflow in
     * the following calculation
     */
    if (vb < vbat_min) return 0;

    uint32_t diff   = vb - vbat_min;
    uint32_t range  = vbat_max - vbat_min;
    uint32_t result = ((diff << 8) / range) * 100;
    result         += 128;
    result         >>= 8;
    if(result > 100) result = 100;

    return result;

    #endif
}
