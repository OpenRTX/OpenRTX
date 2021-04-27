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
void HR_Cx000< C5000_SpiOpModes >::setInputGain(uint8_t value)
{
    // Mic gain is controlled by bytes 7:3 of register 0x0F (ADLinVol).
    if(value > 31) value = 31;
    writeReg(C5000_SpiOpModes::CONFIG, 0x0F, value << 3);
}

template <>
void HR_Cx000< C6000_SpiOpModes >::setInputGain(uint8_t value)
{
    // TODO: Experiments needed for identification of HR_C6000 mic gain register.
    (void) value;
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
