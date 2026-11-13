/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/BK4819.h"
#include "interfaces/delays.h"
#include "core/utils.h"

#define BITV(x, y) ((x) << (y))

/*
 * Register fields. From the BK4819 (V3) register list and application note:
 * https://github.com/amnemonic/Quansheng_UV-K5_Firmware/tree/main/hardware
 * and the UV-K5 firmware's register header:
 * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819-regs.h
 */

/* Serial bus command */
enum {
    BK4819_REG_READ = BIT(7),
    BK4819_REG_WRITE = 0,
};

/* REG_07: CTCSS/CDCSS frequency */
enum {
    BK4819_REG07_MODE_CTC1 = 0,
    BK4819_REG07_MODE_CTC2 = BIT(13),
    BK4819_REG07_MODE_CDCSS = BITV(2, 13),
    BK4819_REG07_FREQUENCY_MASK = 0x1fff,
};

/* REG_0C: status */
enum {
    BK4819_REG0C_INTERRUPT = BIT(0),
};

/* REG_0D: frequency scan result, high word */
enum {
    BK4819_REG0D_SCAN_BUSY = BIT(15),
    BK4819_REG0D_SCAN_FREQ_MASK = 0x07ff,
};

/* REG_24: DTMF/SelCall */
enum {
    BK4819_REG24_THRESHOLD_MASK = BITV(0x2f, 7),
    BK4819_REG24_ENABLE = BIT(5),
    BK4819_REG24_SELECT_DTMF = BIT(4),
    BK4819_REG24_MAX_SYMBOLS_MASK = 0x000f,
};

/* REG_2B: AF filters */
enum {
    BK4819_REG2B_AF_RX_HPF300_DISABLE = BIT(10),
    BK4819_REG2B_AF_RX_LPF3K_DISABLE = BIT(9),
    BK4819_REG2B_AF_RX_DEEMPH_DISABLE = BIT(8),
    BK4819_REG2B_AF_TX_HPF300_DISABLE = BIT(2),
    BK4819_REG2B_AF_TX_LPF1_DISABLE = BIT(1),
    BK4819_REG2B_AF_TX_PREEMPH_DISABLE = BIT(0),
    /* Speech filters and emphasis, on transmit and receive */
    BK4819_REG2B_AF_FILTERS_DISABLE =
        BK4819_REG2B_AF_RX_HPF300_DISABLE | BK4819_REG2B_AF_RX_LPF3K_DISABLE
        | BK4819_REG2B_AF_RX_DEEMPH_DISABLE | BK4819_REG2B_AF_TX_HPF300_DISABLE
        | BK4819_REG2B_AF_TX_LPF1_DISABLE | BK4819_REG2B_AF_TX_PREEMPH_DISABLE,
};

/* REG_30: power and signal path enables */
enum {
    BK4819_REG30_VCO_CALIBRATION = BIT(15),
    BK4819_REG30_REVERSE1_ENABLE = BIT(14),
    BK4819_REG30_RX_LINK_ENABLE = BITV(0x0f, 10),
    BK4819_REG30_AF_DAC_ENABLE = BIT(9),
    BK4819_REG30_REVERSE2_ENABLE = BIT(8), ///< Discriminator mode.
    BK4819_REG30_PLL_VCO_ENABLE = BITV(0x0f, 4),
    BK4819_REG30_PA_GAIN_ENABLE = BIT(3),
    BK4819_REG30_MIC_ADC_ENABLE = BIT(2),
    BK4819_REG30_TX_DSP_ENABLE = BIT(1),
    BK4819_REG30_RX_DSP_ENABLE = BIT(0),
};

/* REG_31: audio processing */
enum {
    BK4819_REG31_COMPANDER = BIT(3),
    BK4819_REG31_VOX = BIT(2),
};

/* REG_32: frequency scan */
enum {
    BK4819_REG32_SCAN_TIME_MASK = 0xc000,
    BK4819_REG32_SCAN_ENABLE = BIT(0),
};

