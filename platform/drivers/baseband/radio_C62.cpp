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
 * CTCSS/CDCSS TX gain of the stock C62 firmware, REG_51 <6:0> "CTCSS/CDCSS
 * Tx Gain1 Tuning" (0 = min, 127 = max) in the BK4819 notes of
 * https://github.com/egzumer/uv-k5-firmware-custom/blob/7607f0a4bd6203d1f06b70556fc1ce0d7399d6b3/driver/bk4819.c#L463-L471
 */
static constexpr uint8_t SUBAUDIO_TX_GAIN = 65;

/*
 * C62 BK4819 settings for M17, on top of the audio bypass mode, from the
 * A36Plus profile: https://wiki.m17foundation.org/index.php?title=A36Plus
 */
// Raw deviation tuning (not Hz), set for the TX DAC gain in hwconfig.h
static constexpr uint16_t M17_TX_DEVIATION = 0x582;
static constexpr uint8_t M17_RF_BW = 7;          // 4.5 kHz
static constexpr uint8_t M17_RF_BW_WEAK = 4;     // 3.75 kHz
static constexpr uint8_t M17_TX_LPF2_BW = 0;     // 3 kHz
static constexpr uint8_t M17_RX_AUDIO_GAIN1 = 1; // -6 dB
static constexpr uint8_t M17_RX_AUDIO_GAIN2 = 0x3f;
static constexpr uint8_t M17_MIC_GAIN = 0;

static struct C62Calib calData;  // TX calibration
static struct C62CalPoint txCal; // TX calibration at the TX frequency

/* The BK4819 GPIOs to control the C62 LNA and PA */
enum {
    GPIO_VHF_RX_LNA = 0,
    GPIO_UHF_RX_LNA,
    GPIO_VHF_TX_PA,
    GPIO_UHF_TX_PA,
    GPIO_ALC_TX_LED
};

/**
 * Set the TX power control (APC) voltage through the PWM on pin A03. The TX
 * calibration gives the duty cycles of the low and high power settings at the
 * TX frequency; the power is interpolated linearly between them, from the low
 * setting at 1 W to the high one at 5 W, as on the MD-3x0 and MD-UV3x0. The
 * PWM runs at 100 kHz, as in the stock firmware: at 1 kHz the APC voltage
 * follows the ripple and the output power stops tracking the duty cycle.
 *
 * @param power: TX power, in mW.
 */
static void set_tx_power(const uint32_t power)
{
    float pwr = static_cast<float>(power) / 1000.0f;
    pwr = std::max(std::min(pwr, 5.0f), 1.0f);
    float pwrHi = static_cast<float>(txCal.hiPwr);
    float pwrLo = static_cast<float>(txCal.loPwr);
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

    bk4819_setAfOutput(&c62_bk4819, BK4819_AF_MUTE);

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
    // The core calls this before disabling the old mode: unkey first
    radio_disableRtx();

    if (mode == OPMODE_M17) {
        bk4819_setAudioMode(&c62_bk4819, BK4819_AUDIO_BYPASS);
        bk4819_setTxDeviation(&c62_bk4819, true, M17_TX_DEVIATION);
        // 12.5 kHz bandwidth mode; the M17 filters then replace its filters
        bk4819_setBandwidth(&c62_bk4819, BK4819_BW_12_5);
        bk4819_setRfFilter(&c62_bk4819, M17_RF_BW, M17_RF_BW_WEAK);
        bk4819_setTxAudioFilter(&c62_bk4819, M17_TX_LPF2_BW);
        bk4819_setRxAudioGain(&c62_bk4819, M17_RX_AUDIO_GAIN1,
                              M17_RX_AUDIO_GAIN2);
        bk4819_setMicGain(&c62_bk4819, M17_MIC_GAIN);
    } else {
        bk4819_setAudioMode(&c62_bk4819, BK4819_AUDIO_VOICE);
    }

    radioMode = mode;
}

bool radio_checkRxDigitalSquelch()
{
    return bk4819_getCtcss(&c62_bk4819);
}

void radio_enableAfOutput()
{
    // Only the RTX to speaker path opens it, and only FM uses that path
    bk4819_setAfOutput(&c62_bk4819, BK4819_AF_FM);
}

void radio_disableAfOutput()
{
    bk4819_setAfOutput(&c62_bk4819, BK4819_AF_MUTE);
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

    bk4819_setFrequency(&c62_bk4819, config->rxFrequency);

    if (radioMode == OPMODE_M17) {
        // The DSP takes the baseband from the AF output for the whole
        // reception. M17 sets its bandwidth on the mode change and has no
        // CTCSS.
        bk4819_setAfOutput(&c62_bk4819, BK4819_AF_BYPASS);
    } else if (config->rxToneEn) {
        bk4819_enableRxCtcss(&c62_bk4819, config->rxTone);
    }

    bk4819_enableRx(&c62_bk4819);
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

    bk4819_setFrequency(&c62_bk4819, config->txFrequency);

    if ((radioMode != OPMODE_M17) && config->txToneEn) {
        bk4819_enableTxCtcss(&c62_bk4819, config->txTone, SUBAUDIO_TX_GAIN);
    }

    if (config->txFrequency < 174000000) {
        gpioDev_set(&c62_bk4819_gpio, GPIO_VHF_TX_PA);
    } else {
        gpioDev_set(&c62_bk4819_gpio, GPIO_UHF_TX_PA);
    }

    gpioDev_set(&c62_bk4819_gpio, GPIO_ALC_TX_LED);

    set_tx_power(config->txPower);

    bk4819_enableTx(&c62_bk4819);
    radioStatus = TX;
}

void radio_disableRtx()
{
    bk4819_disableCtdcss(&c62_bk4819);

    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_RX_LNA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_VHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_UHF_TX_PA);
    gpioDev_clear(&c62_bk4819_gpio, GPIO_ALC_TX_LED);

    // Leaving RX closes the AF output: M17 opened it in radio_enableRx(),
    // FM's audio path closes it too
    if (radioStatus == RX)
        radio_disableAfOutput();

    bk4819_disableRtx(&c62_bk4819);
    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    // TX calibration at the TX frequency, from the band its PA is in
    const struct C62BandCalib *band = &calData.uhf;
    if (config->txFrequency < 174000000)
        band = &calData.vhf;

    txCal = calibC62_interp(band, config->txFrequency);

    // Set bandwidth, only for analog FM mode
    if (config->opMode == OPMODE_FM) {
        switch (config->bandwidth) {
            case BW_12_5:
                bk4819_setBandwidth(&c62_bk4819, BK4819_BW_12_5);
                bk4819_setTxDeviation(&c62_bk4819, true, txCal.txDevNarrow);
                break;

            case BW_25:
                bk4819_setBandwidth(&c62_bk4819, BK4819_BW_25);
                bk4819_setTxDeviation(&c62_bk4819, true, txCal.txDevWide);
                break;

            default:
                break;
        }
    }

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
    return bk4819_getRssi(&c62_bk4819);
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}