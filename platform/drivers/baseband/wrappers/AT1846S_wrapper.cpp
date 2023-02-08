/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include "AT1846S_wrapper.h"
#include "AT1846S.h"

void AT1846S_init()
{
    AT1846S::instance().init();
}

void AT1846S_terminate()
{
    AT1846S::instance().terminate();
}

void AT1846S_setFrequency(const freq_t freq)
{
    AT1846S::instance().setFrequency(freq);
}

void AT1846S_setBandwidth(const AT1846S_bw_t band)
{
    // Conversion is safe: the fields in AT1846S_bw_t enum and in AT1846S_BW
    // enum class concide.
    AT1846S::instance().setBandwidth(static_cast< AT1846S_BW >(band));
}

void AT1846S_setOpMode(const AT1846S_op_t mode)
{
    // Conversion is safe: the fields in AT1846S_op_t enum and in AT1846S_OpMode
    // enum class concide.
    AT1846S::instance().setOpMode(static_cast< AT1846S_OpMode >(mode));
}

void AT1846S_setFuncMode(const AT1846S_func_t mode)
{
    // Conversion is safe: the fields in AT1846S_func_t enum and in
    // AT1846S_FuncMode enum class concide.
    AT1846S::instance().setFuncMode(static_cast< AT1846S_FuncMode >(mode));
}

void AT1846S_enableTxCtcss(tone_t freq)
{
    AT1846S::instance().enableTxCtcss(freq);
}

void AT1846S_disableCtcss()
{
    AT1846S::instance().disableCtcss();
}

int16_t AT1846S_readRSSI()
{
    return AT1846S::instance().readRSSI();
}

void AT1846S_setPgaGain(const uint8_t gain)
{
    AT1846S::instance().setPgaGain(gain);
}

void AT1846S_setMicGain(const uint8_t gain)
{
    AT1846S::instance().setMicGain(gain);
}

void AT1846S_setAgcGain(const uint8_t gain)
{
    AT1846S::instance().setAgcGain(gain);
}

void AT1846S_setTxDeviation(const uint16_t dev)
{
    AT1846S::instance().setTxDeviation(dev);
}

void AT1846S_setRxAudioGain(const uint8_t gainWb, const uint8_t gainNb)
{
    AT1846S::instance().setRxAudioGain(gainWb, gainNb);
}

void AT1846S_setNoise1Thresholds(const uint8_t highTsh, const uint8_t lowTsh)
{
    AT1846S::instance().setNoise1Thresholds(highTsh, lowTsh);
}

void AT1846S_setNoise2Thresholds(const uint8_t highTsh, const uint8_t lowTsh)
{
    AT1846S::instance().setNoise2Thresholds(highTsh, lowTsh);
}

void AT1846S_setRssiThresholds(const uint8_t highTsh, const uint8_t lowTsh)
{
    AT1846S::instance().setRssiThresholds(highTsh, lowTsh);
}

void AT1846S_setPaDrive(const uint8_t value)
{
    AT1846S::instance().setPaDrive(value);
}

void AT1846S_setAnalogSqlThresh(const uint8_t thresh)
{
    AT1846S::instance().setAnalogSqlThresh(thresh);
}