#define BK4819_REG32_SCAN_TIME(x) (BITV((x), 14) & BK4819_REG32_SCAN_TIME_MASK)

/* REG_40: TX deviation */
enum {
    BK4819_REG40_TX_DEVIATION_ENABLE = BIT(12),
    BK4819_REG40_TX_DEVIATION_MASK = 0x0fff,
};

#define BK4819_REG40_TX_DEVIATION(x) ((x) & BK4819_REG40_TX_DEVIATION_MASK)

/* REG_43: RF and AF TX filters, bandwidth mode */
enum {
    BK4819_REG43_RF_BW_MASK = 0x7000,
    BK4819_REG43_RF_BW_WEAK_MASK = 0x0e00,
    BK4819_REG43_AF_TX_LPF2_BW_MASK = 0x01c0,
    BK4819_REG43_BW_MODE_MASK = 0x0030,
};

#define BK4819_REG43_RF_BW(x) (BITV((x), 12) & BK4819_REG43_RF_BW_MASK)
#define BK4819_REG43_RF_BW_WEAK(x) (BITV((x), 9) & BK4819_REG43_RF_BW_WEAK_MASK)
#define BK4819_REG43_AF_TX_LPF2_BW(x) \
    (BITV((x), 6) & BK4819_REG43_AF_TX_LPF2_BW_MASK)
#define BK4819_REG43_BW_MODE(x) (BITV((x), 4) & BK4819_REG43_BW_MODE_MASK)

/* REG_47: AF output */
enum {
    BK4819_REG47_AF_TX_FILTER_BYPASS = BIT(0),
    /* Undocumented, "for gain" in the application note's RF_EnterBypass() */
    BK4819_REG47_UNDOC1 = BIT(1),
    BK4819_REG47_BYPASS = BK4819_REG47_AF_TX_FILTER_BYPASS
                        | BK4819_REG47_UNDOC1,
    BK4819_REG47_AF_OUTPUT_MASK = 0x0f00,
};

#define BK4819_REG47_AF_OUTPUT(x) (BITV((x), 8) & BK4819_REG47_AF_OUTPUT_MASK)

/* REG_48: AF RX gains */
enum {
    BK4819_REG48_AF_RX_GAIN1_MASK = 0x0c00,
    BK4819_REG48_AF_RX_GAIN2_MASK = 0x03f0,
};

#define BK4819_REG48_AF_RX_GAIN1(x) \
    (BITV((x), 10) & BK4819_REG48_AF_RX_GAIN1_MASK)
#define BK4819_REG48_AF_RX_GAIN2(x) \
    (BITV((x), 4) & BK4819_REG48_AF_RX_GAIN2_MASK)

/* REG_4B: ALC */
enum {
    BK4819_REG4B_ALC_DISABLE = BIT(5),
};

/* REG_51: CTCSS/CDCSS */
enum {
    BK4819_REG51_TX_CTCDSS_ENABLE = BIT(15),
    BK4819_REG51_GPIO6_CDCSS = BIT(14),
    BK4819_REG51_TRANSMIT_NEG_CDCSS_CODE = BIT(13),
    BK4819_REG51_CTCSCSS_MODE_SEL = BIT(12),
    BK4819_REG51_CDCSS_BIT_SEL = BIT(11),
    BK4819_REG51_1050HZ_DET_MOD = BIT(10),
    BK4819_REG51_AUTO_CDCSS_BW_MOD = BIT(9),
    BK4819_REG51_AUTO_CTCSS_BW_MOD = BIT(8),
    BK4819_REG51_CTCDCSS_TX_GAIN1T_MASK = 0x007f,
};

#define BK4819_REG51_CTCDCSS_TX_GAIN1T(x) \
    ((x) & BK4819_REG51_CTCDCSS_TX_GAIN1T_MASK)

