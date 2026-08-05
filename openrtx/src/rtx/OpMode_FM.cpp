/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/radio.h"
#include "rtx/OpMode_FM.hpp"
#include "rtx/rtx.h"

#if defined(PLATFORM_TTWRPLUS)
#include "drivers/baseband/AT1846S.h"
#endif

#ifdef CONFIG_FM_INBAND_TONES
#include "drivers/audio/Cx000_dac.h"

/**
 * \internal
 * DTMF tone pairs, indexed by keypad code (0-9 digits, 10 = '*', 11 = '#').
 * These mirror the tone table the HR_C6000 uses to generate the transmitted
 * digit and are only used to reproduce it locally on the speaker.
 */
static const uint16_t dtmfTonePairs[12][2] =
{
    {941, 1336},    // 0
    {697, 1209},    // 1
    {697, 1336},    // 2
    {697, 1477},    // 3
    {770, 1209},    // 4
    {770, 1336},    // 5
    {770, 1477},    // 6
    {852, 1209},    // 7
    {852, 1336},    // 8
    {852, 1477},    // 9
    {941, 1209},    // *
    {941, 1477}     // #
};

/**
 * \internal
 * Sidetone identifier for the repeater tone burst, taking the first value past
 * the DTMF keypad codes so that both share one "currently playing" state.
 */
static const uint8_t SIDETONE_TONE_BURST = 12;

/**
 * \internal
 * Frequency of the repeater tone burst, in Hz.
 */
static const uint16_t TONE_BURST_FREQ = 1750;
#endif

/**
 * \internal
 * On MD-UV3x0 radios the volume knob does not regulate the amplitude of the
 * analog signal towards the audio amplifier but it rather serves to provide a
 * digital value to be fed into the HR_C6000 lineout DAC gain. We thus have to
 * provide the helper function below to keep the real volume level consistent
 * with the knob position.
 */
#if defined(PLATFORM_TTWRPLUS)
void _setVolume()
{
    static uint8_t oldVolume = 0xFF;
    uint8_t volume = platform_getVolumeLevel();

    if(volume == oldVolume)
        return;

    // AT1846S volume control is 4 bit
    AT1846S::instance().setRxAudioGain(volume / 16, volume / 16);
    oldVolume = volume;
}
#endif

OpMode_FM::OpMode_FM() : rfSqlOpen(false), sqlOpen(false), enterRx(true)
#ifdef CONFIG_FM_INBAND_TONES
                       , txSidetone(DTMF_CODE_NONE), sidetoneAudioPath(0)
#endif
{
}

#ifdef CONFIG_FM_INBAND_TONES
void OpMode_FM::updateTxSidetone(const uint8_t dtmfCode, const bool toneBurst)
{
    // A digit and the tone burst are on separate keys and can be held at once.
    // The digit wins, matching the order the radio driver keys them in.
    uint8_t wanted = DTMF_CODE_NONE;
    if(dtmfCode < 12)
        wanted = dtmfCode;
    else if(toneBurst)
        wanted = SIDETONE_TONE_BURST;

    if(wanted == txSidetone)
        return;

    // Any change tears down the current sidetone first: this both stops a
    // released key and lets a new digit start from a clean state when the
    // operator slides from one key to another without releasing.
    if(txSidetone != DTMF_CODE_NONE)
    {
        Cx000dac_stopBeep();
        audioPath_release(sidetoneAudioPath);
        sidetoneAudioPath = 0;
    }

    txSidetone = DTMF_CODE_NONE;

    if(wanted == DTMF_CODE_NONE)
        return;

    // The speaker is not connected to anything while transmitting, so the
    // path has to be opened explicitly. PRIO_BEEP keeps the sidetone from
    // stealing the speaker from voice prompts.
    sidetoneAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_BEEP);
    if(sidetoneAudioPath <= 0)
        return;

    int ret;
    if(wanted == SIDETONE_TONE_BURST)
        ret = Cx000dac_startBeep(TONE_BURST_FREQ);
    else
        ret = Cx000dac_startDualTone(dtmfTonePairs[wanted][0],
                                     dtmfTonePairs[wanted][1]);

    if(ret < 0)
    {
        audioPath_release(sidetoneAudioPath);
        sidetoneAudioPath = 0;
        return;
    }

    txSidetone = wanted;
}
#endif

