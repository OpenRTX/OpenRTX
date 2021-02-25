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
#include <calibInfo_GDx.h>
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
    uint16_t funcMode = i2c_readReg16(0x30) & 0x0060;   /* Get current op. status */
    _maskSetRegister(0x30, 0x0060, 0x0000);             /* RX and TX off          */
    _maskSetRegister(0x30, 0x0060, funcMode);           /* Restore op. status     */
}


void AT1846S_init()
{
    i2c_writeReg16(0x30, 0x0001);   /* Soft reset              */
    delayMs(50);

    i2c_writeReg16(0x30, 0x0004);   /* Chip enable             */
    i2c_writeReg16(0x04, 0x0FD0);   /* 26MHz crystal frequency */
    i2c_writeReg16(0x1F, 0x1000);   /* Gpio6 squelch output    */
    i2c_writeReg16(0x09, 0x03AC);
    i2c_writeReg16(0x24, 0x0001);
    i2c_writeReg16(0x31, 0x0031);
    i2c_writeReg16(0x33, 0x45F5);   /* AGC number              */
    i2c_writeReg16(0x34, 0x2B89);   /* RX digital gain         */
    i2c_writeReg16(0x3F, 0x3263);   /* RSSI 3 threshold        */
    i2c_writeReg16(0x41, 0x470F);   /* Tx digital gain         */
    i2c_writeReg16(0x42, 0x1036);
    i2c_writeReg16(0x43, 0x00BB);
    i2c_writeReg16(0x44, 0x06FF);   /* Tx digital gain         */
    i2c_writeReg16(0x47, 0x7F2F);   /* Soft mute               */
    i2c_writeReg16(0x4E, 0x0082);
    i2c_writeReg16(0x4F, 0x2C62);
    i2c_writeReg16(0x53, 0x0094);
    i2c_writeReg16(0x54, 0x2A3C);
    i2c_writeReg16(0x55, 0x0081);
    i2c_writeReg16(0x56, 0x0B02);
    i2c_writeReg16(0x57, 0x1C00);   /* Bypass RSSI low-pass    */
    i2c_writeReg16(0x5A, 0x4935);   /* SQ detection time       */
    i2c_writeReg16(0x58, 0xBCCD);
    i2c_writeReg16(0x62, 0x3263);   /* Modulation detect tresh */
    i2c_writeReg16(0x4E, 0x2082);
    i2c_writeReg16(0x63, 0x16AD);
    i2c_writeReg16(0x30, 0x40A4);
    delayMs(50);

    i2c_writeReg16(0x30, 0x40A6);   /* Start calibration       */
    delayMs(100);
    i2c_writeReg16(0x30, 0x4006);   /* Stop calibration        */

    delayMs(100);

    i2c_writeReg16(0x58, 0xBCED);
    i2c_writeReg16(0x0A, 0x7BA0);   /* PGA gain                */
    i2c_writeReg16(0x41, 0x4731);   /* Tx digital gain         */
    i2c_writeReg16(0x44, 0x05FF);   /* Tx digital gain         */
    i2c_writeReg16(0x59, 0x09D2);   /* Mixer gain              */
    i2c_writeReg16(0x44, 0x05CF);   /* Tx digital gain         */
    i2c_writeReg16(0x44, 0x05CC);   /* Tx digital gain         */
    i2c_writeReg16(0x48, 0x1A32);   /* Noise 1 threshold       */
    i2c_writeReg16(0x60, 0x1A32);   /* Noise 2 threshold       */
    i2c_writeReg16(0x3F, 0x29D1);   /* RSSI 3 threshold        */
    i2c_writeReg16(0x0A, 0x7BA0);   /* PGA gain                */
    i2c_writeReg16(0x49, 0x0C96);   /* RSSI SQL thresholds     */
    i2c_writeReg16(0x33, 0x45F5);   /* AGC number              */
    i2c_writeReg16(0x41, 0x470F);   /* Tx digital gain         */
    i2c_writeReg16(0x42, 0x1036);
    i2c_writeReg16(0x43, 0x00BB);
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

    i2c_writeReg16(0x29, fHi);
    i2c_writeReg16(0x2A, fLo);

    _reloadConfig();
}