/* REG_70: tone generator */
enum {
    BK4819_REG70_ENABLE_TONE1 = BIT(15),
    BK4819_REG70_TONE1_GAIN_MASK = 0x7f00,
    BK4819_REG70_ENABLE_TONE2 = BIT(7),
    BK4819_REG70_TONE2_GAIN_MASK = 0x007f,
};

#define BK4819_REG70_TONE1_GAIN(x) (BITV((x), 8) & BK4819_REG70_TONE1_GAIN_MASK)

/* REG_7D: microphone sensitivity */
enum {
    BK4819_REG7D_MIC_GAIN_MASK = 0x001f,
};

#define BK4819_REG7D_MIC_GAIN(x) ((x) & BK4819_REG7D_MIC_GAIN_MASK)

/* REG_7E: DC filters */
enum {
    BK4819_REG7E_TX_DCF_BW_MASK = 0x0038,
    BK4819_REG7E_RX_DCF_BW_MASK = 0x0007,
};

#define BK4819_REG7E_TX_DCF_BW(x) (BITV((x), 3) & BK4819_REG7E_TX_DCF_BW_MASK)
#define BK4819_REG7E_RX_DCF_BW(x) ((x) & BK4819_REG7E_RX_DCF_BW_MASK)

/* Status flags */
enum {
    BK4819_FLAG_DTMF_REV = BITV(0x8, 11),
    BK4819_FLAG_FSK_RX_SNF = BIT(7),
    BK4819_FLAG_FSK_RX_SPF = BIT(6),
    BK4819_FLAG_FSK_RX_CRC = BIT(4),
    BK4819_FLAG_CDCSS_PCR = BIT(14),
    BK4819_FLAG_CDCSS_PNR = BIT(15),
};

/* Field values set by bk4819_init() and bk4819_setBandwidth() */
enum {
    BK4819_DEFAULT_TX_DEVIATION = 0x4d2,
    BK4819_DEFAULT_RF_BW = 4,      ///< 3.75 kHz.
    BK4819_DEFAULT_RF_BW_WEAK = 0, ///< 1.7 kHz.
    BK4819_DEFAULT_TX_LPF2_BW = 1, ///< 2.5 kHz.
    BK4819_DEFAULT_RX_AUDIO_GAIN1 = 0,
    BK4819_DEFAULT_RX_AUDIO_GAIN2 = 0x3c,
    BK4819_DEFAULT_MIC_GAIN = 0x19,
    BK4819_DEFAULT_TX_DCF_BW = 5,
    BK4819_DEFAULT_RX_DCF_BW = 6,
    BK4819_WIDE_RF_BW = 3,      ///< 3 kHz, doubled in 25 kHz channel mode.
    BK4819_WIDE_TX_LPF2_BW = 0, ///< 3 kHz.
};

/*
 * Audio processing fields, with their values by enum bk4819_audioMode: voice
 * (the values bk4819_init() sets), then bypass, as the application note's
 * RF_EnterBypass() sets it.
 */
struct audioModeFields {
    uint8_t reg;       ///< Register address.
    uint16_t mask;     ///< Fields set by the audio mode.
    uint16_t value[2]; ///< Field values, by audio mode.
};

