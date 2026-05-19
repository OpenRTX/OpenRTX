/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "interfaces/delays.h"
#include "hwconfig.h"
#include "drivers/baseband/HR_C6000.h"

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
    gpio_setMode(DMR_SLEEP, OUTPUT);
    gpio_setPin(DMR_SLEEP);

    delayMs(10);
    gpio_clearPin(DMR_SLEEP);           // Exit from sleep pulling down DMR_SLEEP
    delayMs(10);

    writeReg(M::CONFIG, 0x0A, 0x80);    // Clock connected to crystal
    writeReg(M::CONFIG, 0x0B, 0x28);    // Set PLL M Register
    writeReg(M::CONFIG, 0x0C, 0x33);    // Set PLL Dividers
    delayMs(250);

    writeReg(M::CONFIG, 0x0A, 0x00);    // Clock connected to PLL
    writeReg(M::CONFIG, 0xB9, 0x05);    // System clock frequency
    writeReg(M::CONFIG, 0xBA, 0x04);    // Codec clock frequency
    writeReg(M::CONFIG, 0xBB, 0x02);    // Output clock frequency
    writeReg(M::CONFIG, 0xA1, 0x80);    // FM_mod, all modes cleared
    writeReg(M::CONFIG, 0x10, 0xF3);    // FM mode, Tier II, TimeSlot, 3rd layer mode, aligned (?)
    writeReg(M::CONFIG, 0x40, 0x43);    // Enable RX synchronisation, normal mode (no test)
    writeReg(M::CONFIG, 0x07, 0x0B);    // IF frequency - high 8 bit
    writeReg(M::CONFIG, 0x08, 0xB8);    // IF frequency - mid 8 bit
    writeReg(M::CONFIG, 0x09, 0x00);    // IF frequency - low 8 bit
    sendSequence(initSeq1, sizeof(initSeq1));
    writeReg(M::CONFIG, 0x01, 0xF8);    // Swap TX IQ, swap RX IQ, two point mode for TX, baseband IQ mode for RX
    sendSequence(initSeq2, sizeof(initSeq2));
    writeReg(M::CONFIG, 0x00, 0x2A);    // Reset codec, reset vocoder, reset I2S

    writeReg(M::CONFIG, 0x06, 0x20);    // Vocoder output connected to universal interface (?)
    writeReg(M::CONFIG, 0x14, 0x59);    // local address - low 8 bit
    writeReg(M::CONFIG, 0x15, 0xF5);    // local address - mid 8 bit
    writeReg(M::CONFIG, 0x16, 0x21);    // local address - high 8 bit
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
    writeReg(M::CONFIG, 0x39, 0x02);    // Undocumented register
    writeReg(M::CONFIG, 0x3D, 0x0A);    // Undocumented register
    writeReg(M::CONFIG, 0x83, 0xFF);    // Clear all interrupt flags
    writeReg(M::CONFIG, 0x87, 0x00);    // Disable all interrupt sources
    writeReg(M::CONFIG, 0x65, 0x0A);    // Undocumented register
    writeReg(M::CONFIG, 0x1D, 0xFF);    // Local unaddress, mask unaddress (?)
    writeReg(M::CONFIG, 0x1E, 0xF1);    // Broadcast RX address, broadcast address mask
    writeReg(M::CONFIG, 0xE2, 0x00);    // DAC off, mic preamp disabled
    writeReg(M::CONFIG, 0xE4, 0x27);    // Lineout gain, first and second stage mic gain
    writeReg(M::CONFIG, 0xE3, 0x52);    // Internal ADC and DAC passthrough enable
    writeReg(M::CONFIG, 0xE5, 0x1A);    // Undocumented register
    writeReg(M::CONFIG, 0xE1, 0x0F);    // Undocumented register
    writeReg(M::CONFIG, 0xD1, 0xC4);    // DTMF code width (?)
    writeReg(M::CONFIG, 0x25, 0x0E);    // Undocumented register
    writeReg(M::CONFIG, 0x26, 0xFD);    // Undocumented register
    writeReg(M::CONFIG, 0x64, 0x00);    // Undocumented register
}

