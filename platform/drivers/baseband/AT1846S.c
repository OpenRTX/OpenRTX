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

#include "AT1846S.h"
#include "interfaces.h"
#include <calibInfo_GDx.h>
#include <interfaces/delays.h>
#include <stdio.h>

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
    uint16_t funcMode = i2c_readReg16(0x30) & 0x0060;   /* Get current op. status */
    _maskSetRegister(0x30, 0x0060, 0x0000);             /* RX and TX off          */
    _maskSetRegister(0x30, 0x0060, funcMode);           /* Restore op. status     */
}


void AT1846S_init()
{
    i2c_writeReg16(0x30, 0x0001);    // Soft reset
    delayMs(50);

    i2c_writeReg16(0x30, 0x0004);    // Init settings
    i2c_writeReg16(0x04, 0x0FD0);
    i2c_writeReg16(0x1F, 0x1000);
    i2c_writeReg16(0x09, 0x03AC);
    i2c_writeReg16(0x24, 0x0001);
    i2c_writeReg16(0x31, 0x0031);
    i2c_writeReg16(0x33, 0x45F5);
    i2c_writeReg16(0x34, 0x2B89);
    i2c_writeReg16(0x3F, 0x3263);
    i2c_writeReg16(0x41, 0x470F);
    i2c_writeReg16(0x42, 0x1036);
    i2c_writeReg16(0x43, 0x00BB);
    i2c_writeReg16(0x44, 0x06FF);
    i2c_writeReg16(0x47, 0x7F2F);
    i2c_writeReg16(0x4E, 0x0082);
    i2c_writeReg16(0x4F, 0x2C62);
    i2c_writeReg16(0x53, 0x0094);
    i2c_writeReg16(0x54, 0x2A3C);
    i2c_writeReg16(0x55, 0x0081);
    i2c_writeReg16(0x56, 0x0B02);
    i2c_writeReg16(0x57, 0x1C00);
    i2c_writeReg16(0x5A, 0x4935);
    i2c_writeReg16(0x58, 0xBCCD);
    i2c_writeReg16(0x62, 0x3263);
    i2c_writeReg16(0x4E, 0x2082);
    i2c_writeReg16(0x63, 0x16AD);
    i2c_writeReg16(0x30, 0x40A4);
    delayMs(50);

    i2c_writeReg16(0x30, 0x40A6);    // cal start
    delayMs(100);
    i2c_writeReg16(0x30, 0x4006);    // cal end

    delayMs(100);

    i2c_writeReg16(0x58, 0xBCED);
    i2c_writeReg16(0x0A, 0x7BA0);
    i2c_writeReg16(0x41, 0x4731);
    i2c_writeReg16(0x44, 0x05FF);
    i2c_writeReg16(0x59, 0x09D2);
    i2c_writeReg16(0x44, 0x05CF);
    i2c_writeReg16(0x44, 0x05CC);
    i2c_writeReg16(0x48, 0x1A32);
    i2c_writeReg16(0x60, 0x1A32);
    i2c_writeReg16(0x3F, 0x29D1);
    i2c_writeReg16(0x0A, 0x7BA0);
    i2c_writeReg16(0x49, 0x0C96);
    i2c_writeReg16(0x33, 0x45F5);
    i2c_writeReg16(0x41, 0x470F);
    i2c_writeReg16(0x42, 0x1036);
    i2c_writeReg16(0x43, 0x00BB);
}

void AT1846S_setFrequency(const freq_t freq)
{
    /* The value to be written in registers is given by: 0.0016*freqency */
    uint32_t val = (freq/1000)*16;
    uint16_t fHi = (val >> 16) & 0xFFFF;
    uint16_t fLo = val & 0xFFFF;

    i2c_writeReg16(0x29, fHi);
    i2c_writeReg16(0x2A, fLo);

    _reloadConfig();
}

