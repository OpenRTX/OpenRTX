/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/nvmem.h>

#include <hwconfig.h>
#include <interfaces/nvmem.h>
#include <interfaces/radio.h>
#include <interfaces/delays.h>
#include <core/utils.h>

#include <algorithm>
#include <string>

#include "calibration/calib_C62.h"
#include "drivers/baseband/BK4819.h"
#include "radioUtils.h"

#include <zephyr/drivers/pwm.h>
#include <zephyr/kernel.h>

static const rtxStatus_t
    *config; // Pointer to data structure with radio configuration

static enum opstatus radioStatus; // Current operating status
static enum opmode radioMode;     // Current operating mode

/*
 * C62 BK4819 fields for M17, on top of bk4819_setFlatAudio(): RX and TX
 * values, and the bits that keep their FM value in TX. M17 RX uses AF bypass,
 * REG_47 bit 1 and RX gain2 0x3f, see
 * https://wiki.m17foundation.org/index.php?title=A36Plus
 * The FM values are saved when entering M17 and restored when leaving.
 */
static const struct {
    bk4819_reg_t reg;
    uint16_t mask;
    uint16_t rx;
    uint16_t tx;
    uint16_t txFm;
} m17Fields[] = {
    // TX deviation on, raw value 0x582 (not Hz) for the TX DAC gain in
    // hwconfig.h
    { BK4819_REG_40, 0x1fff, 0x1582, 0x1582, 0x0000 },
    // 4.5 kHz RF / 3.75 kHz weak-signal filter, 12.5 kHz channel
    { BK4819_REG_43, 0x7ffc, 0x7808, 0x7808, 0x0000 },
    { BK4819_REG_47, 0x0002, 0x0002, 0x0000, 0x0002 }, // Bit 1
    // RX gain1 -6 dB, RX gain2 0x3f
    { BK4819_REG_48, 0x0ff0, 0x07f0, 0x0400, 0x03f0 },
    { BK4819_REG_7D, 0x001f, 0x0000, 0x0000, 0x0000 }, // Minimum mic gain
    { BK4819_REG_7E, 0x8000, 0x0000, 0x8000, 0x0000 }, // AGC fix mode in TX
};

static uint16_t fmFields[ARRAY_SIZE(m17Fields)];

/**
 * Apply the M17 audio settings for receiving or transmitting.
 *
 * @param transmit: true for TX, false for RX.
 */
static void applyM17Fields(bool transmit)
{
    bk4819_setFlatAudio(&c62_bk4819, true);

    for (size_t i = 0; i < ARRAY_SIZE(m17Fields); i++) {
        const auto &field = m17Fields[i];
        uint16_t value = BK4819_readReg(&c62_bk4819, field.reg) & ~field.mask;
        value |= transmit ? field.tx | (fmFields[i] & field.txFm) : field.rx;
        BK4819_writeReg(&c62_bk4819, field.reg, value);
    }
}

static struct C62Calib calData;   // TX power calibration

/* The BK4819 GPIOs to control the C62 LNA and PA */
enum {
    GPIO_VHF_RX_LNA = 0,
    GPIO_UHF_RX_LNA,
    GPIO_VHF_TX_PA,
    GPIO_UHF_TX_PA,
    GPIO_ALC_TX_LED
};

/**
 * Set the TX power control (APC) voltage through the PWM on pin A03. The band
 * calibration gives the duty cycles of the low and high power settings at the
 * frequency; the power is interpolated linearly between them, from the low
 * setting at 1 W to the high one at 5 W, as on the MD-3x0 and MD-UV3x0. The
 * PWM runs at 100 kHz, as in the stock firmware: at 1 kHz the APC voltage
 * follows the ripple and the output power stops tracking the duty cycle.
 *
 * @param freq: TX frequency, in Hz.
 * @param power: TX power, in mW.
 */
static void set_tx_power(const freq_t freq, const uint32_t power)
{
    const struct C62BandCalib *band = &calData.uhf;
    if (freq < 174000000)
        band = &calData.vhf;

    struct C62CalPoint cal = calibC62_interp(band, freq);

    float pwr = static_cast<float>(power) / 1000.0f;
    pwr = std::max(std::min(pwr, 5.0f), 1.0f);
    float pwrHi = static_cast<float>(cal.hiPwr);
    float pwrLo = static_cast<float>(cal.loPwr);
    uint8_t duty =
        static_cast<uint8_t>(pwrLo + (pwrHi - pwrLo) / 4.0f * (pwr - 1.0f));

    const uint32_t period_ns = 10000;
    uint32_t pulse_ns = (period_ns * duty) / 100;

    int ret = pwm_set_dt(&pwm_rf_apc, period_ns, pulse_ns);

    if (ret < 0) {
        printk("Failed to set TX power PWM: %d\n", ret);
    }
}

