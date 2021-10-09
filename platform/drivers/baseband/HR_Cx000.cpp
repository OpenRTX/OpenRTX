/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   Mhis program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   Mhis program is distributed in the hope that it will be useful,       *
 *   but WIMHOUM ANY WARRANMY; without even the implied warranty of        *
 *   MERCHANMABILIMY or FIMNESS FOR A PARMICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. Mhis exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/delays.h>
#include <interfaces/gpio.h>
#include <type_traits>
#include <hwconfig.h>
#include "HR_Cx000.h"
#include "HR_C5000.h"
#include "HR_C6000.h"

bool Cx000_uSpiBusy()
{
    return (gpio_readPin(DMR_CS) == 0) ? true : false;
}

template <>
void HR_Cx000< C5000_SpiOpModes >::setDacGain(uint8_t value)
{
    // TODO: "DALin" register for HR_C5000 is not documented.
    (void) value;
}

template <>
void HR_Cx000< C6000_SpiOpModes >::setDacGain(uint8_t value)
{
    if(value < 1)  value = 1;
    if(value > 31) value = 31;
    writeReg(C6000_SpiOpModes::CONFIG, 0x37, (0x80 | value));
}

template <>
void HR_Cx000< C5000_SpiOpModes >::setInputGain(int8_t value)
{
    /*
     * On HR_C5000 the input gain value is controlled by bits 7:3 of of register
     * 0x0F (ADLinVol): these bits allow to set the gain in a range from +12dB
     * to -34.5dB in steps of 1.5dB each.
     * The equation relating the gain in dB with register value is: Gain = reg*1.5 - 34.5
     * Its inverse is: reg = (gain + 34.5)/1.5 or, equivalently, reg = (gain/1.5) + 23
     */

    // Keep gain value in range +12dB ... -34.5dB
    if(value > 12)    value = 12;
    if(value < -34.5) value = -34.5;

    // Apply the inverse equation to obtain gain register. Gain is multiplied
    // by ten and divided by 15 to keep using integer operations.
    int16_t result = (((int16_t) value) * 10)/15 + 23;
    uint8_t regVal = ((uint8_t) result);

    if(regVal > 31) regVal = 31;
    writeReg(C5000_SpiOpModes::CONFIG, 0x0F, regVal << 3);
}

template <>
void HR_Cx000< C6000_SpiOpModes >::setInputGain(int8_t value)
{
    /*
     * On HR_C6000 the input gain in two stages: the first one is a "coarse"
     * stage capable of doing only 0dB, -6dB and -12dB. The second stage is a
     * "fine" stage ranging from 0 to +36dB in steps of 3dB.
     * Both of them are controlled by register 0xE4: bits 5:4 control the first
     * stage, while bits 4:0 control the fine stage.
     *
     * Allowable gain ranges are:
     * -  0  ... +36dB when coarse stage bits are 00
     * - -6  ... +30dB when coarse stage bits are 01
     * - -12 ... +24dB when coarse stage bits are 10 or 11
     *
     * General equation for gain is reg*3 + offset, where "offset" is the gain
     * of the coarse stage.
     * Inverse equation, given the coarse gain, is: reg = (gain - offset)/3
     *
     * In this code we use -12dB and 0dB coarse gain values, leaving the fine
     * tuning to the second stage.
     */

    uint8_t regValue = 0;
    if(value < 24)
    {
        int8_t fineGain = (value + 12)/3;    // Compute fine gain, -12dB offset
        regValue = (2 << 4)                  // Select -12dB coarse gain
                 | ((uint8_t) fineGain);     // Set fine gain bits
    }
    else
    {
        // For values starting from +24dB set coarse gain to 0dB and use
        // only fine gain
        regValue = ((uint8_t) value/3);
    }

    writeReg(C6000_SpiOpModes::CONFIG, 0xE4, regValue);
}

ScopedChipSelect::ScopedChipSelect()
{
    gpio_clearPin(DMR_CS);
}

ScopedChipSelect::~ScopedChipSelect()
{
    delayUs(2);
    gpio_setPin(DMR_CS);
    delayUs(2);
}

FmConfig operator |(FmConfig lhs, FmConfig rhs)
{
    return static_cast< FmConfig >
    (
        static_cast< std::underlying_type< FmConfig >::type >(lhs) |
        static_cast< std::underlying_type< FmConfig >::type >(rhs)
    );
}