static const struct audioModeFields audioModeFields[] = {
    { .reg = 0x2B,
      .mask = BK4819_REG2B_AF_FILTERS_DISABLE,
      .value = { 0, BK4819_REG2B_AF_FILTERS_DISABLE } },
    { .reg = 0x31,
      .mask = BK4819_REG31_COMPANDER | BK4819_REG31_VOX,
      .value = { 0, 0 } },
    { .reg = 0x47,
      .mask = BK4819_REG47_BYPASS,
      .value = { 0, BK4819_REG47_BYPASS } },
    { .reg = 0x4B,
      .mask = BK4819_REG4B_ALC_DISABLE,
      .value = { 0, BK4819_REG4B_ALC_DISABLE } },
    { .reg = 0x70,
      .mask = BK4819_REG70_ENABLE_TONE1 | BK4819_REG70_ENABLE_TONE2,
      .value = { 0, 0 } },
    { .reg = 0x7E,
      .mask = BK4819_REG7E_TX_DCF_BW_MASK | BK4819_REG7E_RX_DCF_BW_MASK,
      .value = { BK4819_REG7E_TX_DCF_BW(BK4819_DEFAULT_TX_DCF_BW)
                     | BK4819_REG7E_RX_DCF_BW(BK4819_DEFAULT_RX_DCF_BW),
                 0 } },
};

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
    const uint8_t tx[2] = { (data >> 8) & 0xFF, data & 0xFF };

    spi_acquire(dev->spi);
    gpioPin_clear(&dev->scn);
    delayUs(1);

    spi_send(dev->spi, &cmd, 1);
    spi_send(dev->spi, tx, sizeof(tx));

    delayUs(1);
    gpioPin_set(&dev->scn);
    spi_release(dev->spi);
}

/**
 * Write the bits of a register selected by a mask.
 *
 * @param dev: pointer to device data.
 * @param reg: register address.
 * @param mask: bits to write.
 * @param value: new value of those bits.
 */
static void maskSetRegister(const struct BK4819 *dev, uint8_t reg,
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

uint16_t bk4819_getInterrupt(const struct BK4819 *dev,
                             enum bk4819_interrupt interrupt)
{
    if ((bk4819_readReg(dev, 0x0C) & BK4819_REG0C_INTERRUPT) == 0)
        return 0;

    /*
     * Latch and read the interrupt status, as the UV-K5 firmware does:
     * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/app/app.c#L509-L513
     */
    bk4819_writeReg(dev, 0x02, 0);
    return bk4819_readReg(dev, 0x02) & interrupt;
}

void bk4819_enableInterrupt(const struct BK4819 *dev,
                            enum bk4819_interrupt interrupt)
{
    bk4819_writeReg(dev, 0x3F, bk4819_readReg(dev, 0x3F) | interrupt);
}

void bk4819_disableInterrupt(const struct BK4819 *dev,
                             enum bk4819_interrupt interrupt)
{
    bk4819_writeReg(dev, 0x3F, bk4819_readReg(dev, 0x3F) & (~interrupt));
}

void bk4819_setFrequency(const struct BK4819 *dev, uint32_t freq)
{
    freq = freq / 10; /* Convert to 10 Hz units */
    bk4819_writeReg(dev, 0x39, (freq >> 16) & 0xFFFF);
    bk4819_writeReg(dev, 0x38, freq & 0xFFFF);
    bk4819_enableRx(dev);
}

void bk4819_enableRx(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x37, 0x1F0F);
    delayUs(1);
    bk4819_writeReg(dev, 0x30, BK4819_REG30_AF_DAC_ENABLE);
    bk4819_writeReg(dev, 0x30, 0xBFF1);
}

void bk4819_enableTx(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x30, 0x00); /* reset */
    bk4819_writeReg(
        dev, 0x30,
        BK4819_REG30_REVERSE1_ENABLE | BK4819_REG30_REVERSE2_ENABLE
            | BK4819_REG30_VCO_CALIBRATION | BK4819_REG30_MIC_ADC_ENABLE
            | BK4819_REG30_TX_DSP_ENABLE | BK4819_REG30_PLL_VCO_ENABLE
            | BK4819_REG30_PA_GAIN_ENABLE);
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
    /* Preserve polarity and TX filter bypass when muting/unmuting RX. */
    uint16_t value = bk4819_readReg(dev, 0x47);
    bk4819_writeReg(dev, 0x47,
                    (value & ~BK4819_REG47_AF_OUTPUT_MASK)
                        | BK4819_REG47_AF_OUTPUT(af));
}

