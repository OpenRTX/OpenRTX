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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include "HR_C6000.h"
#include "interfaces.h"
#include <hwconfig.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <calibUtils.h>

#include <stdio.h>

static const uint8_t initSeq1[] = { 0x01, 0x04, 0xD5, 0xD7, 0xF7, 0x7F, 0xD7, 0x57 };
static const uint8_t initSeq2[] =
{
    0x04, 0x11, 0x80, 0x0C, 0x22, 0x01, 0x00, 0x00, 0x33, 0xEF, 0x00, 0xFF, 0xFF,
    0xFF, 0xF0, 0xF0, 0x10, 0x00, 0x00, 0x06, 0x3B, 0xF8, 0x0E, 0xFD, 0x40, 0xFF,
    0x00, 0x0B, 0x00, 0x00, 0x00, 0x06, 0x0B, 0x00, 0x17, 0x02, 0xFF, 0xE0, 0x14,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t initSeq3[] =
{
    0x01, 0x10, 0x69, 0x69, 0x96, 0x96, 0x96, 0x99, 0x99, 0x99, 0xA5, 0xA5, 0xAA,
    0xAA, 0xCC, 0xCC, 0x00, 0xF0, 0x01, 0xFF, 0x01, 0x0F, 0x00, 0x00, 0x00, 0x00,
    0x0D, 0x70, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
static const uint8_t initSeq4[] =
{
    0x01, 0x30, 0x00, 0x00, 0x20, 0x3C, 0xFF, 0xFF, 0x3F, 0x50, 0x07, 0x60, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00

};
static const uint8_t initSeq5[] = { 0x01, 0x40, 0x00, 0x01, 0x01, 0x02, 0x01, 0x1E, 0xF0 };
static const uint8_t initSeq6[] = { 0x01, 0x50, 0x00, 0x08, 0xEB, 0x78, 0x67 };
static const uint8_t initSeq7[] = { 0x01, 0x04, 0xD5, 0xD7, 0xF7, 0x7F, 0xD7, 0x57 };

void _writeReg(uint8_t type, uint8_t reg, uint8_t val)
{
    gpio_clearPin(DMR_CS);
    (void) uSpi_sendRecv(type);
    (void) uSpi_sendRecv(reg);
    (void) uSpi_sendRecv(val);
    delayUs(2);
    gpio_setPin(DMR_CS);
    delayUs(2);
}

uint8_t _readReg(uint8_t type, uint8_t reg)
{
    gpio_clearPin(DMR_CS);
    (void) uSpi_sendRecv(type);
    (void) uSpi_sendRecv(reg);
    uint8_t val = uSpi_sendRecv(0xFF);
    delayUs(2);
    gpio_setPin(DMR_CS);
    delayUs(2);

    return val;
}

void _sendSequence(const uint8_t *seq, uint8_t len)
{
    gpio_clearPin(DMR_CS);

    uint8_t i = 0;
    for(; i < len; i++)
    {
        (void) uSpi_sendRecv(seq[i]);
    }

   delayUs(2);
   gpio_setPin(DMR_CS);
   delayUs(2);
}

void C6000_init()
{

    uSpi_init();

    gpio_setMode(DMR_CS,    OUTPUT);
    gpio_setMode(DMR_SLEEP, OUTPUT);

    gpio_setPin(DMR_SLEEP);

    delayMs(10);
    gpio_clearPin(DMR_SLEEP);         // Exit from sleep pulling down DMR_SLEEP
    delayMs(10);

    _writeReg(0x04, 0x0A, 0x80);    //Clock connected to crystal
    _writeReg(0x04, 0x0B, 0x28);    //Set PLL M Register
    _writeReg(0x04, 0x0C, 0x33);    //Set PLL Dividers
    delayMs(250);

    _writeReg(0x04, 0x0A, 0x00);
    _writeReg(0x04, 0xB9, 0x05);
    _writeReg(0x04, 0xBA, 0x04);
    _writeReg(0x04, 0xBB, 0x02);
    _writeReg(0x04, 0xA1, 0x80);
    _writeReg(0x04, 0x10, 0xF3);
    _writeReg(0x04, 0x40, 0x43);
    _writeReg(0x04, 0x07, 0x0B);
    _writeReg(0x04, 0x08, 0xB8);
    _writeReg(0x04, 0x09, 0x00);
    _writeReg(0x04, 0x06, 0x21);
    _sendSequence(initSeq1, sizeof(initSeq1));
    _writeReg(0x04, 0x01, 0xF8);
    _sendSequence(initSeq2, sizeof(initSeq2));
    _writeReg(0x04, 0x00, 0x2A);
    _writeReg(0x04, 0x06, 0x22);

    gpio_clearPin(DMR_CS);
    (void) uSpi_sendRecv(0x03);
    (void) uSpi_sendRecv(0x00);
    for(uint8_t i = 0; i < 128; i++) uSpi_sendRecv(0xAA);
    delayUs(2);
    gpio_setPin(DMR_CS);
    delayUs(2);

    _writeReg(0x04, 0x06, 0x20);
    _writeReg(0x04, 0x14, 0x59);
    _writeReg(0x04, 0x15, 0xF5);
    _writeReg(0x04, 0x16, 0x21);
    _sendSequence(initSeq3, sizeof(initSeq3));
    _sendSequence(initSeq4, sizeof(initSeq4));
    _sendSequence(initSeq5, sizeof(initSeq5));
    _sendSequence(initSeq6, sizeof(initSeq6));
    _writeReg(0x01, 0x52, 0x08);
    _writeReg(0x01, 0x53, 0xEB);
    _writeReg(0x01, 0x54, 0x78);
    _writeReg(0x01, 0x45, 0x1E);
    _writeReg(0x01, 0x37, 0x50);
    _writeReg(0x01, 0x35, 0xFF);
    _writeReg(0x04, 0x39, 0x02);
    _writeReg(0x04, 0x3D, 0x0A);
    _writeReg(0x04, 0x83, 0xFF);
    _writeReg(0x04, 0x87, 0x00);
    _writeReg(0x04, 0x65, 0x0A);
    _writeReg(0x04, 0x1D, 0xFF);
    _writeReg(0x04, 0x1E, 0xF1);
    _writeReg(0x04, 0xE2, 0x06);
    _writeReg(0x04, 0xE4, 0x27);
    _writeReg(0x04, 0xE3, 0x52);
    _writeReg(0x04, 0xE5, 0x1A);
    _writeReg(0x04, 0xE1, 0x0F);
    _writeReg(0x04, 0xD1, 0xC4);
    _writeReg(0x04, 0x25, 0x0E);
    _writeReg(0x04, 0x26, 0xFD);
    _writeReg(0x04, 0x64, 0x00);
}

void C6000_terminate()
{
    gpio_setPin(DMR_SLEEP);
    gpio_setMode(DMR_CS, INPUT);
}

void C6000_setModOffset(uint16_t offset)
{
    /*
     * Same as original TYT firmware.
     * Reference: functions @0802e7d4 and @080546cc in S18.16 binary image
     */
    uint8_t offUpper = (offset < 0x80) ? 0x03 : 0x00;
    uint8_t offLower = offset + 0x80;

    _writeReg(0x04, 0x48, offUpper);    // Two-point bias, upper value
    _writeReg(0x04, 0x47, offLower);    // Two-point bias, lower value

    _writeReg(0x04, 0x04, offLower);    // Bias value for TX, Q-channel
}

void C6000_setModAmplitude(uint8_t iAmp, uint8_t qAmp)
{
    _writeReg(0x04, 0x45, iAmp);    // Mod2 magnitude (HR_C6000)
    _writeReg(0x04, 0x46, qAmp);    // Mod1 magnitude (HR_C6000)
}

void C6000_setMod2Bias(uint8_t bias)
{
    _writeReg(0x04, 0x04, bias);
}

void C6000_setDacGain(uint8_t value)
{
    if(value < 1)  value = 1;
    if(value > 31) value = 31;
    _writeReg(0x04, 0x37, (0x80 | value));
}

void C6000_dmrMode()
{
    _writeReg(0x04, 0x10, 0x4F);
    _writeReg(0x04, 0x81, 0x19);
    _writeReg(0x04, 0x01, 0xF0);
    _writeReg(0x04, 0xE4, 0x27);
    _writeReg(0x04, 0xE5, 0x1A);
    _writeReg(0x04, 0x25, 0x0E);
    _writeReg(0x04, 0x26, 0xFD);
    _writeReg(0x01, 0x54, 0x78);
//     _writeReg(0x04, 0x48, 0x00);
//     _writeReg(0x04, 0x47, 0x25);
    _writeReg(0x04, 0x1F, 0x10);
    _writeReg(0x01, 0x24, 0x00);
    _writeReg(0x01, 0x25, 0x00);
    _writeReg(0x01, 0x26, 0x00);
    _writeReg(0x01, 0x27, 0x00);
    _writeReg(0x04, 0x41, 0x40);
    _writeReg(0x04, 0x56, 0x00);
    _writeReg(0x04, 0x41, 0x40);
    _writeReg(0x04, 0x5C, 0x09);
    _writeReg(0x04, 0x5F, 0xC0);
    _sendSequence(initSeq7, sizeof(initSeq7));
    _writeReg(0x04, 0x11, 0x80);
}

void C6000_fmMode()
{
    _writeReg(0x04, 0x10, 0xF3);
    _writeReg(0x04, 0x01, 0xB0);
    _writeReg(0x04, 0x81, 0x04);
    _writeReg(0x04, 0xE5, 0x1A);
    _writeReg(0x04, 0x36, 0x02);
    _writeReg(0x04, 0xE4, 0x27);
    _writeReg(0x04, 0xE2, 0x06);
    _writeReg(0x04, 0x34, 0x98);
    _writeReg(0x04, 0x60, 0x00);
    _writeReg(0x04, 0x1F, 0x00);
    _writeReg(0x01, 0x24, 0x00);
    _writeReg(0x01, 0x25, 0x00);
    _writeReg(0x01, 0x26, 0x00);
    _writeReg(0x01, 0x27, 0x00);
    _writeReg(0x04, 0x56, 0x00);
    _writeReg(0x04, 0x41, 0x40);
    _writeReg(0x04, 0x5C, 0x09);
    _writeReg(0x04, 0x5F, 0xC0);
    _sendSequence(initSeq7, sizeof(initSeq7));
    _writeReg(0x04, 0x11, 0x80);
    _writeReg(0x04, 0xE0, 0xC9);

    _writeReg(0x04, 0x37, 0x81);
}

void C6000_startAnalogTx()
{
    _writeReg(0x04, 0xE2, 0x00);
    _writeReg(0x04, 0xE4, 0x23);
    _writeReg(0x04, 0xC2, 0x00);
    _writeReg(0x04, 0xA1, 0x80);
//     _writeReg(0x04, 0x25, 0x0E);
//     _writeReg(0x04, 0x26, 0xFE);
    _writeReg(0x04, 0x83, 0xFF);
    _writeReg(0x04, 0x87, 0x00);
    _writeReg(0x04, 0x04, 0x24);
    _writeReg(0x04, 0x35, 0x40);
    _writeReg(0x04, 0x3F, 0x04);
    _writeReg(0x04, 0x34, 0x1C);    // Disable pre-emphasis, 25kHz bandwidth
    _writeReg(0x04, 0x3E, 0x08);
    _writeReg(0x01, 0x50, 0x00);
    _writeReg(0x01, 0x51, 0x00);
    _writeReg(0x04, 0x60, 0x80);
    _writeReg(0x04, 0x10, 0xF3);

    _writeReg(0x04, 0xE0, 0x83);    // Enable mic_p input
    _writeReg(0x04, 0x37, 0x8C);
}

void C6000_stopAnalogTx()
{
    _writeReg(0x04, 0x60, 0x00);
    _writeReg(0x04, 0xE0, 0xC9);
    _writeReg(0x04, 0xE2, 0x06);
    _writeReg(0x04, 0x34, 0x98);
    _writeReg(0x04, 0x37, 0x81);
}

bool C6000_spiInUse()
{
    return (gpio_readPin(DMR_CS) == 0) ? true :  false;
}
