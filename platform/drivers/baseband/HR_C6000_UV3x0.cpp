/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include "HR_C6000.h"

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

template class HR_Cx000 < C6000_SpiOpModes >;

template< class M >
void HR_Cx000< M >::init()
{
    gpio_setMode(DMR_CS,    OUTPUT);
    gpio_setMode(DMR_SLEEP, OUTPUT);

    gpio_setPin(DMR_SLEEP);

    delayMs(10);
    gpio_clearPin(DMR_SLEEP);         // Exit from sleep pulling down DMR_SLEEP
    delayMs(10);

    writeReg(M::CONFIG, 0x0A, 0x80);    //Clock connected to crystal
    writeReg(M::CONFIG, 0x0B, 0x28);    //Set PLL M Register
    writeReg(M::CONFIG, 0x0C, 0x33);    //Set PLL Dividers
    delayMs(250);

    writeReg(M::CONFIG, 0x0A, 0x00);
    writeReg(M::CONFIG, 0xB9, 0x05);
    writeReg(M::CONFIG, 0xBA, 0x04);
    writeReg(M::CONFIG, 0xBB, 0x02);
    writeReg(M::CONFIG, 0xA1, 0x80);
    writeReg(M::CONFIG, 0x10, 0xF3);
    writeReg(M::CONFIG, 0x40, 0x43);
    writeReg(M::CONFIG, 0x07, 0x0B);
    writeReg(M::CONFIG, 0x08, 0xB8);
    writeReg(M::CONFIG, 0x09, 0x00);
    sendSequence(initSeq1, sizeof(initSeq1));
    writeReg(M::CONFIG, 0x01, 0xF8);
    sendSequence(initSeq2, sizeof(initSeq2));
    writeReg(M::CONFIG, 0x00, 0x2A);

    writeReg(M::CONFIG, 0x06, 0x20);
    writeReg(M::CONFIG, 0x14, 0x59);
    writeReg(M::CONFIG, 0x15, 0xF5);
    writeReg(M::CONFIG, 0x16, 0x21);
    sendSequence(initSeq3, sizeof(initSeq3));
    sendSequence(initSeq4, sizeof(initSeq4));
    sendSequence(initSeq5, sizeof(initSeq5));
    sendSequence(initSeq6, sizeof(initSeq6));
    writeReg(M::AUX,    0x52, 0x08);
    writeReg(M::AUX,    0x53, 0xEB);
    writeReg(M::AUX,    0x54, 0x78);
    writeReg(M::AUX,    0x45, 0x1E);
    writeReg(M::AUX,    0x37, 0x50);
    writeReg(M::AUX,    0x35, 0xFF);
    writeReg(M::CONFIG, 0x39, 0x02);
    writeReg(M::CONFIG, 0x3D, 0x0A);
    writeReg(M::CONFIG, 0x83, 0xFF);
    writeReg(M::CONFIG, 0x87, 0x00);
    writeReg(M::CONFIG, 0x65, 0x0A);
    writeReg(M::CONFIG, 0x1D, 0xFF);
    writeReg(M::CONFIG, 0x1E, 0xF1);
    writeReg(M::CONFIG, 0xE2, 0x06);
    writeReg(M::CONFIG, 0xE4, 0x27);
    writeReg(M::CONFIG, 0xE3, 0x52);
    writeReg(M::CONFIG, 0xE5, 0x1A);
    writeReg(M::CONFIG, 0xE1, 0x0F);
    writeReg(M::CONFIG, 0xD1, 0xC4);
    writeReg(M::CONFIG, 0x25, 0x0E);
    writeReg(M::CONFIG, 0x26, 0xFD);
    writeReg(M::CONFIG, 0x64, 0x00);
}

template< class M >
void HR_Cx000< M >::terminate()
{
    gpio_setPin(DMR_SLEEP);
    gpio_setMode(DMR_CS, INPUT);
}

template< class M >
void HR_Cx000< M >::setModOffset(const uint16_t offset)
{
    /*
     * Same as original TYT firmware.
     * Reference: functions @0802e7d4 and @080546cc in S18.16 binary image.
     *
     * Cast to uint8_t to have the exact situation of the original firmware.
     */
    uint8_t value    = static_cast< uint8_t>(offset);
    uint8_t offUpper = (value < 0x80) ? 0x03 : 0x00;
    uint8_t offLower = value + 0x80;

    writeReg(M::CONFIG, 0x48, offUpper);    // Two-point bias, upper value
    writeReg(M::CONFIG, 0x47, offLower);    // Two-point bias, lower value

    writeReg(M::CONFIG, 0x04, offLower);    // Bias value for TX, Q-channel
}