/**
 * Convert a CTCSS tone to the REG_07 frequency word, for a 26 MHz crystal.
 *
 * @param freq: tone frequency, in 0.1 Hz.
 * @return REG_07 frequency word.
 */
static uint16_t ctcssWord(uint16_t freq)
{
    // 2064888 / 1000000 reduced by 8, so that the product fits 32 bits
    return ((uint32_t)freq * 258111) / 125000;
}

void bk4819_enableTxCtcss(const struct BK4819 *dev, uint16_t freq, uint8_t gain)
{
    uint16_t reg = bk4819_readReg(dev, 0x51);
    reg &= ~BK4819_REG51_CTCDCSS_TX_GAIN1T_MASK;
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE | BK4819_REG51_CTCSCSS_MODE_SEL
         | BK4819_REG51_CTCDCSS_TX_GAIN1T(gain);
    bk4819_writeReg(dev, 0x51, reg);
    bk4819_writeReg(dev, 0x07, ctcssWord(freq));
}

void bk4819_enableRxCtcss(const struct BK4819 *dev, uint16_t freq)
{
    uint16_t reg = bk4819_readReg(dev, 0x51);
    reg |= BK4819_REG51_CTCSCSS_MODE_SEL;
    bk4819_writeReg(dev, 0x51, reg);
    bk4819_writeReg(dev, 0x07, ctcssWord(freq));
}

void bk4819_enableCtcss2(const struct BK4819 *dev, uint16_t freq)
{
    const uint16_t enable = BK4819_REG51_TX_CTCDSS_ENABLE
                          | BK4819_REG51_CTCSCSS_MODE_SEL;

    maskSetRegister(dev, 0x51, enable, enable);
    bk4819_writeReg(dev, 0x07, freq | BK4819_REG07_MODE_CTC2);
}

void bk4819_enableTxCdcss(const struct BK4819 *dev, uint8_t negative,
                          uint8_t bits24, uint32_t code, uint8_t gain)
{
    bk4819_writeReg(dev, 0x51,
                    BK4819_REG51_TX_CTCDSS_ENABLE | BITV(negative, 13)
                        | BITV(bits24, 11)
                        | BK4819_REG51_CTCDCSS_TX_GAIN1T(gain));
    bk4819_writeReg(dev, 0x07, BK4819_REG07_MODE_CDCSS | 0x0AD7);
    bk4819_writeReg(dev, 0x08, BIT(15) | ((code >> 12) & 0xFFF));
    bk4819_writeReg(dev, 0x08, code & 0xFFF);
}

void bk4819_disableCtdcss(const struct BK4819 *dev)
{
    uint16_t reg = bk4819_readReg(dev, 0x51);
    reg &= ~BK4819_REG51_TX_CTCDSS_ENABLE;
    bk4819_writeReg(dev, 0x51, reg);
}

uint16_t bk4819_getCtcss(const struct BK4819 *dev)
{
    return bk4819_readReg(dev, 0x0C) & BIT(10);
}

void bk4819_enableVox(const struct BK4819 *dev, uint8_t delay, uint8_t interval,
                      uint16_t thresholdOn, uint16_t thresholdOff)
{
    bk4819_writeReg(dev, 0x31, bk4819_readReg(dev, 0x31) | BK4819_REG31_VOX);
    bk4819_writeReg(dev, 0x79, BITV(interval, 10) | thresholdOff);
    bk4819_writeReg(dev, 0x46, thresholdOn);
}

uint8_t bk4819_getVox(const struct BK4819 *dev)
{
    return bk4819_readReg(dev, 0x0C) & BIT(2);
}

