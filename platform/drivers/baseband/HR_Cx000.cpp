/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/delays.h"
#include "peripherals/gpio.h"
#include <type_traits>
#include "hwconfig.h"
#include "drivers/baseband/HR_Cx000.h"
#include "drivers/baseband/HR_C5000.h"
#include "drivers/baseband/HR_C6000.h"

template <>
void HR_Cx000< C5000_SpiOpModes >::setDacGain(int8_t gain)
{
    // TODO: "DALin" register for HR_C5000 is not documented.
    (void) gain;
}

template <>
void HR_Cx000< C6000_SpiOpModes >::setDacGain(int8_t gain)
{
    // If gain is at minimum, just turn off DAC output and return
    if(gain <= -31)
    {
        writeReg(C6000_SpiOpModes::CONFIG, 0xE2, 0x00);
        return;
    }

    uint8_t value = 0x80 | (((uint8_t) gain) & 0x1F);
    if(gain > 0)
        value |= 0x40;

    writeReg(C6000_SpiOpModes::CONFIG, 0x37, value);    // DAC gain
    writeReg(C6000_SpiOpModes::CONFIG, 0xE2, 0x02);     // Enable DAC
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

ScopedChipSelect::ScopedChipSelect(const struct spiDevice *spi,
                                   const struct gpioPin& cs) : spi(spi), cs(cs)
{
    spi_acquire(spi);
    gpioPin_clear(&cs);
}

ScopedChipSelect::~ScopedChipSelect()
{
    // NOTE: it seems that, without the 2us delays before and after the nCS
    // deassertion, the HR_C6000 doesn't work properly. At least this is what
    // has been observed on the Retevis RT3s.
    delayUs(2);
    gpioPin_set(&cs);
    delayUs(2);
    spi_release(spi);
}

FmConfig operator |(FmConfig lhs, FmConfig rhs)
{
    return static_cast< FmConfig >
    (
        static_cast< std::underlying_type< FmConfig >::type >(lhs) |
        static_cast< std::underlying_type< FmConfig >::type >(rhs)
    );
}
