/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * SPDX-FileCopyrightText: Copyright 2024 Jamiexu (Jamie793)
 * SPDX-FileCopyrightText: Copyright 2024 Andrej (Tunas1337)
 *
 * SPDX-License-Identifier: GPL-3.0-or-later AND MIT
 *
 * Based on Jamie793's MIT-licensed BK4819 driver for the Talkpod A36Plus,
 * https://github.com/Jamie793/A36Plus_Radio, as extended by Andrej in
 * https://github.com/OpenRTX/OpenRTX/pull/278.
 */

#include "drivers/baseband/BK4819.h"
#include "drivers/baseband/BK4819_priv.h"
#include "interfaces/delays.h"
#include "core/utils.h"

/* REG_30 paths of transmission, microphone included */
static const uint16_t txPaths =
    BK4819_REG30_VCO_CALIB_ENABLE | BK4819_REG30_UNDOC14
    | BK4819_REG30_DISC_MODE_ENABLE | BK4819_REG30_PLL_VCO_ENABLE
    | BK4819_REG30_PA_GAIN_ENABLE | BK4819_REG30_MIC_ADC_ENABLE
    | BK4819_REG30_TX_DSP_ENABLE;

/*
 * Audio processing fields, with their values by enum bk4819_audioMode: voice
 * (the values bk4819_init() sets), then bypass, as the application note's
 * RF_EnterBypass() sets it.
 */
struct audioModeField {
    uint8_t reg;       ///< Register address.
    uint16_t mask;     ///< Fields set by the audio mode.
    uint16_t value[2]; ///< Field values, by audio mode.
};

static const struct audioModeField audioModeFields[] = {
    {
        .reg = 0x2B,
        .mask = BK4819_REG2B_AF_FILTERS_DISABLE,
        .value = { 0, BK4819_REG2B_AF_FILTERS_DISABLE, },
    },
    {
        .reg = 0x31,
        .mask = BK4819_REG31_COMPANDER | BK4819_REG31_VOX,
        .value = { 0, 0, },
    },
    {
        .reg = 0x47,
        .mask = BK4819_REG47_BYPASS,
        .value = { 0, BK4819_REG47_BYPASS, },
    },
    {
        .reg = 0x4B,
        .mask = BK4819_REG4B_ALC_DISABLE,
        .value = { 0, BK4819_REG4B_ALC_DISABLE, },
    },
    {
        .reg = 0x70,
        .mask = BK4819_REG70_TONE1_ENABLE | BK4819_REG70_TONE2_ENABLE,
        .value = { 0, 0, },
    },
    {
        .reg = 0x7E,
        .mask = BK4819_REG7E_TX_DCF_BW_MASK | BK4819_REG7E_RX_DCF_BW_MASK,
        .value = { BK4819_REG7E_TX_DCF_BW(BK4819_DEFAULT_TX_DCF_BW)
                       | BK4819_REG7E_RX_DCF_BW(BK4819_DEFAULT_RX_DCF_BW),
                   0, },
    },
};

/*
 * REG_43 filters by enum bk4819_bandwidth. 12.5 and 25 kHz are the 0x4048
 * and 0x3028 of the stock C62 firmware and of the UV-K5 firmware:
 * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819.c#L292-L299
 * 6.25 kHz is the UV-K5 custom firmware's:
 * https://github.com/egzumer/uv-k5-firmware-custom/blob/7607f0a4bd6203d1f06b70556fc1ce0d7399d6b3/driver/bk4819.c#L647-L660
 */
struct bandwidthFilter {
    uint8_t rfBw;     ///< RX RF filter bandwidth.
    uint8_t rfBwWeak; ///< RX RF filter bandwidth on weak signals.
    uint8_t txLpf2Bw; ///< TX audio LPF2 bandwidth.
};

static const struct bandwidthFilter bandwidthFilters[] = {
    [BK4819_BW_12K5] = { .rfBw = 4, .rfBwWeak = 0, .txLpf2Bw = 1, },
    [BK4819_BW_6K25] = { .rfBw = 3, .rfBwWeak = 3, .txLpf2Bw = 1, },
    [BK4819_BW_25K] = { .rfBw = 3, .rfBwWeak = 0, .txLpf2Bw = 0, },
};

