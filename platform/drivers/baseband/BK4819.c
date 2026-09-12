/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/baseband/BK4819.h"
#include "interfaces/delays.h"

static void spi_write_byte(const struct BK4819 *dev, uint8_t data)
{
    gpioPin_clear(&dev->sck);
    gpioPin_setMode(&dev->sda, OUTPUT);
    for (uint8_t i = 0; i < 8; i++) {
        if (data & 0x80)
            gpioPin_set(&dev->sda);
        else
            gpioPin_clear(&dev->sda);
        gpioPin_set(&dev->sck);
        delayUs(1);
        gpioPin_clear(&dev->sck);
        delayUs(1);
        data <<= 1;
    }
}

static void spi_write_half_word(const struct BK4819 *dev, uint16_t data)
{
    spi_write_byte(dev, (data >> 8) & 0xFF);
    spi_write_byte(dev, data & 0xFF);
}

static uint16_t spi_read_half_word(const struct BK4819 *dev)
{
    uint16_t data = 0;
    gpioPin_setMode(&dev->sda, INPUT);
    gpioPin_clear(&dev->sck);
    for (uint8_t i = 0; i < 16; i++) {
        data <<= 1;
        gpioPin_clear(&dev->sck);
        delayUs(1);
        gpioPin_set(&dev->sck);
        delayUs(1);
        data |= gpioPin_read(&dev->sda) ? 1 : 0;
    }
    return data;
}

uint16_t BK4819_readReg(const struct BK4819 *dev, uint8_t reg)
{
    uint16_t data;
    gpioPin_clear(&dev->scn);
    delayUs(1);

    spi_write_byte(dev, reg | BK4819_REG_READ);
    data = spi_read_half_word(dev);

    delayUs(1);
    gpioPin_set(&dev->scn);
    return data;
}

void BK4819_writeReg(const struct BK4819 *dev, bk4819_reg_t reg, uint16_t data)
{
    gpioPin_clear(&dev->scn);
    delayUs(1);

    spi_write_byte(dev, reg | BK4819_REG_WRITE);
    spi_write_half_word(dev, data);

    delayUs(1);
    gpioPin_set(&dev->scn);
}

