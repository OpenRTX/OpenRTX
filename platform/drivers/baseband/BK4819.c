/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/BK4819.h"
#include "interfaces/delays.h"
#include "core/utils.h"

uint16_t BK4819_readReg(const struct BK4819 *dev, uint8_t reg)
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

void BK4819_writeReg(const struct BK4819 *dev, bk4819_reg_t reg, uint16_t data)
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

void bk4819_init(const struct BK4819 *dev)
{
    /* Configure CS as output, idle high */
    gpioPin_setMode(&dev->scn, OUTPUT);
    gpioPin_set(&dev->scn);

    uint16_t uVar1;
    BK4819_writeReg(dev, BK4819_REG_00, BIT(15)); // Soft Reset
    BK4819_writeReg(dev, BK4819_REG_00, 0);       // Normal operation
    BK4819_writeReg(dev, BK4819_REG_37, 0x1d0f);  // Power mode

    /* Rx AGC Gain */
    BK4819_writeReg(dev, BK4819_REG_13, 0x3be);
    BK4819_writeReg(dev, BK4819_REG_12, 0x37b);
    BK4819_writeReg(dev, BK4819_REG_11, 0x27b);
    BK4819_writeReg(dev, BK4819_REG_10, 0x7a);
    BK4819_writeReg(dev, BK4819_REG_14, 0x19);

    BK4819_writeReg(dev, BK4819_REG_49, 0x2a38);
    BK4819_writeReg(dev, BK4819_REG_7B, 0x8420);
    BK4819_writeReg(dev, BK4819_REG_7D, 0xe940);
    bk4819_setMicGain(dev, BK4819_DEFAULT_MIC_GAIN);
    BK4819_writeReg(dev, BK4819_REG_48, 0xb001); // AF DAC gain
    bk4819_setAfRxGain(dev, BK4819_DEFAULT_AF_RX_GAIN1,
                       BK4819_DEFAULT_AF_RX_GAIN2);
    bk4819_setRxFilter(dev, BK4819_DEFAULT_RX_FILTER);
    BK4819_writeReg(dev, BK4819_REG_1E, 0x4c58);
    BK4819_writeReg(dev, BK4819_REG_1F, 0xa656);
    BK4819_writeReg(dev, BK4819_REG_3E, 0xa037); // Band selection threshold
    BK4819_writeReg(dev, BK4819_REG_3F, 0x07fe); // Interrupt enable
    BK4819_writeReg(dev, BK4819_REG_2A, 0x7fff);
    BK4819_writeReg(dev, BK4819_REG_28, 0x6b00); // Expander
    BK4819_writeReg(dev, BK4819_REG_53, 59000);
    BK4819_writeReg(dev, BK4819_REG_2C, 0x5705);
    BK4819_writeReg(dev, BK4819_REG_4B, 0x7102); // ALC
    bk4819_setTxDeviation(dev, false, BK4819_DEFAULT_TX_DEVIATION);
    BK4819_writeReg(dev, BK4819_REG_77, 0x88ef);
    BK4819_writeReg(dev, BK4819_REG_26, 0x13a0);
    BK4819_writeReg(dev, BK4819_REG_4E, 0x6f15); // Squelch
    BK4819_writeReg(dev, BK4819_REG_4F,
                    0x3f3e); // Ex-noise threhold for Squelch

    /* DTMF Symbol 0 - 15 Coefficient */
    BK4819_writeReg(dev, BK4819_REG_09, 0x006f);
    BK4819_writeReg(dev, BK4819_REG_09, 0x106b);
    BK4819_writeReg(dev, BK4819_REG_09, 0x2067);
    BK4819_writeReg(dev, BK4819_REG_09, 0x3062);
    BK4819_writeReg(dev, BK4819_REG_09, 0x4050);
    BK4819_writeReg(dev, BK4819_REG_09, 0x5047);
    BK4819_writeReg(dev, BK4819_REG_09, 0x603a);
    BK4819_writeReg(dev, BK4819_REG_09, 0x702c);
    BK4819_writeReg(dev, BK4819_REG_09, 0x8041);
    BK4819_writeReg(dev, BK4819_REG_09, 0x9037);
    BK4819_writeReg(dev, BK4819_REG_09, 0xa025);
    BK4819_writeReg(dev, BK4819_REG_09, 0xb017);
    BK4819_writeReg(dev, BK4819_REG_09, 0xc0e4);
    BK4819_writeReg(dev, BK4819_REG_09, 0xd0cb);
    BK4819_writeReg(dev, BK4819_REG_09, 0xe0b5);
    BK4819_writeReg(dev, BK4819_REG_09, 0xf09f);

    BK4819_writeReg(dev, BK4819_REG_74,
                    0xfa02); // 3000Hz AF Response coefficient for Tx
    BK4819_writeReg(dev, BK4819_REG_44,
                    0x8f88); // 300Hz AF Response coefficient for Tx
    BK4819_writeReg(dev, BK4819_REG_45,
                    0x3201); // 300Hz AF Response coefficient for Rx
    uVar1 = BK4819_readReg(dev, BK4819_REG_31);
    BK4819_writeReg(dev, BK4819_REG_31,
                    uVar1 & 0xfffffff7);         // Disable Compander Function
    BK4819_writeReg(dev, BK4819_REG_28, 0x6b38); // Expander
    BK4819_writeReg(dev, BK4819_REG_29, 0xb4cb); // Compress
    BK4819_writeReg(
        dev, BK4819_REG_36,
        0xdfbf); // Tx Output power Bias 0xdf = 2,8V PA Tuning 0xbf = Enable PACTKoutput + 7,26dBm
    BK4819_writeReg(dev, BK4819_REG_47, 0x6040); // AF

    /*
     * Let the microphone input bias settle with only the mic ADC enabled,
     * before any audio starts: otherwise the mic AGC can stay at a low gain.
     */
    uint16_t power = BK4819_readReg(dev, BK4819_REG_30);
    BK4819_writeReg(dev, BK4819_REG_30, BK4819_REG30_MIC_ADC_ENABLE);
    delayMs(250);
    BK4819_writeReg(dev, BK4819_REG_30, power);
}

