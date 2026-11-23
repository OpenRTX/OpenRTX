/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <hwconfig.h>
#include <interfaces/nvmem.h>
#include <interfaces/radio.h>
#include <interfaces/delays.h>

#include <algorithm>

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
 * CTCSS TX gain of the stock C62 firmware, REG_51 <6:0> "CTCSS/CDCSS Tx Gain1
 * Tuning" (0 = min, 127 = max) in the BK4819 notes of
 * https://github.com/egzumer/uv-k5-firmware-custom/blob/7607f0a4bd6203d1f06b70556fc1ce0d7399d6b3/driver/bk4819.c#L463-L471
 */
static constexpr uint8_t CTCSS_TX_GAIN = 65;

// Use the same gain as the CTCSS tone for the tone burst
static constexpr uint16_t TONE_BURST_FREQ = 1750;
static constexpr uint8_t TONE_BURST_GAIN = 65;

// CW keyed tone in F2A, at the deviation of the tone burst
static constexpr uint16_t CW_TONE_FREQ = 700;
static constexpr uint8_t CW_TONE_GAIN = TONE_BURST_GAIN;

/*
 * CW key edges, against key clicks: a raised cosine over 4 ms, the level in
 * 1/256 of full scale at each 500 us step, applied to the APC in A1A and to
 * the tone gain in F2A.
 */
static constexpr uint16_t KEY_RAMP[] = {
    0, 10, 37, 79, 128, 177, 219, 246, 256,
};
static constexpr uint16_t KEY_RAMP_FULL = 256;
static constexpr int32_t KEY_RAMP_STEP_US = 500;

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
static uint32_t apcPulse;        // APC PWM pulse for the TX power, in ns

static constexpr uint32_t APC_PERIOD_NS = 10000;

/**
 * Set the TX power control (APC) PWM pulse.
 *
 * @param pulse_ns: pulse width, in ns, up to APC_PERIOD_NS.
 */
static void set_apc(const uint32_t pulse_ns)
{
    int ret = pwm_set_dt(&pwm_rf_apc, APC_PERIOD_NS, pulse_ns);

    if (ret < 0) {
        printk("Failed to set TX power PWM: %d\n", ret);
    }
}

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

    apcPulse = (APC_PERIOD_NS * duty) / 100;
    set_apc(apcPulse);
}