void bk4819_init(const struct BK4819 *dev)
{
    gpioPin_setMode(&dev->sda, OUTPUT);
    gpioPin_set(&dev->sda);
    /* Configure CS and CLK as outputs, CS idle high */
    gpioPin_setMode(&dev->scn, OUTPUT);
    gpioPin_set(&dev->scn);
    gpioPin_setMode(&dev->sck, OUTPUT);

    uint16_t uVar1;
    BK4819_writeReg(dev, 0, 0x8000);
    BK4819_writeReg(dev, 0, 0);
    BK4819_writeReg(dev, 0x37, 0x1d0f);
    BK4819_writeReg(dev, 0x13, 0x3be);
    BK4819_writeReg(dev, 0x12, 0x37b);
    BK4819_writeReg(dev, 0x11, 0x27b);
    BK4819_writeReg(dev, 0x10, 0x7a);
    BK4819_writeReg(dev, 0x14, 0x19);
    BK4819_writeReg(dev, 0x49, 0x2a38);
    BK4819_writeReg(dev, 0x7b, 0x8420);
    BK4819_writeReg(dev, 0x7d, 0xe959);
    BK4819_writeReg(dev, 0x48, 0xb3c1);
    BK4819_writeReg(dev, 0x1e, 0x4c58);
    BK4819_writeReg(dev, 0x1f, 0xa656);
    BK4819_writeReg(dev, 0x3e, 0xa037);
    BK4819_writeReg(dev, 0x3f, 0x7fe);
    BK4819_writeReg(dev, 0x2a, 0x7fff);
    BK4819_writeReg(dev, 0x28, 0x6b00);
    BK4819_writeReg(dev, 0x53, 59000);
    BK4819_writeReg(dev, 0x2c, 0x5705);
    BK4819_writeReg(dev, 0x4b, 0x7102);
    uVar1 = BK4819_readReg(dev, 0x40);
    BK4819_writeReg(dev, 0x40, (uVar1 & 0xf000) | 0x4d2);
    BK4819_writeReg(dev, 0x77, 0x88ef);
    BK4819_writeReg(dev, 0x26, 0x13a0);
    BK4819_writeReg(dev, 0x4e, 0x6f15);
    BK4819_writeReg(dev, 0x4f, 0x3f3e);
    BK4819_writeReg(dev, 9, 0x6f);
    BK4819_writeReg(dev, 9, 0x106b);
    BK4819_writeReg(dev, 9, 0x2067);
    BK4819_writeReg(dev, 9, 0x3062);
    BK4819_writeReg(dev, 9, 0x4050);
    BK4819_writeReg(dev, 9, 0x5047);
    BK4819_writeReg(dev, 9, 0x603a);
    BK4819_writeReg(dev, 9, 0x702c);
    BK4819_writeReg(dev, 9, 0x8041);
    BK4819_writeReg(dev, 9, 0x9037);
    BK4819_writeReg(dev, 9, 0xa025);
    BK4819_writeReg(dev, 9, 0xb017);
    BK4819_writeReg(dev, 9, 0xc0e4);
    BK4819_writeReg(dev, 9, 0xd0cb);
    BK4819_writeReg(dev, 9, 0xe0b5);
    BK4819_writeReg(dev, 9, 0xf09f);
    BK4819_writeReg(dev, 0x74, 0xfa02);
    BK4819_writeReg(dev, 0x44, 0x8f88);
    BK4819_writeReg(dev, 0x45, 0x3201);
    uVar1 = BK4819_readReg(dev, 0x31);
    BK4819_writeReg(dev, 0x31, uVar1 & 0xfffffff7);
    BK4819_writeReg(dev, 0x28, 0x6b38);
    BK4819_writeReg(dev, 0x29, 0xb4cb);
    BK4819_writeReg(dev, BK4819_REG_36, 0xdfbf);
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
    BK4819_writeReg(dev, 0x37, 0x1F0F);
    delayUs(1);
    BK4819_writeReg(dev, 0x30, 0x0200);
    BK4819_writeReg(dev, 0x30, 0xBFF1);
}

void bk4819_set_modulation(const struct BK4819 *dev, bool is_FM)
{
    BK4819_SetAF(dev, is_FM ? 1 : 7);
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
    uint16_t Value = BK4819_readReg(dev, 0x43);
    if (bandwidth) { /* 25kHz */
        BK4819_writeReg(dev, 0x43, (Value & ~0x30) | 32);
    } else {         /* 12.5kHz */
        BK4819_writeReg(dev, 0x43, (Value & ~0x30) | 0);
    }
}

void bk4819_gpio_pin_set(const struct BK4819 *dev, uint8_t Pin, bool bSet)
{
    Pin += 1;
    uint16_t BK4819_GpioOutState = BK4819_readReg(dev, BK4819_REG_33);
    /*
     * Enable GPIO output (set REG_33<15:8> to 0x00)
     * HACK: this enables all GPIO pins as output
     */
    BK4819_GpioOutState &= 0x00FF;
    if (bSet) {
        BK4819_GpioOutState |= (0x0080 >> Pin);
    } else {
        BK4819_GpioOutState &= ~(0x0080 >> Pin);
    }
    BK4819_writeReg(dev, BK4819_REG_33, BK4819_GpioOutState);
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
    BK4819_writeReg(dev, BK4819_REG_4F, (ETSC << 8) | ETSO);
    BK4819_writeReg(dev, BK4819_REG_4D, GTSC);
    BK4819_writeReg(dev, BK4819_REG_4E, GTSO);
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
    /* AF Output Inverse Mode = Inverse, undocumented bits 0x2040 */
    BK4819_writeReg(dev, BK4819_REG_47, (6u << 12) | (AF << 8) | (1u << 6));
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