template< class M >
void HR_Cx000< M >::terminate()
{
    gpio_setPin(DMR_SLEEP);
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
    writeReg(M::CONFIG, 0x10, 0x4F);    // DMR mode, Tier I, TimeSlot, 2nd layer mode, relay, aligned (?)
    writeReg(M::CONFIG, 0x81, 0x19);    // Interrupt mask
    writeReg(M::CONFIG, 0x01, 0xF0);    // Swap TX IQ, swap RX IQ, two point mode for TX, IF mode for RX
    writeReg(M::CONFIG, 0xE4, 0x27);    // Lineout gain, first and second stage mic gain
    writeReg(M::CONFIG, 0xE5, 0x1A);    // Undocumented register
    writeReg(M::CONFIG, 0x25, 0x0E);    // Undocumented register
    writeReg(M::CONFIG, 0x26, 0xFD);    // Undocumented register
    writeReg(M::AUX,    0x54, 0x78);
    writeReg(M::CONFIG, 0x1F, 0x10);    // Color code, encryption disabled
    writeReg(M::AUX,    0x24, 0x00);
    writeReg(M::AUX,    0x25, 0x00);
    writeReg(M::AUX,    0x26, 0x00);
    writeReg(M::AUX,    0x27, 0x00);
    writeReg(M::CONFIG, 0x41, 0x40);    // Start RX for upcoming time slot interrupt
    writeReg(M::CONFIG, 0x56, 0x00);    // Undocumented register
    writeReg(M::CONFIG, 0x41, 0x40);    // Start RX for upcoming time slot interrupt
    writeReg(M::CONFIG, 0x5C, 0x09);    // Undocumented register
    writeReg(M::CONFIG, 0x5F, 0xC0);    // Detect BS and MS frame sequences in 2 layer mode
    sendSequence(initSeq7, sizeof(initSeq7));
    writeReg(M::CONFIG, 0x11, 0x80);    // Local channel mode
}

template< class M >
void HR_Cx000< M >::fmMode()
{
    writeReg(M::CONFIG, 0x10, 0x80);    // FM mode, Tier II, TimeSlot, 3rd layer mode, aligned (?)
    writeReg(M::CONFIG, 0x01, 0xB0);    // Swap TX IQ, two point mode for TX, IF mode for RX
    writeReg(M::CONFIG, 0x81, 0x04);    // Interrupt mask
    writeReg(M::CONFIG, 0xE5, 0x1A);    // Undocumented register
    writeReg(M::CONFIG, 0xE4, 0x27);    // Lineout gain, first and second stage mic gain
    writeReg(M::CONFIG, 0x34, 0x98);    // FM bpf enabled, 25kHz bandwidth
    writeReg(M::CONFIG, 0x60, 0x00);    // Disable both analog and DMR transmission
    writeReg(M::CONFIG, 0x1F, 0x00);    // Color code, encryption disabled
    writeReg(M::AUX,    0x24, 0x00);
    writeReg(M::AUX,    0x25, 0x00);
    writeReg(M::AUX,    0x26, 0x00);
    writeReg(M::AUX,    0x27, 0x00);
    writeReg(M::CONFIG, 0x56, 0x00);    // Undocumented register
    writeReg(M::CONFIG, 0x41, 0x40);    // Start RX for upcoming time slot interrupt
    writeReg(M::CONFIG, 0x5C, 0x09);    // Undocumented register
    writeReg(M::CONFIG, 0x5F, 0xC0);    // Detect BS and MS frame sequences in 2 layer mode
    sendSequence(initSeq7, sizeof(initSeq7));
    writeReg(M::CONFIG, 0x11, 0x80);    // Local channel mode
    writeReg(M::CONFIG, 0xE0, 0xC9);    // Codec enabled, LineIn1, LineOut2, I2S slave mode
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

    // writeReg(M::CONFIG, 0xE2, 0x00);    // Mic preamp disabled, anti-pop disabled
    writeReg(M::CONFIG, 0xE0, audioCfg);
    writeReg(M::CONFIG, 0xC2, 0x00);    // Codec AGC gain
    writeReg(M::CONFIG, 0xE5, 0x1A);    // Unknown (Default value = 0A)
    writeReg(M::CONFIG, 0x25, 0x0E);    // Undocumented Register
    writeReg(M::CONFIG, 0x83, 0xFF);    // Clear all Interrupts
    writeReg(M::CONFIG, 0x87, 0x00);    // Clear Int Masks
    writeReg(M::CONFIG, 0xA1, 0x80);    // FM_mod, all modes cleared
    writeReg(M::CONFIG, 0x83, 0xFF);    // Clear all interrupt flags
    writeReg(M::CONFIG, 0x87, 0x00);    // Disable all interrupt sources
    writeReg(M::CONFIG, 0x34, static_cast< uint8_t >(cfg));
    writeReg(M::AUX,    0x50, 0x00);
    writeReg(M::AUX,    0x51, 0x00);
    writeReg(M::CONFIG, 0x3E, 0x08);    // FM Modulation frequency deviation coefficient at the receiving end
    writeReg(M::CONFIG, 0x60, 0x80);    // Start analog transmission
}

template< class M >
void HR_Cx000< M >::stopAnalogTx()
{
    writeReg(M::CONFIG, 0x60, 0x00);    // Stop analog transmission
    writeReg(M::CONFIG, 0xE0, 0xC9);    // Codec enabled, LineIn1, LineOut2, I2S slave mode
    writeReg(M::CONFIG, 0x34, 0x98);    // FM bpf enabled, 25kHz bandwidth
}