/**
 * Convert a CTCSS tone to the REG_07 frequency word, in CTC1 mode, for a
 * 26 MHz crystal.
 *
 * @param freq: tone frequency, in 0.1 Hz.
 * @return REG_07 frequency word.
 */
static uint16_t ctcssWord(uint16_t freq)
{
    // 2064888 / 1000000 reduced by 8, so that the product fits 32 bits
    return ((uint32_t)freq * 258111) / 125000;
}

/**
 * Convert a tone frequency to the tone generator's units, for a 26 MHz
 * crystal: 10.32444 from the datasheet.
 *
 * @param freq: frequency, in Hz.
 * @return REG_71 value.
 */
static uint16_t scaleFreq(const uint16_t freq)
{
    return (((uint32_t)freq * 1032444u) + 50000u) / 100000u; /* with rounding */
}

uint16_t bk4819_readReg(const struct BK4819 *dev, uint8_t reg)
{
    const uint8_t cmd = reg | BK4819_REG_READ;
    uint8_t rx[2];

    spi_acquire(dev->spi);
    gpioPin_clear(&dev->scn);
    delayUs(1);

    spi_send(dev->spi, &cmd, 1);
    spi_receive(dev->spi, rx, sizeof(rx));

    delayUs(1);
    gpioPin_set(&dev->scn);
    spi_release(dev->spi);

    return ((uint16_t)rx[0] << 8) | rx[1];
}

void bk4819_writeReg(const struct BK4819 *dev, uint8_t reg, uint16_t data)
{
    const uint8_t cmd = reg | BK4819_REG_WRITE;
    const uint8_t tx[2] = {
        (data >> 8) & 0xFF,
        data & 0xFF,
    };

    spi_acquire(dev->spi);
    gpioPin_clear(&dev->scn);
    delayUs(1);

    spi_send(dev->spi, &cmd, 1);
    spi_send(dev->spi, tx, sizeof(tx));

    delayUs(1);
    gpioPin_set(&dev->scn);
    spi_release(dev->spi);
}

void bk4819_maskSetRegister(const struct BK4819 *dev, uint8_t reg,
                            uint16_t mask, uint16_t value)
{
    uint16_t current = bk4819_readReg(dev, reg);
    bk4819_writeReg(dev, reg, (current & ~mask) | (value & mask));
}