void radio_init(const rtxStatus_t *rtxState)
{
    config = rtxState;
    radioStatus = OFF;
    radioMode = OPMODE_FM;

    nvm_readCalibData(&calData);

    bk4819_setAfOutput(&c62_bk4819, BK4819_AF_MUTE);

    gpioDev_setMode(LNA_VHF_EN, OUTPUT_ACTIVE_LOW);
    gpioDev_setMode(LNA_UHF_EN, OUTPUT_ACTIVE_LOW);
    gpioDev_setMode(PA_VHF_EN, OUTPUT);
    gpioDev_setMode(PA_UHF_EN, OUTPUT);
    gpioDev_setMode(ALC_TX_LED_EN, OUTPUT);
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
        bk4819_setBandwidth(&c62_bk4819, BK4819_BW_12K5);
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
    gpioDev_clear(LNA_VHF_EN);
    gpioDev_clear(LNA_UHF_EN);
    gpioDev_clear(PA_VHF_EN);
    gpioDev_clear(PA_UHF_EN);
    gpioDev_clear(ALC_TX_LED_EN);

    //Before enable RX path delay to allow hardware to settle
    delayMs(5);

    if (config->rxFrequency < 174000000) {
        gpioDev_set(LNA_VHF_EN);
    } else {
        gpioDev_set(LNA_UHF_EN);
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

    gpioDev_clear(LNA_VHF_EN);
    gpioDev_clear(LNA_UHF_EN);
    gpioDev_clear(PA_VHF_EN);
    gpioDev_clear(PA_UHF_EN);
    gpioDev_clear(ALC_TX_LED_EN);

    //Before enable TX path delay to allow hardware to settle
    delayMs(5);

    if (config->txFrequency < 136000000 || config->txFrequency > 600000000)
        return;

    bk4819_setFrequency(&c62_bk4819, config->txFrequency);

    if ((radioMode != OPMODE_M17) && config->txToneEn) {
        bk4819_enableTxCtcss(&c62_bk4819, config->txTone, CTCSS_TX_GAIN);
    }

    if (config->txFrequency < 174000000) {
        gpioDev_set(PA_VHF_EN);
    } else {
        gpioDev_set(PA_UHF_EN);
    }

    gpioDev_set(ALC_TX_LED_EN);

    set_tx_power(config->txPower);

    // CW starts key up: in A1A without carrier
    bool a1a = (radioMode == OPMODE_CW) && (config->cwEmission == CW_A1A);
    if (a1a)
        set_apc(0);

    bk4819_enableTx(&c62_bk4819);

    if (a1a)
        bk4819_enablePa(&c62_bk4819, false);

    // CW keying tone off until key down, tone burst while the UI asks for
    // it, both in place of the microphone
    if (radioMode == OPMODE_CW) {
        bk4819_enableTxTone(&c62_bk4819, CW_TONE_FREQ, 0);
        bk4819_setTxToneGain(&c62_bk4819, 0);
    } else if ((radioMode == OPMODE_FM) && config->toneEn)
        bk4819_enableTxTone(&c62_bk4819, TONE_BURST_FREQ, TONE_BURST_GAIN);
    else
        bk4819_disableTxTone(&c62_bk4819);

    radioStatus = TX;
}

void radio_disableRtx()
{
    bk4819_disableCtdcss(&c62_bk4819);
    bk4819_disableTxTone(&c62_bk4819);

    gpioDev_clear(LNA_VHF_EN);
    gpioDev_clear(LNA_UHF_EN);
    gpioDev_clear(PA_VHF_EN);
    gpioDev_clear(PA_UHF_EN);
    gpioDev_clear(ALC_TX_LED_EN);

    // Leaving RX closes the AF output: M17 opened it in radio_enableRx(),
    // FM's audio path closes it too
    if (radioStatus == RX)
        radio_disableAfOutput();

    bk4819_disableRtx(&c62_bk4819);
    radioStatus = OFF;
}

void radio_setCwKey(const bool down)
{
    // radio_enableTx() may have refused to transmit
    if (radioStatus != TX)
        return;

    bool a1a = (config->cwEmission == CW_A1A);
    if (a1a && down)
        bk4819_enablePa(&c62_bk4819, true);

    for (size_t i = 1; i < ARRAY_SIZE(KEY_RAMP); i++) {
        uint16_t level = down ? KEY_RAMP[i] :
                                KEY_RAMP[ARRAY_SIZE(KEY_RAMP) - 1 - i];

        if (a1a)
            set_apc((apcPulse * level) / KEY_RAMP_FULL);
        else
            bk4819_setTxToneGain(&c62_bk4819,
                                 (CW_TONE_GAIN * level) / KEY_RAMP_FULL);

        k_usleep(KEY_RAMP_STEP_US);
    }

    if (a1a && !down)
        bk4819_enablePa(&c62_bk4819, false);
}

void radio_updateConfiguration()
{
    // TX calibration at the TX frequency, from the band its PA is in
    const struct C62BandCalib *band = &calData.uhf;
    if (config->txFrequency < 174000000)
        band = &calData.vhf;

    txCal = calibC62_interp(band, config->txFrequency);

    // Set bandwidth and deviation, for analog FM and the CW keyed tone
    if ((config->opMode == OPMODE_FM) || (config->opMode == OPMODE_CW)) {
        switch (config->bandwidth) {
            case BW_12_5:
                bk4819_setBandwidth(&c62_bk4819, BK4819_BW_12K5);
                bk4819_setTxDeviation(&c62_bk4819, true, txCal.txDevNarrow);
                break;

            case BW_25:
                bk4819_setBandwidth(&c62_bk4819, BK4819_BW_25K);
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