void AT1846S_setBandwidth(AT1846S_bw_t band)
{
    if(band == AT1846S_BW_25)
    {
        /* 25kHz bandwidth */
        i2c_writeReg16(0x15, 0x1F00);
        i2c_writeReg16(0x32, 0x7564);
        i2c_writeReg16(0x3A, 0x44C3);
        i2c_writeReg16(0x3F, 0x29D2);
        i2c_writeReg16(0x3C, 0x0E1C);
        i2c_writeReg16(0x48, 0x1E38);
        i2c_writeReg16(0x62, 0x3767);
        i2c_writeReg16(0x65, 0x248A);
        i2c_writeReg16(0x66, 0xFF2E);
        i2c_writeReg16(0x7F, 0x0001);
        i2c_writeReg16(0x06, 0x0024);
        i2c_writeReg16(0x07, 0x0214);
        i2c_writeReg16(0x08, 0x0224);
        i2c_writeReg16(0x09, 0x0314);
        i2c_writeReg16(0x0A, 0x0324);
        i2c_writeReg16(0x0B, 0x0344);
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
        i2c_writeReg16(0x3A, 0x40C3);
        i2c_writeReg16(0x3F, 0x28D0);
        i2c_writeReg16(0x3C, 0x0F1E);
        i2c_writeReg16(0x48, 0x1DB6);
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

void AT1846S_setOpMode(AT1846S_op_t mode)
{
    if(mode == AT1846S_OP_DMR)
    {
        /* DMR mode */
        i2c_writeReg16(0x3A, 0x00C2);
        i2c_writeReg16(0x33, 0x45F5);
        i2c_writeReg16(0x41, 0x4731);
        i2c_writeReg16(0x42, 0x1036);
        i2c_writeReg16(0x43, 0x00BB);
        i2c_writeReg16(0x58, 0xBCFD);
        i2c_writeReg16(0x44, 0x06CC);
        i2c_writeReg16(0x40, 0x0031);
    }
    else
    {
        /* FM mode */
        i2c_writeReg16(0x33, 0x44A5);
        i2c_writeReg16(0x41, 0x4431);
        i2c_writeReg16(0x42, 0x10F0);
        i2c_writeReg16(0x43, 0x00A9);
        i2c_writeReg16(0x58, 0xBC05);
        i2c_writeReg16(0x44, 0x06FF);
        i2c_writeReg16(0x40, 0x0030);
    }

    _reloadConfig();
}

void AT1846S_setFuncMode(AT1846S_func_t mode)
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
}

uint16_t AT1846S_readRSSI()
{
    return i2c_readReg16(0x1B);
}

void AT1846S_setPgaGain(uint8_t gain)
{
    uint16_t pga = (gain & 0x1F) << 6;
    _maskSetRegister(0x0A, 0x07C0, pga);
}

void AT1846S_setMicGain(uint8_t gain)
{
    _maskSetRegister(0x41, 0x007F, ((uint16_t) gain));
}

void AT1846S_setAgcGain(uint8_t gain)
{
    uint16_t agc = (gain & 0x0F) << 8;
    _maskSetRegister(0x44, 0x0F00, agc);
}

void AT1846S_setTxDeviation(uint8_t dev)
{
    _maskSetRegister(0x59, 0x003F, ((uint16_t) dev));
}

void AT1846S_setRxAudioGain(uint8_t gainWb, uint8_t gainNb)
{
    uint16_t value = (gainWb & 0x0F) << 8;
    _maskSetRegister(0x44, 0x0F00, value);
    _maskSetRegister(0x44, 0x000F, ((uint16_t) gainNb));
}

void AT1846S_setPaDrive(uint8_t value)
{
    uint16_t pa = value << 11;
    _maskSetRegister(0x0A, 0x7800, pa);
}

void AT1846S_setAnalogSqlThresh(uint8_t thresh)
{
    i2c_writeReg16(0x49, ((uint16_t) thresh));
}