uint8_t bk4819_int_get(const struct BK4819 *dev, bk4819_int_t interrupt)
{
    if ((BK4819_readReg(dev, BK4819_REG_0C) & BIT(0x01)) == 0)
        return 0;
    return BK4819_readReg(dev, BK4819_REG_02 & interrupt);
}

void bk4819_int_enable(const struct BK4819 *dev, bk4819_int_t interrupt)
{
    BK4819_writeReg(dev, BK4819_REG_3F,
                    BK4819_readReg(dev, BK4819_REG_3F) | interrupt);
}

void bk4819_int_disable(const struct BK4819 *dev, bk4819_int_t interrupt)
{
    BK4819_writeReg(dev, BK4819_REG_3F,
                    BK4819_readReg(dev, BK4819_REG_3F) & (~interrupt));
}

void bk4819_set_freq(const struct BK4819 *dev, uint32_t freq)
{
    freq = freq / 10; /* Convert to 10 Hz units */
    BK4819_writeReg(dev, BK4819_REG_39, (freq >> 16) & 0xFFFF);
    BK4819_writeReg(dev, BK4819_REG_38, freq & 0xFFFF);
    bk4819_rx_on(dev);
}

void bk4819_rx_on(const struct BK4819 *dev)
{
    BK4819_writeReg(dev, BK4819_REG_37, 0x1F0F);
    delayUs(1);
    BK4819_writeReg(dev, BK4819_REG_30, BK4819_REG30_AF_DAC_ENABLE);
    BK4819_writeReg(dev, BK4819_REG_30, 0xBFF1);
}

void bk4819_set_modulation(const struct BK4819 *dev, bool is_FM)
{
    BK4819_SetAF(dev, is_FM ? BK4819_AF_TYPE_FM : BK4819_AF_TYPE_AM);
}

