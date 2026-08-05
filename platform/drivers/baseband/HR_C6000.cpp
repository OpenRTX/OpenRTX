/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/utils.h"
#include "drivers/baseband/HR_C6000.h"
#include "radioUtils.h"

/*
 * Table of HR_C6000 CTCSS tones, used for reverse lookup of tone index to be
 * written in the configuration register. Taken from datasheet at page 90.
 */
static const uint16_t ctcssToneTable[] =
{
    670,  719,  744,  770,  797,  825,  854,
    885,  915,  948,  974,  1000, 1035, 1072,
    1109, 1148, 1188, 1230, 1273, 1318, 1365,
    1413, 1462, 1514, 1567, 1622, 1679, 1738,
    1799, 1862, 1928, 2035, 2107, 2181, 2257,
    2336, 2418, 2503, 693,  625,  1598, 1655,
    1713, 1773, 1835, 1899, 1966, 1995, 2065,
    2291, 2541
};

static uint8_t getToneIndex(const tone_t tone)
{
    uint8_t idx;

    for(idx = 0; idx < ARRAY_SIZE(ctcssToneTable); idx += 1)
    {
        if(ctcssToneTable[idx] == tone)
            break;
    }

    return idx + 1;
}

void HR_C6000::setTxCtcss(const tone_t tone, const uint8_t deviation)
{
    uint8_t index = getToneIndex(tone);
    writeCfgRegister(0xA8, index);        // Set CTCSS tone index
    writeCfgRegister(0xA0, deviation);    // Set CTCSS tone deviation
    writeCfgRegister(0xA1, 0x08);         // Enable CTCSS
}

void HR_C6000::setRxCtcss(const tone_t tone)
{
    uint8_t index = getToneIndex(tone);
    writeCfgRegister(0xA1, 0x08);         // Enable CTCSS
    writeCfgRegister(0xA7, 0x10);         // CTCSS detection threshold, value from datasheet
    writeCfgRegister(0xD3, 0x07);         // CTCSS sampling depth, value from datasheet
    writeCfgRegister(0xD2, 0xD0);
    writeCfgRegister(0xD4, index);        // Tone index
}

void HR_C6000::sendTone(const uint32_t freq, const uint8_t deviation)
{
    uint32_t tone = (freq * 65536) / 32000;

    // A single tone is produced by retuning both of the oscillators that DTMF
    // digit '1' mixes (697 Hz and 1209 Hz) to the requested frequency and then
    // keying that digit: the two oscillators beat at the same frequency, so
    // only one tone reaches the modulator. The oscillator slots live in the
    // auxiliary register bank, see initDtmf().
    writeReg16(C6000_SpiOpModes::AUX, 0x11B, (tone >> 8) & 0xFF);
    writeReg16(C6000_SpiOpModes::AUX, 0x11A, (tone & 0xFF));
    writeReg16(C6000_SpiOpModes::AUX, 0x123, (tone >> 8) & 0xFF);
    writeReg16(C6000_SpiOpModes::AUX, 0x122, (tone & 0xFF));

    // The 697 Hz and 1209 Hz slots are left detuned, so the tone table has to
    // be programmed again before the next DTMF digit. sendDtmfKey() picks this
    // up on its own.
    dtmfTableValid = false;

    sendDtmf(0x01, deviation);
}

void HR_C6000::initDtmf()
{
    // The HR_C6000 has a built-in DTMF generator: eight oscillator slots
    // (0x11A..0x129) hold the eight standard DTMF frequencies, and the digit
    // code selects which low+high pair the chip mixes. The slots live in the
    // auxiliary register bank, not in the main configuration one.
    static const uint16_t dtmfFreq[8] =
        { 697, 770, 852, 941, 1209, 1336, 1477, 1633 };
    for(uint8_t i = 0; i < 8; i++)
    {
        uint32_t tone = ((uint32_t)dtmfFreq[i] * 65536) / 32000;
        uint16_t addr = 0x11A + (i * 2);
        writeReg16(C6000_SpiOpModes::AUX, addr + 1, (tone >> 8) & 0xFF);
        writeReg16(C6000_SpiOpModes::AUX, addr,     (tone & 0xFF));
    }

    dtmfTableValid = true;
}

void HR_C6000::sendDtmf(const uint8_t code, const uint8_t deviation)
{
    // Assumes initDtmf() has already programmed the tone table. Selects the
    // digit's low+high pair from that table and keys it on air.
    writeCfgRegister(0xA1, 0x82);          // Enable DTMF mode
    writeCfgRegister(0xA0, deviation);     // Set DTMF tone deviation
    writeCfgRegister(0xA4, 0xFA);          // Tone time (large -> continuous)
    writeCfgRegister(0xA3, 0x19);          // Tone gap (unused for a single code)

    // The high nibble of 0xD1 holds undocumented FM-DTMF bits set elsewhere;
    // preserve them and only set the code count (low nibble) to one.
    uint8_t codeCount = (readCfgRegister(0xD1) & 0xF0) | 0x01;
    writeCfgRegister(0xD1, codeCount);
    writeCfgRegister(0xAF, code << 4);     // First code goes in the high nibble
    writeCfgRegister(0x60, 0x00);          // Disable FM transmission
    writeCfgRegister(0x60, 0x80);          // Enable FM transmission, start sending
}

void HR_C6000::sendDtmfKey(const uint8_t key, const freq_t txFrequency,
                           const bool narrowBand)
{
    // Keypad code to HR_C6000 digit code: the digits map to themselves, while
    // '*' and '#' are the last two entries of the chip's code space.
    static const uint8_t codeMap[12] =
        { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xE /* * */, 0xF /* # */ };

    if(key >= 12)
        return;

    // Program the tone table when it is not already good: this is the case on
    // the first digit, and after sendTone() has borrowed one of the slots.
    if(dtmfTableValid == false)
        initDtmf();

    // Deviation coefficients for the DTMF generator, per band and bandwidth.
    uint8_t deviation;
    if(getBandFromFrequency(txFrequency) == BND_VHF)
        deviation = narrowBand ? 0x3A : 0x75;
    else
        deviation = narrowBand ? 0x12 : 0x24;

    sendDtmf(codeMap[key], deviation);
}