void radio_init(const rtxStatus_t *rtxState)
{
    config = rtxState;
    radioStatus = OFF;
    radioMode = OPMODE_FM;

    nvm_readCalibData(&calData);

    BK4819_SetAF(&c62_bk4819, BK4819_AF_TYPE_MUTE);

    gpioDev_setMode(&c62_bk4819_gpio, GPIO_VHF_RX_LNA, OUTPUT_ACTIVE_LOW);
    gpioDev_setMode(&c62_bk4819_gpio, GPIO_UHF_RX_LNA, OUTPUT_ACTIVE_LOW);
    gpioDev_setMode(&c62_bk4819_gpio, GPIO_VHF_TX_PA, OUTPUT);
    gpioDev_setMode(&c62_bk4819_gpio, GPIO_UHF_TX_PA, OUTPUT);
    gpioDev_setMode(&c62_bk4819_gpio, GPIO_ALC_TX_LED, OUTPUT);
}

void radio_terminate()
{
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    (void)vhfOffset;
    (void)uhfOffset;
}

void radio_setOpmode(const enum opmode mode)
{
    if (mode == radioMode)
        return;

    // The core calls this before disabling the old mode: unkey first
    radio_disableRtx();
    radio_disableAfOutput();

    if (radioMode == OPMODE_M17) {
        for (size_t i = 0; i < ARRAY_SIZE(m17Fields); i++) {
            const auto &field = m17Fields[i];
            uint16_t value = BK4819_readReg(&c62_bk4819, field.reg);
            BK4819_writeReg(&c62_bk4819, field.reg,
                            (value & ~field.mask) | fmFields[i]);
        }
        bk4819_setFlatAudio(&c62_bk4819, false);
    }

    // Save the FM values; radio_enableRx/Tx() apply the M17 ones
    if (mode == OPMODE_M17) {
        for (size_t i = 0; i < ARRAY_SIZE(m17Fields); i++) {
            const auto &field = m17Fields[i];
            fmFields[i] = BK4819_readReg(&c62_bk4819, field.reg) & field.mask;
        }
    }

    radioMode = mode;
}

bool radio_checkRxDigitalSquelch()
{
    return bk4819_get_ctcss(&c62_bk4819);
}

void radio_enableAfOutput()
{
    if (radioMode == OPMODE_M17)
        BK4819_SetAF(&c62_bk4819, BK4819_AF_TYPE_BYPASS);
    else
        BK4819_SetAF(&c62_bk4819, BK4819_AF_TYPE_FM);
}

void radio_disableAfOutput()
{
    BK4819_SetAF(&c62_bk4819, 0);
}

void radio_enableRx()
{
    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_ALC_TX_LED);

    //Before enable RX path delay to allow hardware to settle
    delayMs(5);

    if (config->rxFrequency < 174000000) {
        gpioDev_set(&c62_bk4819_gpio, GPIO_VHF_RX_LNA);
    } else {
        gpioDev_set(&c62_bk4819_gpio, GPIO_UHF_RX_LNA);
    }

    bk4819_set_freq(&c62_bk4819, config->rxFrequency);

    if (radioMode == OPMODE_M17) {
        applyM17Fields(false);
    } else {
        bk4819_SetFilterBandwidth(&c62_bk4819, config->bandwidth);
        if (config->rxToneEn)
            bk4819_enable_rx_ctcss(&c62_bk4819, config->rxTone);
    }

    bk4819_rx_on(&c62_bk4819);
    radioStatus = RX;
}

void radio_enableTx()
{
    if (config->txDisable == 1)
        return;

    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_ALC_TX_LED);

    //Before enable TX path delay to allow hardware to settle
    delayMs(5);

    if (config->txFrequency < 136000000 || config->txFrequency > 600000000)
        return;

    bk4819_set_freq(&c62_bk4819, config->txFrequency);

    if (radioMode == OPMODE_M17)
        applyM17Fields(true);
    else if (config->txToneEn)
        bk4819_enable_tx_ctcss(&c62_bk4819, config->txTone);

    if (config->txFrequency < 174000000) {
        gpioDev_set(&c62_bk4819_gpio, GPIO_VHF_TX_PA);
    } else {
        gpioDev_set(&c62_bk4819_gpio, GPIO_UHF_TX_PA);
    }

    gpioDev_set(&c62_bk4819_gpio, GPIO_ALC_TX_LED);

    set_tx_power(config->txFrequency, config->txPower);

    bk4819_tx_on(&c62_bk4819);
    radioStatus = TX;
}

void radio_disableRtx()
{
    bk4819_disable_ctdcss(&c62_bk4819);

    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_ALC_TX_LED);

    bk4819_rtx_off(&c62_bk4819);
    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    /*
     * Update VCO frequency and tuning parameters if current operating status
     * is different from OFF.
     * This is done by calling again the corresponding functions, which is safe
     * to do and avoids code duplication.
     */
    if (radioStatus == RX)
        radio_enableRx();
    if (radioStatus == TX)
        radio_enableTx();
}

rssi_t radio_getRssi()
{
    return bk4819_get_rssi(&c62_bk4819);
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}