void bk4819_init(const struct BK4819 *dev)
{
    /* Configure CS as output, idle high */
    gpioPin_setMode(&dev->scn, OUTPUT);
    gpioPin_set(&dev->scn);

    /*
     * Init sequence of the A36Plus driver's bk4819_init(), except REG_36,
     * REG_47, REG_48 and REG_7D, which differ on the C62:
     * https://github.com/Jamie793/A36Plus_Radio/blob/530a6089c0dba2023c6b7aa9e13d10cd8128ba4a/Drivers/BK4819/bk4819.c#L123-L176
     */
    bk4819_writeReg(dev, 0x00, BIT(15)); // Soft Reset
    bk4819_writeReg(dev, 0x00, 0);       // Normal operation
    bk4819_writeReg(dev, 0x37, 0x1d0f);  // Power mode

    /* Rx AGC Gain */
    bk4819_writeReg(dev, 0x13, 0x3be);
    bk4819_writeReg(dev, 0x12, 0x37b);
    bk4819_writeReg(dev, 0x11, 0x27b);
    bk4819_writeReg(dev, 0x10, 0x7a);
    bk4819_writeReg(dev, 0x14, 0x19);

    bk4819_writeReg(dev, 0x49, 0x2a38);
    bk4819_writeReg(dev, 0x7B, 0x8420);
    bk4819_writeReg(dev, 0x7D, 0xe959); // Mic sensitivity
    bk4819_writeReg(dev, 0x48, 0xb3c1); // AF Gains
    bk4819_writeReg(dev, 0x1E, 0x4c58);
    bk4819_writeReg(dev, 0x1F, 0xa656);
    bk4819_writeReg(dev, 0x3E, 0xa037); // Band selection threshold
    bk4819_writeReg(dev, 0x3F, 0x07fe); // Interrupt enable
    bk4819_writeReg(dev, 0x2A, 0x7fff);
    bk4819_writeReg(dev, 0x28, 0x6b00); // Expander
    bk4819_writeReg(dev, 0x53, 59000);
    bk4819_writeReg(dev, 0x2C, 0x5705);
    bk4819_writeReg(dev, 0x4B, 0x7102); // ALC
    bk4819_setAudioMode(dev, BK4819_AUDIO_VOICE);
    bk4819_writeReg(dev, 0x77, 0x88ef);
    bk4819_writeReg(dev, 0x26, 0x13a0);
    bk4819_writeReg(dev, 0x4E, 0x6f15); // Squelch
    bk4819_writeReg(dev, 0x4F, 0x3f3e); // Ex-noise threshold for Squelch

    /* DTMF Symbol 0 - 15 Coefficient */
    bk4819_writeReg(dev, 0x09, 0x006f);
    bk4819_writeReg(dev, 0x09, 0x106b);
    bk4819_writeReg(dev, 0x09, 0x2067);
    bk4819_writeReg(dev, 0x09, 0x3062);
    bk4819_writeReg(dev, 0x09, 0x4050);
    bk4819_writeReg(dev, 0x09, 0x5047);
    bk4819_writeReg(dev, 0x09, 0x603a);
    bk4819_writeReg(dev, 0x09, 0x702c);
    bk4819_writeReg(dev, 0x09, 0x8041);
    bk4819_writeReg(dev, 0x09, 0x9037);
    bk4819_writeReg(dev, 0x09, 0xa025);
    bk4819_writeReg(dev, 0x09, 0xb017);
    bk4819_writeReg(dev, 0x09, 0xc0e4);
    bk4819_writeReg(dev, 0x09, 0xd0cb);
    bk4819_writeReg(dev, 0x09, 0xe0b5);
    bk4819_writeReg(dev, 0x09, 0xf09f);

    bk4819_writeReg(dev, 0x74, 0xfa02); // 3000Hz AF Response coefficient for Tx
    bk4819_writeReg(dev, 0x44, 0x8f88); // 300Hz AF Response coefficient for Tx
    bk4819_writeReg(dev, 0x45, 0x3201); // 300Hz AF Response coefficient for Rx
    bk4819_writeReg(dev, 0x28, 0x6b38); // Expander
    bk4819_writeReg(dev, 0x29, 0xb4cb); // Compress
    // Tx Output power Bias 0xdf = 2,8V PA Tuning 0xbf = Enable PACTKoutput + 7,26dBm
    bk4819_writeReg(dev, 0x36, 0xdfbf);
    bk4819_writeReg(dev, 0x47, 0x6040); // AF

    /*
     * Let the microphone input bias settle with only the mic ADC enabled,
     * before any audio starts: otherwise the mic AGC can stay at a low gain.
     */
    uint16_t power = bk4819_readReg(dev, 0x30);
    bk4819_writeReg(dev, 0x30, BK4819_REG30_MIC_ADC_ENABLE);
    delayMs(250);
    bk4819_writeReg(dev, 0x30, power);
}

void bk4819_setFrequency(const struct BK4819 *dev, uint32_t freq)
{
    freq = freq / 10; /* Convert to 10 Hz units */
    bk4819_writeReg(dev, 0x39, (freq >> 16) & 0xFFFF);
    bk4819_writeReg(dev, 0x38, freq & 0xFFFF);
}

void bk4819_enableRx(const struct BK4819 *dev)
{
    const uint16_t paths =
        BK4819_REG30_VCO_CALIB_ENABLE | BK4819_REG30_RX_LINK_ENABLE
        | BK4819_REG30_AF_DAC_ENABLE | BK4819_REG30_DISC_MODE_ENABLE
        | BK4819_REG30_PLL_VCO_ENABLE | BK4819_REG30_RX_DSP_ENABLE;

    bk4819_writeReg(dev, 0x37, BK4819_REG37_ACTIVE);
    delayUs(1);
    bk4819_writeReg(dev, 0x30, BK4819_REG30_AF_DAC_ENABLE);
    bk4819_writeReg(dev, 0x30, paths);
}

