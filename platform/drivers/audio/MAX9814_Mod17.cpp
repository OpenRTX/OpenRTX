/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Mathis Schmieder DB9MAT                  *
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

#include "peripherals/gpio.h"
#include "hwconfig.h"
#include "MAX9814.h"

/*
 * Implementation of MAX9814 gain setting
 */

void max9814_setGain(uint8_t gain)
{
    if (gain == 0)
    {
        gpio_setMode(MIC_GAIN, OUTPUT);
        gpio_setPin(MIC_GAIN); // 40 dB gain
    }
    else if (gain == 1)
    {
        gpio_setMode(MIC_GAIN, OUTPUT);
        gpio_clearPin(MIC_GAIN); // 50 dB gain
    }
    else
    {
        gpio_setMode(MIC_GAIN, INPUT); // High impedance, 60 dB gain
    }
}