template< class M >
void HR_Cx000< M >::dmrMode()
{
    writeReg(M::CONFIG, 0x10, 0x4F);
    writeReg(M::CONFIG, 0x81, 0x19);
    writeReg(M::CONFIG, 0x01, 0xF0);
    writeReg(M::CONFIG, 0xE4, 0x27);
    writeReg(M::CONFIG, 0xE5, 0x1A);
    writeReg(M::CONFIG, 0x25, 0x0E);
    writeReg(M::CONFIG, 0x26, 0xFD);
    writeReg(M::AUX,    0x54, 0x78);
    writeReg(M::CONFIG, 0x1F, 0x10);
    writeReg(M::AUX,    0x24, 0x00);
    writeReg(M::AUX,    0x25, 0x00);
    writeReg(M::AUX,    0x26, 0x00);
    writeReg(M::AUX,    0x27, 0x00);
    writeReg(M::CONFIG, 0x41, 0x40);
    writeReg(M::CONFIG, 0x56, 0x00);
    writeReg(M::CONFIG, 0x41, 0x40);
    writeReg(M::CONFIG, 0x5C, 0x09);
    writeReg(M::CONFIG, 0x5F, 0xC0);
    sendSequence(initSeq7, sizeof(initSeq7));
    writeReg(M::CONFIG, 0x11, 0x80);
}

template< class M >
void HR_Cx000< M >::fmMode()
{
    writeReg(M::CONFIG, 0x10, 0xF3);
    writeReg(M::CONFIG, 0x01, 0xB0);
    writeReg(M::CONFIG, 0x81, 0x04);
    writeReg(M::CONFIG, 0xE5, 0x1A);
    writeReg(M::CONFIG, 0x36, 0x02);
    writeReg(M::CONFIG, 0xE4, 0x27);
    writeReg(M::CONFIG, 0xE2, 0x06);
    writeReg(M::CONFIG, 0x34, 0x98);
    writeReg(M::CONFIG, 0x60, 0x00);
    writeReg(M::CONFIG, 0x1F, 0x00);
    writeReg(M::AUX,    0x24, 0x00);
    writeReg(M::AUX,    0x25, 0x00);
    writeReg(M::AUX,    0x26, 0x00);
    writeReg(M::AUX,    0x27, 0x00);
    writeReg(M::CONFIG, 0x56, 0x00);
    writeReg(M::CONFIG, 0x41, 0x40);
    writeReg(M::CONFIG, 0x5C, 0x09);
    writeReg(M::CONFIG, 0x5F, 0xC0);
    sendSequence(initSeq7, sizeof(initSeq7));
    writeReg(M::CONFIG, 0x11, 0x80);
    writeReg(M::CONFIG, 0xE0, 0xC9);

    writeReg(M::CONFIG, 0x37, 0x81);
}

template< class M >
void HR_Cx000< M >::startAnalogTx(const TxAudioSource source, const FmConfig cfg)
{
    /*
     * NOTE: on MD-UV3x0 the incoming audio from the microphone is connected to
     * "Linein1" input, while signal coming from the tone generator is connected
     *  to "Mic_p".
     */
    uint8_t audioCfg = 0x81;
    if(source == TxAudioSource::MIC)     audioCfg |= 0x40;
    if(source == TxAudioSource::LINE_IN) audioCfg |= 0x02;

    writeReg(M::CONFIG, 0xE2, 0x00);
    writeReg(M::CONFIG, 0xE4, 0x23);
    writeReg(M::CONFIG, 0xC2, 0x00);
    writeReg(M::CONFIG, 0xA1, 0x80);
//     writeReg(M::CONFIG, 0x25, 0x0E);
//     writeReg(M::CONFIG, 0x26, 0xFE);
    writeReg(M::CONFIG, 0x83, 0xFF);
    writeReg(M::CONFIG, 0x87, 0x00);
    writeReg(M::CONFIG, 0x04, 0x24);
    writeReg(M::CONFIG, 0x35, 0x40);
    writeReg(M::CONFIG, 0x3F, 0x04);
    writeReg(M::CONFIG, 0x34, static_cast< uint8_t >(cfg));
    writeReg(M::CONFIG, 0x3E, 0x08);
    writeReg(M::AUX,    0x50, 0x00);
    writeReg(M::AUX,    0x51, 0x00);
    writeReg(M::CONFIG, 0x60, 0x80);
    writeReg(M::CONFIG, 0x10, 0xF3);

    writeReg(M::CONFIG, 0xE0, audioCfg);
    writeReg(M::CONFIG, 0x37, 0x8C);
}

template< class M >
void HR_Cx000< M >::stopAnalogTx()
{
    writeReg(M::CONFIG, 0x60, 0x00);
    writeReg(M::CONFIG, 0xE0, 0xC9);
    writeReg(M::CONFIG, 0xE2, 0x06);
    writeReg(M::CONFIG, 0x34, 0x98);
    writeReg(M::CONFIG, 0x37, 0x81);
}

/*
 * SPI interface driver
 */
template< class M >
void HR_Cx000<M>::uSpi_init()
{
    gpio_setMode(DMR_CS,    OUTPUT);
    gpio_setMode(DMR_CLK,   OUTPUT);
    gpio_setMode(DMR_MOSI,  OUTPUT);
    gpio_setMode(DMR_MISO,  OUTPUT);

    // Deselect HR_C6000, idle state of the CS line.
    gpio_setPin(DMR_CS);
}

template< class M >
uint8_t HR_Cx000<M>::uSpi_sendRecv(const uint8_t value)
{
    gpio_clearPin(DMR_CLK);

    uint8_t incoming = 0;

    for(uint8_t cnt = 0; cnt < 8; cnt++)
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