void bk4819_enableTx(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x37, BK4819_REG37_ACTIVE);
    delayUs(1);
    bk4819_writeReg(dev, 0x30, 0x00); /* reset */
    bk4819_writeReg(dev, 0x30, txPaths);
}

void bk4819_enableTxTone(const struct BK4819 *dev, uint16_t freq, uint8_t gain)
{
    const uint16_t tone = BK4819_REG70_TONE1_ENABLE
                        | BK4819_REG70_TONE1_GAIN(gain);

    bk4819_writeReg(dev, 0x70, tone);
    bk4819_writeReg(dev, 0x71, scaleFreq(freq));
    bk4819_writeReg(dev, 0x30, txPaths & ~BK4819_REG30_MIC_ADC_ENABLE);
}

void bk4819_disableTxTone(const struct BK4819 *dev)
{
    bk4819_maskSetRegister(dev, 0x70, BK4819_REG70_TONE1_ENABLE, 0);
}

void bk4819_disableRtx(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x30, 0x00); /* reset */
    bk4819_writeReg(dev, 0x30, BK4819_REG30_AF_DAC_ENABLE);
}

void bk4819_setModulation(const struct BK4819 *dev, bool fm)
{
    bk4819_setAfOutput(dev, fm ? BK4819_AF_FM : BK4819_AF_AM);
}

void bk4819_setAfOutput(const struct BK4819 *dev, enum bk4819_afOutput af)
{
    bk4819_maskSetRegister(dev, 0x47, BK4819_REG47_AF_OUTPUT_MASK,
                           BK4819_REG47_AF_OUTPUT(af));
}

void bk4819_enableTxCtcss(const struct BK4819 *dev, uint16_t freq, uint8_t gain)
{
    const uint16_t mask = BK4819_REG51_CXCSS_TX_ENABLE | BK4819_REG51_CTCSS_MODE
                        | BK4819_REG51_CXCSS_TX_GAIN_MASK;
    const uint16_t value = BK4819_REG51_CXCSS_TX_ENABLE
                         | BK4819_REG51_CTCSS_MODE
                         | BK4819_REG51_CXCSS_TX_GAIN(gain);

    bk4819_maskSetRegister(dev, 0x51, mask, value);
    bk4819_writeReg(dev, 0x07, ctcssWord(freq));
}

void bk4819_enableRxCtcss(const struct BK4819 *dev, uint16_t freq)
{
    bk4819_maskSetRegister(dev, 0x51, BK4819_REG51_CTCSS_MODE,
                           BK4819_REG51_CTCSS_MODE);
    bk4819_writeReg(dev, 0x07, ctcssWord(freq));
}

void bk4819_enableTxCdcss(const struct BK4819 *dev, uint8_t negative,
                          uint8_t bits24, uint32_t code, uint8_t gain)
{
    const uint16_t mode = BK4819_REG07_MODE_CDCSS
                        | BK4819_REG07_CDCSS_BAUD_134_4HZ;
    const uint16_t high = BK4819_REG08_CDCSS_HIGH
                        | ((code >> 12) & BK4819_REG08_CDCSS_CODE_MASK);
    const uint16_t low = code & BK4819_REG08_CDCSS_CODE_MASK;
    uint16_t reg = BK4819_REG51_CXCSS_TX_ENABLE
                 | BK4819_REG51_CXCSS_TX_GAIN(gain);

    if (negative != 0)
        reg |= BK4819_REG51_TX_CDCSS_NEGATIVE;
    if (bits24 != 0)
        reg |= BK4819_REG51_CDCSS_24BIT;

    bk4819_writeReg(dev, 0x51, reg);
    bk4819_writeReg(dev, 0x07, mode);
    bk4819_writeReg(dev, 0x08, high);
    bk4819_writeReg(dev, 0x08, low);
}

