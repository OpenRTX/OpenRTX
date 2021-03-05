/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
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

#include <interfaces/delays.h>
#include "interfaces.h"
#include "AT1846S.h"

void _maskSetRegister(uint8_t reg, uint16_t mask, uint16_t value)
{
    uint16_t regVal = i2c_readReg16(reg);
    regVal = (regVal & ~mask) | (value & mask);
    i2c_writeReg16(reg, regVal);
}

/*
 * NOTE: after some main AT1846S parameters (frequency, bandwidth, ...) are changed,
 * the chip must be "cycled" to make them effective. Cycling consists of turning
 * both TX and RX off and then switch back to the desired functionality.
 *
 * The function _reloadConfig() provides an helper to do this.
 */
static inline void _reloadConfig()
{
    uint16_t reg = i2c_readReg16(0x30);    /* Get current op. status */
    i2c_writeReg16(0x30, reg & 0xFF9F);    /* RX and TX off          */
    i2c_writeReg16(0x30, reg);             /* Restore op. status     */
}


void AT1846S_init()
{
    i2c_writeReg16(0x30, 0x0001);   /* Soft reset              */
    delayMs(100);

    i2c_writeReg16(0x30, 0x0004);

    i2c_writeReg16(0x04, 0x0FD0);
    i2c_writeReg16(0x0A, 0x7C20);
    i2c_writeReg16(0x13, 0xA100);
    i2c_writeReg16(0x1F, 0x1001);
    i2c_writeReg16(0x31, 0x0031);
    i2c_writeReg16(0x33, 0x44A5);
    i2c_writeReg16(0x34, 0x2B89);
    i2c_writeReg16(0x41, 0x4122);
    i2c_writeReg16(0x42, 0x1052);
    i2c_writeReg16(0x43, 0x0100);
    i2c_writeReg16(0x44, 0x07FF);
    i2c_writeReg16(0x59, 0x0B90);
    i2c_writeReg16(0x47, 0x7F2F);
    i2c_writeReg16(0x4F, 0x2C62);
    i2c_writeReg16(0x53, 0x0094);
    i2c_writeReg16(0x54, 0x2A3C);
    i2c_writeReg16(0x55, 0x0081);
    i2c_writeReg16(0x56, 0x0B02);
    i2c_writeReg16(0x57, 0x1C00);
    i2c_writeReg16(0x58, 0x9CDD);
    i2c_writeReg16(0x5A, 0x06DB);
    i2c_writeReg16(0x63, 0x16AD);
    i2c_writeReg16(0x67, 0x0628);
    i2c_writeReg16(0x68, 0x05E5);
    i2c_writeReg16(0x69, 0x0555);
    i2c_writeReg16(0x6A, 0x04B8);
    i2c_writeReg16(0x6B, 0x02FE);
    i2c_writeReg16(0x6C, 0x01DD);
    i2c_writeReg16(0x6D, 0x00B1);
    i2c_writeReg16(0x6E, 0x0F82);
    i2c_writeReg16(0x6F, 0x017A);
    i2c_writeReg16(0x70, 0x004C);
    i2c_writeReg16(0x71, 0x0F1D);
    i2c_writeReg16(0x72, 0x0D91);
    i2c_writeReg16(0x73, 0x0A3E);
    i2c_writeReg16(0x74, 0x090F);
    i2c_writeReg16(0x75, 0x0833);
    i2c_writeReg16(0x76, 0x0806);

    i2c_writeReg16(0x30, 0x40A4);
    delayMs(100);

    i2c_writeReg16(0x30, 0x40A6);   /* Start calibration       */
    delayMs(100);
    i2c_writeReg16(0x30, 0x4006);   /* Stop calibration        */
    delayMs(100);

    i2c_writeReg16(0x40, 0x0031);
}

void AT1846S_terminate()
{
    AT1846S_disableCtcss();
    AT1846S_setFuncMode(AT1846S_OFF);
}

void AT1846S_setFrequency(const freq_t freq)
{
    /* The value to be written in registers is given by: 0.0016*freqency */
    uint32_t val = (freq/1000)*16;
    uint16_t fHi = (val >> 16) & 0xFFFF;
    uint16_t fLo = val & 0xFFFF;

    i2c_writeReg16(0x05, 0x8763);
    i2c_writeReg16(0x29, fHi);
    i2c_writeReg16(0x2A, fLo);

    _reloadConfig();
}

