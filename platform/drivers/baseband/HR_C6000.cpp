/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <array>
#include "core/utils.h"
#include "drivers/baseband/HR_C6000.h"

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

    // Set  DTMF tone osc 1 to frequency of the required tone
    writeReg16(C6000_SpiOpModes::CONFIG, 0x11A, (tone & 0xFF));
    writeReg16(C6000_SpiOpModes::CONFIG, 0x11B, (tone >> 8) & 0xFF);

    // Set  DTMF tone osc 2 to frequency of the required tone
    writeReg16(C6000_SpiOpModes::CONFIG, 0x122, (tone & 0xFF));
    writeReg16(C6000_SpiOpModes::CONFIG, 0x123, (tone >> 8) & 0xFF);

    writeCfgRegister(0xA1, 0x02);         // Enable DTMF
    writeCfgRegister(0xA0, deviation);    // Set DTMF tone deviation
    writeCfgRegister(0xA4, 0xFF);         // Set the tone time to maximum
    writeCfgRegister(0xA3, 0x00);         // Set the tone gap to zero
    writeCfgRegister(0xD1, 0x06);         // Set the number of codes to six
    writeCfgRegister(0xAF, 0x11);         // Set the same code to be sent six times (2 codes per register)
    writeCfgRegister(0xAE, 0x11);
    writeCfgRegister(0xAD, 0x11);
    writeCfgRegister(0x60, 0x00);         // Disable FM transmission
    writeCfgRegister(0x60, 0x80);         // Enable FM transmission, start sending the tone
}

/*
 * Registers the symbol transmission and the FM copy change. Radios leave them
 * in different receive set-ups, so each mode puts back what was written to
 * them before (some read back otherwise).
 */
static constexpr uint8_t SYMBOL_TX_REGS[] = { 0x41, 0x10, 0x20, 0x21, 0x22, 0x40 };
static constexpr uint8_t FM_COPY_REGS[]   = { 0x41, 0x36, 0x60, 0x34, 0x10, 0x81,
                                              0x26 };

void HR_C6000::startSymbolTx()
{
    for(size_t i = 0; i < ARRAY_SIZE(SYMBOL_TX_REGS); i++)
        savedRegs[i] = writtenCfgRegister(SYMBOL_TX_REGS[i]);

    std::array< uint8_t, SYMBOL_SLOT_BYTES > idle;
    idle.fill(SYMBOL_IDLE);

    writeCfgRegister(0x41, 0x00);       // No slot activity during the set-up
    writeCfgRegister(0x10, 0x66);       // Physical layer mode
    writeCfgRegister(0x20, 0x00);
    writeCfgRegister(0x21, 0x02);       // Impolite access
    writeCfgRegister(0x22, 0x86);
    writeRam(C6000_SpiOpModes::DATA, 0, idle.data(), idle.size());
    writeRam(C6000_SpiOpModes::DATA, SYMBOL_SLOT_BYTES, idle.data(),
             idle.size());
    writeCfgRegister(0x40, 0xE3);       // Own timing: the slot clock starts
}

void HR_C6000::writeSymbolSlot(const uint32_t interrupt, const uint8_t *symbols)
{
    // Slot n + 1 is set up at interrupt n, from the first half when n is odd
    const uint8_t addr = ((interrupt & 1) != 0) ? 0 : SYMBOL_SLOT_BYTES;

    writeRam(C6000_SpiOpModes::DATA, addr, symbols, SYMBOL_SLOT_BYTES);
    writeCfgRegister(0x41, 0x80);       // Send in the next slot
}

void HR_C6000::stopSymbolTx()
{
    writeCfgRegister(0x41, 0x00);       // No more slots

    for(size_t i = ARRAY_SIZE(SYMBOL_TX_REGS); i > 0; i--)
        writeCfgRegister(SYMBOL_TX_REGS[i - 1], savedRegs[i - 1]);
}

void HR_C6000::startFmCopy()
{
    for(size_t i = 0; i < ARRAY_SIZE(FM_COPY_REGS); i++)
        savedRegs[i] = writtenCfgRegister(FM_COPY_REGS[i]);

    writeCfgRegister(0x41, 0x00);       // No physical layer slot activity
    writeCfgRegister(0x36, 0x00);       // FM audio not to the codec
    writeCfgRegister(0x60, 0x00);       // No transmission
    writeCfgRegister(0x34, 0x00);       // Receive audio filters off
    writeCfgRegister(0x10, 0x80);       // FM mode
    writeCfgRegister(0x81, 0x02);       // Only the copy's system interrupt
    writeCfgRegister(0x26, 0xFD);       // FM receive on (0xFE is off)
    writeCfgRegister(0x83, 0xFF);       // Clear the flags

    fmCopyLast[0] = fmCopyLastSample(0);
    fmCopyLast[1] = fmCopyLastSample(1);
    fmCopyHalf    = 1;
}

void HR_C6000::readFmCopy(int16_t *samples)
{
    writeCfgRegister(0x83, 0x02);       // Clear the copy's flag

    // The flag does not say which half: the one whose last sample changed.
    // When both or neither did (a steady input), the other one than last time
    const int16_t now[2]  = { fmCopyLastSample(0), fmCopyLastSample(1) };
    const bool changed[2] = { now[0] != fmCopyLast[0], now[1] != fmCopyLast[1] };
    uint8_t half = fmCopyHalf ^ 1;
    if(changed[0] != changed[1])
        half = changed[0] ? 0 : 1;

    const size_t bytes = FM_COPY_SAMPLES * sizeof(int16_t);
    readRam16(C6000_SpiOpModes::DATA, FM_COPY_RAM + half * bytes,
              reinterpret_cast< uint8_t * >(samples), bytes);

    for(size_t i = 0; i < FM_COPY_SAMPLES; i++)
        samples[i] = static_cast< int16_t >(__builtin_bswap16(samples[i]));

    fmCopyLast[0] = now[0];
    fmCopyLast[1] = now[1];
    fmCopyHalf    = half;
}

void HR_C6000::stopFmCopy()
{
    for(size_t i = ARRAY_SIZE(FM_COPY_REGS); i > 0; i--)
        writeCfgRegister(FM_COPY_REGS[i - 1], savedRegs[i - 1]);

    writeCfgRegister(0x83, 0xFF);
}

int16_t HR_C6000::fmCopyLastSample(const uint8_t half)
{
    const size_t bytes = FM_COPY_SAMPLES * sizeof(int16_t);
    uint8_t b[2];

    readRam16(C6000_SpiOpModes::DATA, FM_COPY_RAM + (half + 1) * bytes - 2,
              b, sizeof(b));

    return static_cast< int16_t >((b[0] << 8) | b[1]);
}