void bk4819_disableCtdcss(const struct BK4819 *dev)
{
    bk4819_maskSetRegister(dev, 0x51, BK4819_REG51_CXCSS_TX_ENABLE, 0);
}

bool bk4819_getCtcss(const struct BK4819 *dev)
{
    return (bk4819_readReg(dev, 0x0C) & BK4819_REG0C_CTC1_RECEIVED) != 0;
}

void bk4819_enableVox(const struct BK4819 *dev, uint8_t delay, uint8_t interval,
                      uint16_t thresholdOn, uint16_t thresholdOff)
{
    const uint16_t timing = BK4819_REG79_VOX_INTERVAL(interval) | thresholdOff;

    (void)delay;
    bk4819_maskSetRegister(dev, 0x31, BK4819_REG31_VOX, BK4819_REG31_VOX);
    bk4819_writeReg(dev, 0x79, timing);
    bk4819_writeReg(dev, 0x46, thresholdOn);
}

bool bk4819_getVox(const struct BK4819 *dev)
{
    return (bk4819_readReg(dev, 0x0C) & BK4819_REG0C_VOX) != 0;
}

void bk4819_setSquelch(const struct BK4819 *dev, uint8_t rssiOpen,
                       uint8_t rssiClose, uint8_t noiseOpen, uint8_t noiseClose,
                       uint8_t glitchOpen, uint8_t glitchClose)
{
    const uint16_t rssi = BK4819_REG78_RSSI_OPEN(rssiOpen)
                        | BK4819_REG78_RSSI_CLOSE(rssiClose);
    const uint16_t noiseMask = BK4819_REG4F_NOISE_CLOSE_MASK
                             | BK4819_REG4F_NOISE_OPEN_MASK;
    const uint16_t noise = BK4819_REG4F_NOISE_CLOSE(noiseClose)
                         | BK4819_REG4F_NOISE_OPEN(noiseOpen);

    bk4819_writeReg(dev, 0x78, rssi);

    // Only the thresholds: timing and reserved bits are kept
    bk4819_maskSetRegister(dev, 0x4F, noiseMask, noise);
    bk4819_maskSetRegister(dev, 0x4D, BK4819_REG4D_GLITCH_CLOSE_MASK,
                           BK4819_REG4D_GLITCH_CLOSE(glitchClose));
    bk4819_maskSetRegister(dev, 0x4E, BK4819_REG4E_GLITCH_OPEN_MASK,
                           BK4819_REG4E_GLITCH_OPEN(glitchOpen));
}

void bk4819_setAudioMode(const struct BK4819 *dev, enum bk4819_audioMode mode)
{
    const bool bypass = (mode == BK4819_AUDIO_BYPASS);

    for (size_t i = 0; i < ARRAY_SIZE(audioModeFields); i++) {
        const struct audioModeField *field = &audioModeFields[i];
        bk4819_maskSetRegister(dev, field->reg, field->mask,
                               field->value[bypass]);
    }

    if (bypass)
        return;

    /*
     * Bypass leaves these to the caller: voice puts back the values of
     * bk4819_init(), and the radio then sets its channel's.
     */
    bk4819_setTxDeviation(dev, true, BK4819_DEFAULT_TX_DEVIATION);
    bk4819_setBandwidth(dev, BK4819_BW_12K5);
    bk4819_setRxAudioGain(dev, BK4819_DEFAULT_RX_AUDIO_GAIN1,
                          BK4819_DEFAULT_RX_AUDIO_GAIN2);
    bk4819_setMicGain(dev, BK4819_DEFAULT_MIC_GAIN);
}

void bk4819_setTxDeviation(const struct BK4819 *dev, bool enable,
                           uint16_t tuning)
{
    const uint16_t mask = BK4819_REG40_TX_DEVIATION_ENABLE
                        | BK4819_REG40_TX_DEVIATION_MASK;
    uint16_t value = BK4819_REG40_TX_DEVIATION(tuning);

    if (enable)
        value |= BK4819_REG40_TX_DEVIATION_ENABLE;

    bk4819_maskSetRegister(dev, 0x40, mask, value);
}

