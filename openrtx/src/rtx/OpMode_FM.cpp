/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 *                                                                         *
 *   (2025) Modified by KD0OSS for FM mode on Module17                     *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <OpMode_FM.hpp>
#include <threads.h>
#include <string.h>
#include <rtx.h>

#if defined(PLATFORM_TTWRPLUS)
#include "AT1846S.h"
#endif

#if !defined(PLATFORM_MOD17)
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
{
}

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

    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
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
#else

#include <calibInfo_Mod17.h>
#include <DSTAR/RingBuffer.h>
#include <fm_audio_module17.h>
//#include <drivers/USART3_MOD17.h> // for debugging

using namespace std;

CRingBuffer<int16_t> pcmBuffer(1600);
CRingBuffer<int16_t> rxBuffer(2401);
CRingBuffer<q15_t>   m_outputRFRB(2400U); // 100ms
bool rx_started = false;
bool tx_started = false;
const uint8_t RX_BLOCK_SIZE = 2;
const uint16_t DC_OFFSET = 2400;
extern mod17Calib_t mod17CalData;
extern const float ctcss_index[];

OpMode_FM::OpMode_FM() : rfSqlOpen(false), sqlOpen(false), enterRx(true)
{
    m_accessMode = 1;
}

OpMode_FM::~OpMode_FM()
{
}

void OpMode_FM::enable()
{
    radio_enableRx();
    fm_init();
    baseband_buffer = std::make_unique< int16_t[] >(2 * 160);
    m_rxLevel = mod17CalData.fm_rx_level;
    m_txLevel = mod17CalData.fm_tx_level;
    m_ctcssTX_level = mod17CalData.ctcsstx_level;
    m_ctcssRX_freq = mod17CalData.ctcssrx_freq;
    m_ctcssTX_freq = mod17CalData.ctcsstx_freq;
    m_ctcssRX_thrshHi = mod17CalData.ctcssrx_thrshhi;
    m_ctcssRX_thrshLo = mod17CalData.ctcssrx_thrshlo;
    m_noiseSq_on = mod17CalData.noisesq_on;
    m_noiseSq_thrshHi = mod17CalData.noisesq_thrshhi;
    m_noiseSq_thrshLo = mod17CalData.noisesq_thrshlo;
    m_maxDev = mod17CalData.maxdev;
    m_preRxLevel = m_rxLevel * 128;
    fm.reset();
    m_accessMode = 1;
    if(m_ctcssRX_freq == 0)
    {
        m_ctcssRX_thrshLo = 0;
        m_ctcssRX_thrshHi = 0;
        m_accessMode = 0;
    }
    else
    {
        if(m_noiseSq_on)
            m_accessMode = 2;
    }
    fm.setMisc(ctcss_index[m_ctcssRX_freq], m_ctcssRX_thrshHi, m_ctcssRX_thrshLo, m_accessMode, 0, m_noiseSq_on, m_noiseSq_thrshHi, m_noiseSq_thrshLo, m_maxDev, 4);
    m_ctcssTX.setParams(ctcss_index[m_ctcssTX_freq], m_ctcssTX_level);
    enterRx   = true;
}

void OpMode_FM::disable()
{
    // Clean shutdown.
    enterRx   = false;
    if(fm_running())
        fm_stop(rxAudioPath);
    fm_terminate();
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    radio_disableRtx();
    rfSqlOpen = false;
    sqlOpen   = false;
    // Ensure proper termination of baseband sampling
    audioPath_release(basebandPath);
    audioStream_terminate(basebandId);
}

void OpMode_FM::reset()
{
    m_rxLevel = mod17CalData.fm_rx_level;
    m_txLevel = mod17CalData.fm_tx_level;
    m_ctcssTX_level = mod17CalData.ctcsstx_level;
    m_ctcssRX_freq = mod17CalData.ctcssrx_freq;
    m_ctcssTX_freq = mod17CalData.ctcsstx_freq;
    m_ctcssRX_thrshHi = mod17CalData.ctcssrx_thrshhi;
    m_ctcssRX_thrshLo = mod17CalData.ctcssrx_thrshlo;
    m_noiseSq_on = mod17CalData.noisesq_on;
    m_noiseSq_thrshHi = mod17CalData.noisesq_thrshhi;
    m_noiseSq_thrshLo = mod17CalData.noisesq_thrshlo;
    m_maxDev = mod17CalData.maxdev;
    m_preRxLevel = m_rxLevel * 128;
    fm.reset();
    m_accessMode = 1;
    if(m_ctcssRX_freq == 0)
    {
        m_ctcssRX_thrshLo = 0;
        m_ctcssRX_thrshHi = 0;
        m_accessMode = 0;
    }
    else
    {
        if(m_noiseSq_on)
            m_accessMode = 2;
    }
    fm.setMisc(ctcss_index[m_ctcssRX_freq], m_ctcssRX_thrshHi, m_ctcssRX_thrshLo, m_accessMode, 0, m_noiseSq_on, m_noiseSq_thrshHi, m_noiseSq_thrshLo, m_maxDev, 4);
    m_ctcssTX.setParams(ctcss_index[m_ctcssTX_freq], m_ctcssTX_level);
}