void AT1846S_setBandwidth(const AT1846S_bw_t band)
{
    if(band == AT1846S_BW_25)
    {
        /* 25kHz bandwidth */
        i2c_writeReg16(0x15, 0x1F00);
        i2c_writeReg16(0x32, 0x7564);
        i2c_writeReg16(0x3A, 0x04C3);
        i2c_writeReg16(0x3C, 0x1B34);
        i2c_writeReg16(0x3F, 0x29D1);
        i2c_writeReg16(0x48, 0x1F3C);
        i2c_writeReg16(0x60, 0x0F17);
        i2c_writeReg16(0x62, 0x3263);
        i2c_writeReg16(0x65, 0x248A);
        i2c_writeReg16(0x66, 0xFFAE);
        i2c_writeReg16(0x7F, 0x0001);
        i2c_writeReg16(0x06, 0x0024);
        i2c_writeReg16(0x07, 0x0214);
        i2c_writeReg16(0x08, 0x0224);
        i2c_writeReg16(0x09, 0x0314);
        i2c_writeReg16(0x0A, 0x0324);
        i2c_writeReg16(0x0B, 0x0344);
        i2c_writeReg16(0x0C, 0x0384);
        i2c_writeReg16(0x0D, 0x1384);
        i2c_writeReg16(0x0E, 0x1B84);
        i2c_writeReg16(0x0F, 0x3F84);
        i2c_writeReg16(0x12, 0xE0EB);
        i2c_writeReg16(0x7F, 0x0000);
        _maskSetRegister(0x30, 0x3000, 0x3000);
    }
    else
    {
        /* 12.5kHz bandwidth */
        i2c_writeReg16(0x15, 0x1100);
        i2c_writeReg16(0x32, 0x4495);
        i2c_writeReg16(0x3A, 0x00C3);
        i2c_writeReg16(0x3F, 0x29D1);
        i2c_writeReg16(0x3C, 0x1B34);
        i2c_writeReg16(0x48, 0x19B1);
        i2c_writeReg16(0x60, 0x0F17);
        i2c_writeReg16(0x62, 0x1425);
        i2c_writeReg16(0x65, 0x2494);
        i2c_writeReg16(0x66, 0xEB2E);
        i2c_writeReg16(0x7F, 0x0001);
        i2c_writeReg16(0x06, 0x0014);
        i2c_writeReg16(0x07, 0x020C);
        i2c_writeReg16(0x08, 0x0214);
        i2c_writeReg16(0x09, 0x030C);
        i2c_writeReg16(0x0A, 0x0314);
        i2c_writeReg16(0x0B, 0x0324);
        i2c_writeReg16(0x0C, 0x0344);
        i2c_writeReg16(0x0D, 0x1344);
        i2c_writeReg16(0x0E, 0x1B44);
        i2c_writeReg16(0x0F, 0x3F44);
        i2c_writeReg16(0x12, 0xE0EB);
        i2c_writeReg16(0x7F, 0x0000);
        _maskSetRegister(0x30, 0x3000, 0x0000);
    }

    _reloadConfig();
}

void AT1846S_setOpMode(const AT1846S_op_t mode)
{
    if(mode == AT1846S_OP_DMR)
    {
        /* TODO: DMR mode */

    }
    else
    {
        /* FM mode */
        i2c_writeReg16(0x58, 0x9C1D);
        i2c_writeReg16(0x40, 0x0030);
    }

    _reloadConfig();
}

void AT1846S_setFuncMode(const AT1846S_func_t mode)
{
    /*
     * Functional mode is controlled by bits 5 (RX on) and 6 (TX on) in register
     * 0x30. With a cast and shift we can do it easily...
     *
     * Note about sanity check: if value is greater than 0x0040 we are trying to
     * turn on both RX and TX, which is NOT good.
     */

    uint16_t value = ((uint16_t) mode) << 5;
    if(value > 0x0040) return;
    _maskSetRegister(0x30, 0x0060, value);
//     i2c_writeReg16(0x44, 0x4ff);
}

void AT1846S_enableTxCtcss(tone_t freq)
{
    i2c_writeReg16(0x4A, freq*10);
    i2c_writeReg16(0x4B, 0x0000);
    i2c_writeReg16(0x4C, 0x0000);
    _maskSetRegister(0x4E, 0x0600, 0x0600);
}

void AT1846S_disableCtcss()
{
    i2c_writeReg16(0x4A, 0x0000);
    _maskSetRegister(0x4E, 0x0600, 0x0000); /* Disable TX CTCSS */
}

uint16_t AT1846S_readRSSI()
{
    return i2c_readReg16(0x1B);
}

void AT1846S_setPgaGain(const uint8_t gain)
{
    uint16_t pga = (gain & 0x1F) << 6;
    _maskSetRegister(0x0A, 0x07C0, pga);
}

void AT1846S_setMicGain(const uint8_t gain)
{
    _maskSetRegister(0x41, 0x007F, ((uint16_t) gain));
}

void AT1846S_setAgcGain(const uint8_t gain)
{
    uint16_t agc = (gain & 0x0F) << 8;
    _maskSetRegister(0x44, 0x0F00, agc);
}

void AT1846S_setTxDeviation(const uint16_t dev)
{
    uint16_t value = (dev & 0x03FF) << 6;
    _maskSetRegister(0x59, 0xFFC0, value);
}

void AT1846S_setRxAudioGain(const uint8_t gainWb, const uint8_t gainNb)
{
    uint16_t value = (gainWb & 0x0F) << 8;
    _maskSetRegister(0x44, 0x0F00, value);
    _maskSetRegister(0x44, 0x000F, ((uint16_t) gainNb));
}

void AT1846S_setNoise1Thresholds(const uint8_t highTsh, const uint8_t lowTsh)
{
    uint16_t value = ((highTsh & 0x1f) << 8) | (lowTsh & 0x1F);
    i2c_writeReg16(0x48, value);
}

void AT1846S_setNoise2Thresholds(const uint8_t highTsh, const uint8_t lowTsh)
{
    uint16_t value = ((highTsh & 0x1f) << 8) | (lowTsh & 0x1F);
    i2c_writeReg16(0x60, value);
}

void AT1846S_setRssiThresholds(const uint8_t highTsh, const uint8_t lowTsh)
{
    uint16_t value = ((highTsh & 0x1f) << 8) | (lowTsh & 0x1F);
    i2c_writeReg16(0x3F, value);
}

void AT1846S_setPaDrive(const uint8_t value)
{
    uint16_t pa = value << 11;
    _maskSetRegister(0x0A, 0x7800, pa);
}

void AT1846S_setAnalogSqlThresh(const uint8_t thresh)
{
    i2c_writeReg16(0x49, ((uint16_t) thresh));
}