void bk4819_setRfFilter(const struct BK4819 *dev, uint8_t bw, uint8_t weakBw)
{
    const uint16_t mask = BK4819_REG43_RF_BW_MASK
                        | BK4819_REG43_RF_BW_WEAK_MASK;
    const uint16_t value = BK4819_REG43_RF_BW(bw)
                         | BK4819_REG43_RF_BW_WEAK(weakBw);

    bk4819_maskSetRegister(dev, 0x43, mask, value);
}

void bk4819_setTxAudioFilter(const struct BK4819 *dev, uint8_t bw)
{
    bk4819_maskSetRegister(dev, 0x43, BK4819_REG43_AF_TX_LPF2_BW_MASK,
                           BK4819_REG43_AF_TX_LPF2_BW(bw));
}

void bk4819_setBandwidth(const struct BK4819 *dev, enum bk4819_bandwidth bw)
{
    const struct bandwidthFilter *filters = &bandwidthFilters[bw];
    const uint16_t mask = BK4819_REG43_RF_BW_MASK | BK4819_REG43_RF_BW_WEAK_MASK
                        | BK4819_REG43_AF_TX_LPF2_BW_MASK
                        | BK4819_REG43_BW_MODE_MASK | BK4819_REG43_UNDOC3;
    const uint16_t value = BK4819_REG43_RF_BW(filters->rfBw)
                         | BK4819_REG43_RF_BW_WEAK(filters->rfBwWeak)
                         | BK4819_REG43_AF_TX_LPF2_BW(filters->txLpf2Bw)
                         | BK4819_REG43_BW_MODE(bw) | BK4819_REG43_UNDOC3;

    bk4819_maskSetRegister(dev, 0x43, mask, value);
}

void bk4819_setRxAudioGain(const struct BK4819 *dev, uint8_t gain1,
                           uint8_t gain2)
{
    const uint16_t mask = BK4819_REG48_AF_RX_GAIN1_MASK
                        | BK4819_REG48_AF_RX_GAIN2_MASK;
    const uint16_t value = BK4819_REG48_AF_RX_GAIN1(gain1)
                         | BK4819_REG48_AF_RX_GAIN2(gain2);

    bk4819_maskSetRegister(dev, 0x48, mask, value);
}

void bk4819_setMicGain(const struct BK4819 *dev, uint8_t gain)
{
    bk4819_maskSetRegister(dev, 0x7D, BK4819_REG7D_MIC_GAIN_MASK,
                           BK4819_REG7D_MIC_GAIN(gain));
}

int16_t bk4819_getRssi(const struct BK4819 *dev)
{
    return ((bk4819_readReg(dev, 0x67) & BK4819_REG67_RSSI_MASK) / 2) - 160;
}

uint16_t bk4819_getMicLevel(const struct BK4819 *dev)
{
    return (bk4819_readReg(dev, 0x6F) & BK4819_REG6F_AF_AMPLITUDE_MASK) * 2;
}

void bk4819_beepStart(const struct BK4819 *dev, uint16_t freq, bool tuningGain)
{
    const uint16_t tone = BK4819_REG70_TONE1_ENABLE
                        | BK4819_REG70_TONE1_GAIN(96);
    const uint16_t paths = BK4819_REG30_AF_DAC_ENABLE
                         | BK4819_REG30_DISC_MODE_ENABLE
                         | BK4819_REG30_TX_DSP_ENABLE;

    (void)tuningGain;
    bk4819_writeReg(dev, 0x50, 0x3B20);
    bk4819_setAfOutput(dev, BK4819_AF_BEEP);
    bk4819_writeReg(dev, 0x70, tone);
    bk4819_writeReg(dev, 0x71, scaleFreq(freq));
    bk4819_writeReg(dev, 0x30, 0);
    bk4819_writeReg(dev, 0x30, paths);
    bk4819_writeReg(dev, 0x50, 0x3B20);
}

void bk4819_beepStop(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x50, 0xBB20);
}