void bk4819_tx_on(const struct BK4819 *dev)
{
    BK4819_writeReg(dev, BK4819_REG_30, 0x00); /* reset */
    BK4819_writeReg(
        dev, BK4819_REG_30,
        BK4819_REG30_REVERSE1_ENABLE | BK4819_REG30_REVERSE2_ENABLE
            | BK4819_REG30_VCO_CALIBRATION | BK4819_REG30_MIC_ADC_ENABLE
            | BK4819_REG30_TX_DSP_ENABLE | BK4819_REG30_PLL_VCO_ENABLE
            | BK4819_REG30_PA_GAIN_ENABLE);
}

void bk4819_rtx_off(const struct BK4819 *dev)
{
    BK4819_writeReg(dev, BK4819_REG_30, 0x00); /* reset */
    BK4819_writeReg(dev, BK4819_REG_30, BK4819_REG_30_ENABLE_AF_DAC);
}

void bk4819_SetFilterBandwidth(const struct BK4819 *dev, uint8_t bandwidth)
{
    uint16_t Value = BK4819_readReg(dev, BK4819_REG_43);
    if (bandwidth) { /* 25kHz */
        BK4819_writeReg(dev, BK4819_REG_43, (Value & ~0x30) | 32);
    } else {         /* 12.5kHz */
        BK4819_writeReg(dev, BK4819_REG_43, (Value & ~0x30) | 0);
    }
}

void bk4819_enable_tx_ctcss(const struct BK4819 *dev, uint16_t frequency)
{
    /* frequency is in .1 Hz units */
    uint32_t ctcss_reg_value = frequency * 2064888 / 100000
                             / 10; /* Register value for 26MHz XTAL */

    uint16_t reg = BK4819_readReg(dev, BK4819_REG_51);
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE | BK4819_REG51_CTCSCSS_MODE_SEL;
    BK4819_writeReg(dev, BK4819_REG_51, reg);
    BK4819_writeReg(dev, BK4819_REG_07, (uint16_t)ctcss_reg_value);
}

void bk4819_enable_rx_ctcss(const struct BK4819 *dev, uint16_t frequency)
{
    /* frequency is in .1 Hz units */
    uint32_t ctcss_reg_value = frequency * 2064888 / 100000
                             / 10; /* Register value for 26MHz XTAL */

    uint16_t reg = BK4819_readReg(dev, BK4819_REG_51);
    reg |= BK4819_REG51_CTCSCSS_MODE_SEL;
    BK4819_writeReg(dev, BK4819_REG_51, reg);
    BK4819_writeReg(dev, BK4819_REG_07, (uint16_t)ctcss_reg_value);
}

void bk4819_enable_ctcss2(const struct BK4819 *dev, uint16_t frequency)
{
    uint16_t reg = BK4819_readReg(dev, BK4819_REG_51);
    reg |= BK4819_REG51_TX_CTCDSS_ENABLE | BK4819_REG51_CTCSCSS_MODE_SEL;
    BK4819_writeReg(dev, BK4819_REG_07, frequency | BIT(13));
}

void bk4819_enable_tx_cdcss(const struct BK4819 *dev, uint8_t code_type,
                            uint8_t bit_sel, uint32_t cdcss_code)
{
    BK4819_writeReg(dev, BK4819_REG_51,
                    BK4819_REG51_TX_CTCDSS_ENABLE | BITV(code_type, 13)
                        | BITV(bit_sel, 11));
    BK4819_writeReg(dev, BK4819_REG_07, BITV(2, 13) | 0x0AD7);
    BK4819_writeReg(dev, BK4819_REG_08, BIT(15) | ((cdcss_code >> 12) & 0XFFF));
    BK4819_writeReg(dev, BK4819_REG_08, cdcss_code & 0XFFF);
}

void bk4819_disable_ctdcss(const struct BK4819 *dev)
{
    uint16_t reg = BK4819_readReg(dev, BK4819_REG_51);
    reg &= ~BK4819_REG51_TX_CTCDSS_ENABLE;
    BK4819_writeReg(dev, BK4819_REG_51, reg);
}

uint16_t bk4819_get_ctcss(const struct BK4819 *dev)
{
    return BK4819_readReg(dev, BK4819_REG_0C) & BIT(10);
}

