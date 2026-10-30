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
 * C62 BK4819 settings for M17, on top of the audio bypass mode, from the
 * A36Plus profile: https://wiki.m17foundation.org/index.php?title=A36Plus
 */
// Raw deviation tuning (not Hz), set for the TX DAC gain in hwconfig.h
static constexpr uint16_t M17_TX_DEVIATION = 0x582;
// 4.5 kHz RF filter, 3.75 kHz when the signal is weak
static constexpr uint16_t M17_RX_FILTER =
    BK4819_REG43_RF_BW(7) | BK4819_REG43_RF_BW_WEAK(4) | BK4819_REG43_UNDOC3;
static constexpr uint8_t M17_AF_RX_GAIN1 = 1; // -6 dB
static constexpr uint8_t M17_AF_RX_GAIN2 = 0x3f;
static constexpr uint8_t M17_MIC_GAIN = 0;

static struct C62Calib calData; // TX power calibration

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

    // Settings common to M17 RX and TX; radio_enableRx/Tx() set the rest
    if (mode == OPMODE_M17) {
        bk4819_setAudioMode(&c62_bk4819, BK4819_AUDIO_BYPASS);
        bk4819_setTxDeviation(&c62_bk4819, true, M17_TX_DEVIATION);
        // 12.5 kHz bandwidth mode; M17_RX_FILTER then replaces its filters
        bk4819_setBandwidth(&c62_bk4819, BK4819_BW_12_5);
        bk4819_setRxFilter(&c62_bk4819, M17_RX_FILTER);
        bk4819_setMicGain(&c62_bk4819, M17_MIC_GAIN);
    } else if (radioMode == OPMODE_M17) {
        bk4819_setAudioMode(&c62_bk4819, BK4819_AUDIO_VOICE);
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
    BK4819_SetAF(&c62_bk4819, BK4819_AF_TYPE_MUTE);
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
        bk4819_setAfRxGain(&c62_bk4819, M17_AF_RX_GAIN1, M17_AF_RX_GAIN2);
        bk4819_setAgcFixed(&c62_bk4819, false);
    } else {
        bk4819_setBandwidth(&c62_bk4819, (config->bandwidth == BW_25) ?
                                             BK4819_BW_25 :
                                             BK4819_BW_12_5);
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

    if (radioMode == OPMODE_M17) {
        bk4819_setAfRxGain(&c62_bk4819, M17_AF_RX_GAIN1,
                           BK4819_DEFAULT_AF_RX_GAIN2);
        bk4819_setAgcFixed(&c62_bk4819, true);
    } else if (config->txToneEn)
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