OpMode_FM::~OpMode_FM()
{
}

void OpMode_FM::enable()
{
    // When starting, close squelch and prepare for entering in RX mode.
    rfSqlOpen = false;
    sqlOpen   = false;
    enterRx   = true;
}

void OpMode_FM::disable()
{
    // Clean shutdown.
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    #ifdef CONFIG_FM_INBAND_TONES
    updateTxSidetone(DTMF_CODE_NONE, false);
    #endif
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    radio_disableRtx();
    rfSqlOpen = false;
    sqlOpen   = false;
    enterRx   = false;
}

void OpMode_FM::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    #if defined(PLATFORM_TTWRPLUS)
    // Set output volume by changing the HR_C6000 DAC gain
    _setVolume();
    #endif

    // RX logic
    if(status->opStatus == RX)
    {
        // RF squelch mechanism
        // This turns squelch (0 to 15) into RSSI (-127.0dbm to -61dbm)
        rssi_t squelch = -127 + (status->sqlLevel * 66) / 15;
        rssi_t rssi    = rtx_getRssi();

        // Provide a bit of hysteresis, only change state if the RSSI has
        // moved more than 1dBm on either side of the current squelch setting.
        if((rfSqlOpen == false) && (rssi > (squelch + 1))) rfSqlOpen = true;
        if((rfSqlOpen == true)  && (rssi < (squelch - 1))) rfSqlOpen = false;

        // Local flags for current RF and tone squelch status
        bool rfSql   = ((status->rxToneEn == 0) && (rfSqlOpen == true));
        bool toneSql = ((status->rxToneEn == 1) && radio_checkRxDigitalSquelch());

        // Audio control
        if((sqlOpen == false) && (rfSql || toneSql))
        {
            rxAudioPath = audioPath_request(SOURCE_RTX, SINK_SPK, PRIO_RX);
            if(rxAudioPath > 0) sqlOpen = true;
        }

        if((sqlOpen == true) && (rfSql == false) && (toneSql == false))
        {
            audioPath_release(rxAudioPath);
            sqlOpen = false;
        }
    }
    else if((status->opStatus == OFF) && enterRx)
    {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        enterRx = false;
    }

    // TX logic
    if(platform_getPttStatus() && (status->opStatus != TX) &&
                                  (status->txDisable == 0))
    {
        audioPath_release(rxAudioPath);
        radio_disableRtx();

        txAudioPath = audioPath_request(SOURCE_MIC, SINK_RTX, PRIO_TX);
        radio_enableTx();

        status->opStatus = TX;
    }

    #ifdef CONFIG_FM_INBAND_TONES
    // Echo what is being transmitted on the speaker. Only while actually
    // transmitting: outside of TX neither tone reaches the air.
    bool txing = (status->opStatus == TX);
    updateTxSidetone(txing ? status->dtmf_code : DTMF_CODE_NONE,
                     txing && (status->toneEn != 0));
    #endif

    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        #ifdef CONFIG_FM_INBAND_TONES
        updateTxSidetone(DTMF_CODE_NONE, false);
        #endif
        audioPath_release(txAudioPath);
        radio_disableRtx();

        status->opStatus = OFF;
        enterRx = true;
        sqlOpen = false;  // Force squelch to be redetected.
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:
            if(radio_checkRxDigitalSquelch())
            {
                platform_ledOn(GREEN);  // Red + green LEDs ("orange"): tone squelch open
                platform_ledOn(RED);
            }
            else if(rfSqlOpen)
            {
                platform_ledOn(GREEN);  // Green LED only: RF squelch open
                platform_ledOff(RED);
            }
            else
            {
                platform_ledOff(GREEN);
                platform_ledOff(RED);
            }

            break;

        case TX:
            platform_ledOff(GREEN);
            platform_ledOn(RED);
            break;

        default:
            platform_ledOff(GREEN);
            platform_ledOff(RED);
            break;
    }

    // Sleep thread for 30ms for 33Hz update rate
    sleepFor(0u, 30u);
}

bool OpMode_FM::rxSquelchOpen()
{
    return sqlOpen;
}