void bk4819_enable_vox(const struct BK4819 *dev, uint8_t delay_time,
                       uint8_t interval_time, uint16_t threshold_on,
                       uint16_t threshold_off)
{
    BK4819_writeReg(dev, BK4819_REG_31,
                    BK4819_readReg(dev, BK4819_REG_31) | BIT(2));
    BK4819_writeReg(dev, BK4819_REG_79,
                    BITV(interval_time, 10) | threshold_off);
    BK4819_writeReg(dev, BK4819_REG_46, threshold_on);
}

uint8_t bk4819_get_vox(const struct BK4819 *dev)
{
    return BK4819_readReg(dev, BK4819_REG_0C) & BIT(2);
}

void bk4819_set_Squelch(const struct BK4819 *dev, uint8_t RTSO, uint8_t RTSC,
                        uint8_t ETSO, uint8_t ETSC, uint8_t GTSO, uint8_t GTSC)
{
    BK4819_writeReg(dev, BK4819_REG_78, (RTSO << 8) | RTSC);

    // Only change threshold fields; preserve timing and reserved bits.
    uint16_t value = BK4819_readReg(dev, BK4819_REG_4F);
    BK4819_writeReg(dev, BK4819_REG_4F,
                    (value & 0x8080) | ((ETSC & 0x7f) << 8) | (ETSO & 0x7f));
    value = BK4819_readReg(dev, BK4819_REG_4D);
    BK4819_writeReg(dev, BK4819_REG_4D, (value & 0xff00) | GTSC);
    value = BK4819_readReg(dev, BK4819_REG_4E);
    BK4819_writeReg(dev, BK4819_REG_4E, (value & 0xff00) | GTSO);
}

int16_t bk4819_get_rssi(const struct BK4819 *dev)
{
    return ((BK4819_readReg(dev, BK4819_REG_67) & 0x01FF) / 2) - 160;
}

uint16_t bk4819_get_mic_level(const struct BK4819 *dev)
{
    /* bits 6:0 of AF TX/RX Input Amplitude */
    return (BK4819_readReg(dev, 0x6f) & 0x7f) * 2;
}

void bk4819_enable_freq_scan(const struct BK4819 *dev, uint8_t scna_time)
{
    BK4819_writeReg(dev, BK4819_REG_32,
                    BK4819_readReg(dev, BK4819_REG_32) | BITV(scna_time, 14));
    BK4819_writeReg(dev, BK4819_REG_32,
                    BK4819_readReg(dev, BK4819_REG_32) | 0x01);
}

void bk4819_disable_freq_scan(const struct BK4819 *dev)
{
    BK4819_writeReg(dev, BK4819_REG_32,
                    BK4819_readReg(dev, BK4819_REG_32) & (~0x01));
}

uint8_t bk4819_get_scan_freq_flag(const struct BK4819 *dev)
{
    return BK4819_readReg(dev, BK4819_REG_0D) & BIT(15);
}

uint32_t bk4819_get_scan_freq(const struct BK4819 *dev)
{
    return ((BK4819_readReg(dev, BK4819_REG_0D) << 16)
            | BK4819_readReg(dev, BK4819_REG_0E))
         / 10;
}

void BK4819_SetAF(const struct BK4819 *dev, uint8_t AF)
{
    /* Preserve polarity and TX filter bypass when muting/unmuting RX. */
    uint16_t value = BK4819_readReg(dev, BK4819_REG_47);
    BK4819_writeReg(dev, BK4819_REG_47,
                    (value & ~0x0f00u) | ((AF & 0x0fu) << 8));
}

/* Flat audio fields: flat values and the values bk4819_init() leaves */
static const struct {
    bk4819_reg_t reg;
    uint16_t mask;
    uint16_t flat;
    uint16_t fm;
} flatAudioFields[] = {
    { BK4819_REG_2B, 0x0707, 0x0707, 0x0000 }, /* Speech filters, emphasis */
    { BK4819_REG_31, 0x000c, 0x0000, 0x0000 }, /* Compander and VOX */
    { BK4819_REG_47, 0x0001, 0x0001, 0x0000 }, /* TX filter bypass */
    { BK4819_REG_4B, 0x0020, 0x0020, 0x0000 }, /* ALC disable */
    { BK4819_REG_70, 0x8080, 0x0000, 0x0000 }, /* Tone generators */
    { BK4819_REG_7E, 0x003f, 0x0000, 0x002e }, /* TX and RX DC filters */
};