void bk4819_setSquelch(const struct BK4819 *dev, uint8_t rssiOpen,
                       uint8_t rssiClose, uint8_t noiseOpen, uint8_t noiseClose,
                       uint8_t glitchOpen, uint8_t glitchClose)
{
    bk4819_writeReg(dev, 0x78, (rssiOpen << 8) | rssiClose);

    // Only change threshold fields; preserve timing and reserved bits.
    uint16_t value = bk4819_readReg(dev, 0x4F);
    bk4819_writeReg(dev, 0x4F,
                    (value & 0x8080) | ((noiseClose & 0x7f) << 8)
                        | (noiseOpen & 0x7f));
    value = bk4819_readReg(dev, 0x4D);
    bk4819_writeReg(dev, 0x4D, (value & 0xff00) | glitchClose);
    value = bk4819_readReg(dev, 0x4E);
    bk4819_writeReg(dev, 0x4E, (value & 0xff00) | glitchOpen);
}

void bk4819_setAudioMode(const struct BK4819 *dev, enum bk4819_audioMode mode)
{
    const bool bypass = (mode == BK4819_AUDIO_BYPASS);

    for (size_t i = 0; i < ARRAY_SIZE(audioModeFields); i++) {
        const struct audioModeFields *field = &audioModeFields[i];
        maskSetRegister(dev, field->reg, field->mask, field->value[bypass]);
    }

    if (bypass)
        return;

    bk4819_setTxDeviation(dev, true, BK4819_DEFAULT_TX_DEVIATION);
    bk4819_setRfFilter(dev, BK4819_DEFAULT_RF_BW, BK4819_DEFAULT_RF_BW_WEAK);
    bk4819_setTxAudioFilter(dev, BK4819_DEFAULT_TX_LPF2_BW);
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

    maskSetRegister(dev, 0x40, mask, value);
}

void bk4819_setRfFilter(const struct BK4819 *dev, uint8_t bw, uint8_t weakBw)
{
    const uint16_t mask = BK4819_REG43_RF_BW_MASK
                        | BK4819_REG43_RF_BW_WEAK_MASK;
    const uint16_t value = BK4819_REG43_RF_BW(bw)
                         | BK4819_REG43_RF_BW_WEAK(weakBw);

    maskSetRegister(dev, 0x43, mask, value);
}

void bk4819_setTxAudioFilter(const struct BK4819 *dev, uint8_t bw)
{
    maskSetRegister(dev, 0x43, BK4819_REG43_AF_TX_LPF2_BW_MASK,
                    BK4819_REG43_AF_TX_LPF2_BW(bw));
}

void bk4819_setBandwidth(const struct BK4819 *dev, enum bk4819_bandwidth bw)
{
    /*
     * 0x4048 and 0x3028, the values of the stock C62 firmware and of the
     * UV-K5 firmware's BK4819_SetFilterBandwidth():
     * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819.c#L292-L299
     * 12.5 kHz is the reset value; 25 kHz has the wide RF and AF TX filters,
     * per the REG_43 notes in
     * https://github.com/egzumer/uv-k5-firmware-custom/blob/7607f0a4bd6203d1f06b70556fc1ce0d7399d6b3/driver/bk4819.c#L562-L609
     */
    const uint16_t mask = BK4819_REG43_RF_BW_MASK | BK4819_REG43_RF_BW_WEAK_MASK
                        | BK4819_REG43_AF_TX_LPF2_BW_MASK
                        | BK4819_REG43_BW_MODE_MASK;
    uint16_t value = BK4819_REG43_RF_BW_WEAK(BK4819_DEFAULT_RF_BW_WEAK)
                   | BK4819_REG43_BW_MODE(bw);

    if (bw == BK4819_BW_25) {
        value |= BK4819_REG43_RF_BW(BK4819_WIDE_RF_BW)
               | BK4819_REG43_AF_TX_LPF2_BW(BK4819_WIDE_TX_LPF2_BW);
    } else {
        value |= BK4819_REG43_RF_BW(BK4819_DEFAULT_RF_BW)
               | BK4819_REG43_AF_TX_LPF2_BW(BK4819_DEFAULT_TX_LPF2_BW);
    }

    maskSetRegister(dev, 0x43, mask, value);
}