void AT1846S_setBandwidth(const AT1846S_bw_t band)
{
    if(band == AT1846S_BW_25)
    {
        /* 25kHz bandwidth */
        i2c_writeReg16(0x15, 0x1F00);   /* Tuning bit              */
        i2c_writeReg16(0x32, 0x7564);   /* AGC target power        */
        i2c_writeReg16(0x3A, 0x44C3);   /* Modulation detect sel   */
        i2c_writeReg16(0x3F, 0x29D2);   /* RSSI 3 threshold        */
        i2c_writeReg16(0x3C, 0x0E1C);   /* Peak detect threshold   */
        i2c_writeReg16(0x48, 0x1E38);   /* Noise 1 threshold       */
        i2c_writeReg16(0x62, 0x3767);   /* Modulation detect tresh */
        i2c_writeReg16(0x65, 0x248A);
        i2c_writeReg16(0x66, 0xFF2E);   /* RSSI comp and AFC range */
        i2c_writeReg16(0x7F, 0x0001);   /* Switch to page 1        */
        i2c_writeReg16(0x06, 0x0024);   /* AGC gain table          */
        i2c_writeReg16(0x07, 0x0214);
        i2c_writeReg16(0x08, 0x0224);
        i2c_writeReg16(0x09, 0x0314);
        i2c_writeReg16(0x0A, 0x0324);
        i2c_writeReg16(0x0B, 0x0344);
        i2c_writeReg16(0x0D, 0x1384);
        i2c_writeReg16(0x0E, 0x1B84);
        i2c_writeReg16(0x0F, 0x3F84);
        i2c_writeReg16(0x12, 0xE0EB);
        i2c_writeReg16(0x7F, 0x0000);   /* Back to page 0          */
        _maskSetRegister(0x30, 0x3000, 0x3000);
    }
    else
    {
        /* 12.5kHz bandwidth */
        i2c_writeReg16(0x15, 0x1100);   /* Tuning bit              */
        i2c_writeReg16(0x32, 0x4495);   /* AGC target power        */
        i2c_writeReg16(0x3A, 0x40C3);   /* Modulation detect sel   */
        i2c_writeReg16(0x3F, 0x28D0);   /* RSSI 3 threshold        */
        i2c_writeReg16(0x3C, 0x0F1E);   /* Peak detect threshold   */
        i2c_writeReg16(0x48, 0x1DB6);   /* Noise 1 threshold       */
        i2c_writeReg16(0x62, 0x1425);   /* Modulation detect tresh */
        i2c_writeReg16(0x65, 0x2494);
        i2c_writeReg16(0x66, 0xEB2E);   /* RSSI comp and AFC range */
        i2c_writeReg16(0x7F, 0x0001);   /* Switch to page 1        */
        i2c_writeReg16(0x06, 0x0014);   /* AGC gain table          */
        i2c_writeReg16(0x07, 0x020C);
        i2c_writeReg16(0x08, 0x0214);
        i2c_writeReg16(0x09, 0x030C);
        i2c_writeReg16(0x0A, 0x0314);
        i2c_writeReg16(0x0B, 0x0324);
        i2c_writeReg16(0x0C, 0x0344);
        i2c_writeReg16(0x0D, 0x1344);
        i2c_writeReg16(0x0E, 0x1B44);
        i2c_writeReg16(0x0F, 0x3F44);
        i2c_writeReg16(0x12, 0xE0EB);   /* Back to page 0          */
        i2c_writeReg16(0x7F, 0x0000);
        _maskSetRegister(0x30, 0x3000, 0x0000);
    }

    _reloadConfig();
}

void AT1846S_setOpMode(const AT1846S_op_t mode)
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

        _maskSetRegister(0x57, 0x0001, 0x00);     /* Audio feedback off   */
        _maskSetRegister(0x3A, 0x7000, 0x4000);   /* Select voice channel */
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