void OpMode_FM::startBasebandSampling(const bool isTx)
{
    if(!isTx)
    {
        basebandPath = audioPath_request(SOURCE_RTX, SINK_MCU, PRIO_RX);
        basebandId = audioStream_start(basebandPath, baseband_buffer.get(),	2 * 160, 24000, STREAM_INPUT | BUF_CIRC_DOUBLE);
    }
    else
    {
        basebandPath = audioPath_request(SOURCE_MCU, SINK_RTX, PRIO_TX);
        basebandId = audioStream_start(basebandPath, baseband_buffer.get(),	2 * 160, 24000,	STREAM_OUTPUT | BUF_CIRC_DOUBLE);
        idleBuffer = baseband_buffer.get();
    }
}

void OpMode_FM::stopBasebandSampling()
{
    audioStream_terminate(basebandId);
    audioPath_release(basebandPath);
}

void OpMode_FM::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    if(!enterRx)
    {
        sleepFor(0, 30);
        return;
    }

    // RX logic
    if(status->opStatus == RX)
    {
        if (!rx_started)
        {
            radio_enableRx();
            startBasebandSampling(false);
            rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
            fm_startDecode(rxAudioPath);
            rx_started = true;
        }

        // Read samples from the ADC
        q15_t samples[RX_BLOCK_SIZE];
        int frame = 0;
        dataBlock_t baseband = inputStream_getData(basebandId);
        if(baseband.data != NULL)
        {
            // Process samples
            for(size_t i = 0; i < baseband.len; i++)
            {
                int16_t elem   = static_cast< int16_t >(baseband.data[i]);
                q15_t res1 = q15_t(elem) - DC_OFFSET;
                q31_t res2 = res1 * m_preRxLevel;
                samples[frame++] = q15_t(__SSAT((res2 >> 15), 16));
                if (frame == 2)
                {
                    fm.samples(true, samples, RX_BLOCK_SIZE);
                    frame = 0;
                }
            }
        }
    }
    else if((status->opStatus == OFF) && enterRx)
    {
        radio_disableRtx();
        status->opStatus = RX;
    }

    // TX logic
    if(platform_getPttStatus() && (status->opStatus != TX) &&
        (status->txDisable == 0))
    {
        if(fm_running())
            fm_stop(rxAudioPath);
        audioPath_release(rxAudioPath);
        if (rx_started)
        {
            stopBasebandSampling();
            rx_started = false;
        }

        if (!tx_started)
        {
            startBasebandSampling(true);
            txAudioPath = audioPath_request(SOURCE_MIC, SINK_MCU, PRIO_RX);
            fm_startEncode(txAudioPath);
            tx_started = true;
        }
        radio_disableRtx();

        radio_enableTx();
        status->opStatus = TX;
    }

    if(status->opStatus == TX && pcmBuffer.getData() >= 160)
    {
        memset(idleBuffer, 0x00, 160 * sizeof(stream_sample_t));
        int16_t tmp;
        for(size_t i = 0;i < 160;i++)
        {
            pcmBuffer.get(tmp);
            idleBuffer[i] = static_cast< int16_t >((tmp * (m_txLevel / 100)) + (m_ctcssTX.getAudio(false) * 2));
        }
        outputStream_sync(basebandId, true);
        idleBuffer = outputStream_getIdleBuffer(basebandId);
    }

    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        if(fm_running())
            fm_stop(txAudioPath);
        audioPath_release(txAudioPath);
        stopBasebandSampling();
        tx_started = false;
        radio_disableRtx();

        status->opStatus = OFF;
        sqlOpen = false;  // Force squelch to be redetected.
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:
            if(fm.m_rfSignal)
            {
                status->lsfOk = true;
                platform_ledOn(GREEN);  // Red + green LEDs ("orange"): tone squelch open
                platform_ledOn(RED);
            }
            else if(fm.m_openSq)
            {
                status->lsfOk = true;
                platform_ledOn(GREEN);  // Green LED only: RF squelch open
                platform_ledOff(RED);
            }
            else
            {
                status->lsfOk = false;
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
}
#endif
