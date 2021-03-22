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

#include <hwconfig.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include "HR_C5000.h"

const uint8_t initSeq1[] = {0x00, 0x00, 0xFF, 0xB0, 0x00, 0x00, 0x00, 0x00};
const uint8_t initSeq2[] =
{
    0x00, 0x11, 0x80, 0x0A, 0x22, 0x01, 0x00, 0x00, 0x33, 0xEF, 0x00, 0xFF, 0xFF,
    0xFF, 0xF0, 0xF0, 0x10, 0x00, 0x00, 0x07, 0x3B, 0xF8, 0x0E, 0xFD, 0x40, 0xFF,
    0x00, 0x0B, 0x00, 0x00, 0x00, 0x04, 0x0B, 0x00, 0x17, 0x02, 0xFF, 0xE0, 0x28,
    0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
const uint8_t initSeq3[] =
{
    0x01, 0x10, 0x69, 0x69, 0x96, 0x96, 0x96, 0x99, 0x99, 0x99, 0xA5, 0xA5, 0xAA,
    0xAA, 0xCC, 0xCC, 0x00, 0xF0, 0x01, 0xFF, 0x01, 0x0F
};
const uint8_t initSeq4[] = {0x01, 0x30, 0x30, 0x4E, 0x14, 0x1E, 0x1A, 0x30, 0x3D,
                            0x50, 0x07, 0x60};
const uint8_t initSeq5[] = {0x01, 0x40, 0x90, 0x03, 0x01, 0x02, 0x05, 0x07, 0xF0};
const uint8_t initSeq6[] = {0x01, 0x50, 0x00, 0x00, 0x00, 0x00, 0x00};

uint8_t _spiSendRecv(uint8_t value)
{
    gpio_clearPin(DMR_CLK);

    uint8_t incoming = 0;
    uint8_t cnt = 0;

    for(; cnt < 8; cnt++)
    {
        gpio_setPin(DMR_CLK);

        if(value & (0x80 >> cnt))
        {
            gpio_setPin(DMR_MOSI);
        }
        else
        {
            gpio_clearPin(DMR_MOSI);
        }

        delayUs(1);
        gpio_clearPin(DMR_CLK);
        incoming = (incoming << 1) | gpio_readPin(DMR_MISO);
        delayUs(1);
    }

    return incoming;
}

void _writeReg(uint8_t type, uint8_t reg, uint8_t val)
{
    gpio_clearPin(DMR_CS);
    (void) _spiSendRecv(type);
    (void) _spiSendRecv(reg);
    (void) _spiSendRecv(val);
    gpio_setPin(DMR_CS);
}

void _sendSequence(const uint8_t *seq, uint8_t len)
{
    gpio_clearPin(DMR_CS);

    uint8_t i = 0;
    for(; i < len; i++)
    {
        (void) _spiSendRecv(seq[i]);
    }

    gpio_setPin(DMR_CS);
}

void C5000_init()
{
    gpio_setMode(DMR_CS,    OUTPUT);
    gpio_setMode(DMR_CLK,   OUTPUT);
    gpio_setMode(DMR_MOSI,  OUTPUT);
    gpio_setMode(DMR_MISO,  OUTPUT);
    gpio_setMode(DMR_SLEEP, OUTPUT);

    gpio_setPin(DMR_CS);
    gpio_clearPin(DMR_SLEEP);         // Exit from sleep pulling down DMR_SLEEP

    _writeReg(0x00, 0x0A, 0x80);      // Internal clock connected to crystal
    _writeReg(0x00, 0x0B, 0x28);      // PLL M register (multiplier)
    _writeReg(0x00, 0x0C, 0x33);      // PLL input and output dividers

    delayMs(1);
    _writeReg(0x00, 0x0A, 0x00);      // Internal clock connected to PLL
    _writeReg(0x00, 0xBA, 0x22);      // Built-in codec clock freq. (HR_C6000)
    _writeReg(0x00, 0xBB, 0x11);      // Output clock operating freq. (HR_C6000)
}

void C5000_terminate()
{
    gpio_setPin(DMR_SLEEP);
    gpio_setMode(DMR_CS,    INPUT);
    gpio_setMode(DMR_CLK,   INPUT);
    gpio_setMode(DMR_MOSI,  INPUT);
    gpio_setMode(DMR_MISO,  INPUT);
    gpio_setMode(DMR_SLEEP, INPUT);
}

void C5000_setModOffset(uint8_t offset)
{
    /*
     * Original TYT MD-380 code does this, both for DMR and FM.
     *
     * References: functions @0x0803fda8 and @0x0804005c
     */
    uint8_t offUpper = (offset < 0x80) ? 0x00 : 0x03;
    uint8_t offLower = 0x7F - offset;

    _writeReg(0x00, 0x48, offUpper);    // Two-point bias, upper value
    _writeReg(0x00, 0x47, offLower);    // Two-point bias, lower value

    _writeReg(0x00, 0x04, offLower);    // Bias value for TX, Q-channel
}

void C5000_setModAmplitude(uint8_t iAmp, uint8_t qAmp)
{
    _writeReg(0x00, 0x45, iAmp);    // Mod2 magnitude (HR_C6000)
    _writeReg(0x00, 0x46, qAmp);    // Mod1 magnitude (HR_C6000)

}

void C5000_setModFactor(uint8_t mf)
{
    _writeReg(0x00, 0x35, mf);      // FM modulation factor
    _writeReg(0x00, 0x3F, 0x04);    // FM Limiting modulation factor (HR_C6000)

}

void C5000_dmrMode()
{
//     _writeReg(0x00, 0x0A, 0x80);
//     _writeReg(0x00, 0x0B, 0x28);
//     _writeReg(0x00, 0x0C, 0x33);
//     OSTimeDly(1, OS_OPT_TIME_DLY, &e);
//     _writeReg(0x00, 0x0A, 0x00);
    _writeReg(0x00, 0xB9, 0x32);
//     _writeReg(0x00, 0xBA, 0x22);
//     _writeReg(0x00, 0xBB, 0x11);
    _writeReg(0x00, 0x10, 0x4F);
    _writeReg(0x00, 0x40, 0x43);
    _writeReg(0x00, 0x41, 0x40);
    _writeReg(0x00, 0x07, 0x0B);
    _writeReg(0x00, 0x08, 0xB8);
    _writeReg(0x00, 0x09, 0x00);
    _writeReg(0x00, 0x06, 0x21);
    _sendSequence(initSeq1, sizeof(initSeq1));
//     _writeReg(0x00, 0x48, 0x00);
//     _writeReg(0x00, 0x47, 0x1F);    // This is 0x7F - freq_adj_mid */
    _sendSequence(initSeq2, sizeof(initSeq2));
    _writeReg(0x00, 0x00, 0x28);

    delayMs(1);

    _writeReg(0x00, 0x14, 0x59);
    _writeReg(0x00, 0x15, 0xF5);
    _writeReg(0x00, 0x16, 0x21);
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
    _writeReg(0x00, 0x39, 0x02);
    _writeReg(0x00, 0x3D, 0x0A);
    _writeReg(0x00, 0x83, 0xFF);
    _writeReg(0x00, 0x87, 0x00);
    _writeReg(0x00, 0x65, 0x0A);
    _writeReg(0x00, 0x1D, 0xFF);
    _writeReg(0x00, 0x1E, 0xF1);
    _writeReg(0x00, 0x1F, 0x10);
    _writeReg(0x00, 0x0D, 0x8C);
    _writeReg(0x00, 0x0E, 0x44);
    _writeReg(0x00, 0x0F, 0xC8);
    _writeReg(0x00, 0x37, 0xC2);
    _writeReg(0x00, 0x25, 0x0E);
    _writeReg(0x00, 0x26, 0xFD);
    _writeReg(0x00, 0x64, 0x00);

    _writeReg(0x01, 0x24, 0x00);
    _writeReg(0x01, 0x25, 0x00);
    _writeReg(0x01, 0x26, 0x00);
    _writeReg(0x01, 0x27, 0x00);
    _writeReg(0x00, 0x81, 0x19);
    _writeReg(0x00, 0x85, 0x00);
}

void C5000_fmMode()
{
    _writeReg(0x00, 0xB9, 0x33);    // System clock frequency (HR_C6000)
    _writeReg(0x00, 0x10, 0x80);    // FM modulator mode
    _writeReg(0x00, 0x07, 0x0E);    // IF frequency - upper 8 bits
    _writeReg(0x00, 0x08, 0x10);    // IF frequency - middle 8 bits
    _writeReg(0x00, 0x09, 0x00);    // IF frequency - lower 8 bits
    _sendSequence(initSeq1, sizeof(initSeq1));
    _writeReg(0x00, 0x06, 0x00);    // VoCoder control
    _sendSequence(initSeq2, sizeof(initSeq2));
    _writeReg(0x00, 0x0D, 0x8C);    // Codec control
    _writeReg(0x00, 0x0E, 0x44);    // Mute HPout and enable MIC 1
    _writeReg(0x00, 0x0F, 0xC8);    // ADLinVol, mic volume
//     _writeReg(0x00, 0x25, 0x0E);
//     _writeReg(0x00, 0x26, 0xFE);
    _writeReg(0x00, 0x83, 0xFF);    // Clear all interrupt flags
    _writeReg(0x00, 0x87, 0x00);    // Disable "stop" interrupts
    _writeReg(0x00, 0x81, 0x00);    // Mask other interrupts
    _writeReg(0x00, 0x60, 0x00);    // Disable both analog and DMR transmission
    _writeReg(0x00, 0x00, 0x28);    // Reset register
}

void C5000_startAnalogTx()
{
    _writeReg(0x00, 0x0D, 0x8C);    // Codec control
    _writeReg(0x00, 0x0E, 0x42);    // Mute HPout and enable Line IN
    _writeReg(0x00, 0x0F, 0xC8);    // ADLinVol, mic volume
//     _writeReg(0x00, 0x25, 0x0E);
//     _writeReg(0x00, 0x26, 0xFE);
    _writeReg(0x00, 0x34, 0x1C);    // Disable pre-emphasis, 25kHz bandwidth
    _writeReg(0x00, 0x3E, 0x08);    // "FM Modulation frequency deviation coefficient at the receiving end" (HR_C6000)
    _writeReg(0x00, 0x37, 0xC2);    // Unknown register
//     _writeReg(0x01, 0x50, 0x00);
//     _writeReg(0x01, 0x51, 0x00);
    _writeReg(0x00, 0x60, 0x80);    // Enable analog voice transmission
}

void C5000_stopAnalogTx()
{
    _writeReg(0x00, 0x60, 0x00);    // Disable both analog and DMR transmission
}

bool C5000_spiInUse()
{
    return (gpio_readPin(DMR_CS) == 0) ? true :  false;
}