/**
 * Write the bits of a register selected by a mask.
 *
 * @param dev: pointer to device data.
 * @param reg: register.
 * @param mask: bits to write.
 * @param value: new value of those bits.
 */
static void writeField(const struct BK4819 *dev, bk4819_reg_t reg,
                       uint16_t mask, uint16_t value)
{
    uint16_t current = BK4819_readReg(dev, reg);
    BK4819_writeReg(dev, reg, (current & ~mask) | (value & mask));
}

void bk4819_setFlatAudio(const struct BK4819 *dev, bool enable)
{
    for (size_t i = 0; i < ARRAY_SIZE(flatAudioFields); i++) {
        writeField(dev, flatAudioFields[i].reg, flatAudioFields[i].mask,
                   enable ? flatAudioFields[i].flat : flatAudioFields[i].fm);
    }
}

void bk4819_setTxDeviation(const struct BK4819 *dev, bool enable,
                           uint16_t tuning)
{
    writeField(dev, BK4819_REG_40,
               BK4819_REG40_TX_DEVIATION_ENABLE | BK4819_REG40_TX_DEVIATION(~0),
               (enable ? BK4819_REG40_TX_DEVIATION_ENABLE : 0)
                   | BK4819_REG40_TX_DEVIATION(tuning));
}

void bk4819_setRxFilter(const struct BK4819 *dev, uint16_t filter)
{
    writeField(dev, BK4819_REG_43, BK4819_REG43_FILTER_MASK, filter);
}

void bk4819_setAfUndoc1(const struct BK4819 *dev, bool enable)
{
    writeField(dev, BK4819_REG_47, BK4819_REG47_UNDOC1,
               enable ? BK4819_REG47_UNDOC1 : 0);
}

void bk4819_setAfRxGain(const struct BK4819 *dev, uint8_t gain1, uint8_t gain2)
{
    writeField(dev, BK4819_REG_48,
               BK4819_REG48_AF_RX_GAIN1(~0) | BK4819_REG48_AF_RX_GAIN2(~0),
               BK4819_REG48_AF_RX_GAIN1(gain1)
                   | BK4819_REG48_AF_RX_GAIN2(gain2));
}

void bk4819_setMicGain(const struct BK4819 *dev, uint8_t gain)
{
    writeField(dev, BK4819_REG_7D, BK4819_REG7D_MIC_GAIN(~0),
               BK4819_REG7D_MIC_GAIN(gain));
}

void bk4819_setAgcFixed(const struct BK4819 *dev, bool fixed)
{
    writeField(dev, BK4819_REG_7E, BK4819_REG7E_AGC_FIX,
               fixed ? BK4819_REG7E_AGC_FIX : 0);
}

__inline uint16_t scale_freq(const uint16_t freq)
{
    return (((uint32_t)freq * 1048576u) + 50000u) / 100000u; /* with rounding */
}

void BK4819_BeepStart(const struct BK4819 *dev, uint16_t Frequency,
                      bool bTuningGainSwitch)
{
    (void)bTuningGainSwitch;
    BK4819_writeReg(dev, BK4819_REG_50, 0x3B20);
    BK4819_SetAF(dev, 3); /* AF Beep */
    uint16_t ToneConfig = BK4819_REG_70_ENABLE_TONE1;
    ToneConfig |= 96u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
    BK4819_writeReg(dev, BK4819_REG_70, ToneConfig);

    BK4819_writeReg(dev, BK4819_REG_71, scale_freq(Frequency));
    BK4819_writeReg(dev, BK4819_REG_30, 0);
    BK4819_writeReg(dev, BK4819_REG_30,
                    BK4819_REG_30_ENABLE_AF_DAC | BK4819_REG_30_ENABLE_DISC_MODE
                        | BK4819_REG_30_ENABLE_TX_DSP);
    BK4819_writeReg(dev, BK4819_REG_50, 0x3B20);
}

void BK4819_BeepStop(const struct BK4819 *dev)
{
    BK4819_writeReg(dev, BK4819_REG_50, 0xBB20);
}