void bk4819_setRxAudioGain(const struct BK4819 *dev, uint8_t gain1,
                           uint8_t gain2)
{
    const uint16_t mask = BK4819_REG48_AF_RX_GAIN1_MASK
                        | BK4819_REG48_AF_RX_GAIN2_MASK;
    const uint16_t value = BK4819_REG48_AF_RX_GAIN1(gain1)
                         | BK4819_REG48_AF_RX_GAIN2(gain2);

    maskSetRegister(dev, 0x48, mask, value);
}

void bk4819_setMicGain(const struct BK4819 *dev, uint8_t gain)
{
    maskSetRegister(dev, 0x7D, BK4819_REG7D_MIC_GAIN_MASK,
                    BK4819_REG7D_MIC_GAIN(gain));
}

int16_t bk4819_getRssi(const struct BK4819 *dev)
{
    return ((bk4819_readReg(dev, 0x67) & 0x01FF) / 2) - 160;
}

uint16_t bk4819_getMicLevel(const struct BK4819 *dev)
{
    /* bits 6:0 of AF TX/RX Input Amplitude */
    return (bk4819_readReg(dev, 0x6F) & 0x7f) * 2;
}

void bk4819_enableFreqScan(const struct BK4819 *dev, enum bk4819_scanTime time)
{
    const uint16_t mask = BK4819_REG32_SCAN_TIME_MASK
                        | BK4819_REG32_SCAN_ENABLE;

    maskSetRegister(dev, 0x32, mask,
                    BK4819_REG32_SCAN_TIME(time) | BK4819_REG32_SCAN_ENABLE);
}

void bk4819_disableFreqScan(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x32,
                    bk4819_readReg(dev, 0x32) & ~BK4819_REG32_SCAN_ENABLE);
}

bool bk4819_getScanFreqFlag(const struct BK4819 *dev)
{
    return (bk4819_readReg(dev, 0x0D) & BK4819_REG0D_SCAN_BUSY) == 0;
}

uint32_t bk4819_getScanFreq(const struct BK4819 *dev)
{
    /*
     * In 10 Hz, as the UV-K5 firmware reads it:
     * https://github.com/DualTachyon/uv-k5-firmware/blob/6f8afac8864e0349ecf8b10f91ce8f69273013db/driver/bk4819.c#L746-L758
     */
    uint32_t high = bk4819_readReg(dev, 0x0D) & BK4819_REG0D_SCAN_FREQ_MASK;
    uint32_t low = bk4819_readReg(dev, 0x0E);

    return ((high << 16) | low) * 10;
}

/**
 * Convert a beep frequency to the tone generator's units.
 *
 * @param freq: frequency, in Hz.
 * @return REG_71 value.
 */
static uint16_t scaleFreq(const uint16_t freq)
{
    return (((uint32_t)freq * 1048576u) + 50000u) / 100000u; /* with rounding */
}

void bk4819_beepStart(const struct BK4819 *dev, uint16_t freq, bool tuningGain)
{
    (void)tuningGain;
    bk4819_writeReg(dev, 0x50, 0x3B20);
    bk4819_setAfOutput(dev, BK4819_AF_BEEP);
    bk4819_writeReg(dev, 0x70,
                    BK4819_REG70_ENABLE_TONE1 | BK4819_REG70_TONE1_GAIN(96));

    bk4819_writeReg(dev, 0x71, scaleFreq(freq));
    bk4819_writeReg(dev, 0x30, 0);
    bk4819_writeReg(dev, 0x30,
                    BK4819_REG30_AF_DAC_ENABLE | BK4819_REG30_REVERSE2_ENABLE
                        | BK4819_REG30_TX_DSP_ENABLE);
    bk4819_writeReg(dev, 0x50, 0x3B20);
}

void bk4819_beepStop(const struct BK4819 *dev)
{
    bk4819_writeReg(dev, 0x50, 0xBB20);
}
