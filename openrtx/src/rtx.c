/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/radio.h>
#include <string.h>
#include <rtx.h>

#include <interfaces/gpio.h>
#include <hwconfig.h>

OS_MUTEX *cfgMutex;     /* Mutex for incoming config messages */
OS_Q cfgMailbox;        /* Queue for incoming config messages */

rtxStatus_t rtxStatus;  /* RTX driver status                  */

bool sqlOpen;           /* Flag for squelch open/close        */
bool enterRx;           /* Flag for RX mode activation        */

/*
 * These functions below provide a basic API for audio path management. They
 * will be removed once the audio driver is set up.
 */

void _afCtrlInit()
{
    #if defined(PLATFORM_MD3x0)
    gpio_setMode(SPK_MUTE, OUTPUT);
    gpio_setMode(AMP_EN,   OUTPUT);
    gpio_setMode(FM_MUTE,  OUTPUT);
    gpio_setMode(MIC_PWR,  OUTPUT);
    #elif defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    #endif
}

void _afCtrlSpeaker(bool enable)
{
    if(enable)
    {
        #if defined(PLATFORM_MD3x0)
        gpio_setPin(AMP_EN);
        gpio_setPin(FM_MUTE);
        gpio_clearPin(SPK_MUTE);
        #elif defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
        gpio_setPin(AUDIO_AMP_EN);
        #endif
    }
    else
    {
        #if defined(PLATFORM_MD3x0)
        gpio_clearPin(AMP_EN);
        gpio_clearPin(FM_MUTE);
        gpio_setPin(SPK_MUTE);
        #elif defined(PLATFORM_GD77) || defined(PLATFORM_DM1801)
        gpio_clearPin(AUDIO_AMP_EN);
        #endif
    }
}

void _afCtrlMic(bool enable)
{
     if(enable)
    {
        #if defined(PLATFORM_MD3x0)
        gpio_setPin(MIC_PWR);
        #endif
    }
    else
    {
        #if defined(PLATFORM_MD3x0)
        gpio_clearPin(MIC_PWR);
        #endif
    }
}

void _afCtrlTerminate()
{
    _afCtrlMic(false);
    _afCtrlSpeaker(false);
}


void rtx_init(OS_MUTEX *m)
{
    /* Initialise mutex for configuration access */
    cfgMutex = m;

    /* Create the message queue for RTX configuration */
    OS_ERR err;
    OSQCreate((OS_Q     *) &cfgMailbox,
              (CPU_CHAR *) "",
              (OS_MSG_QTY) 1,
              (OS_ERR   *) &err);

    /*
     * Default initialisation for rtx status
     */
    rtxStatus.opMode      = FM;
    rtxStatus.bandwidth   = BW_25;
    rtxStatus.txDisable   = 0;
    rtxStatus.opStatus    = OFF;
    rtxStatus.rxFrequency = 430000000;
    rtxStatus.txFrequency = 430000000;
    rtxStatus.txPower     = 0.0f;
    rtxStatus.sqlLevel    = 1;
    rtxStatus.rxToneEn    = 0;
    rtxStatus.rxTone      = 0;
    rtxStatus.txToneEn    = 0;
    rtxStatus.txTone      = 0;

    sqlOpen = false;
    enterRx = false;

    /*
     * Initialise low-level platform-specific driver
     */
    radio_init();

    _afCtrlInit();
}

void rtx_terminate()
{
    _afCtrlTerminate();
    radio_terminate();
}

void rtx_configure(const rtxStatus_t *cfg)
{
    OS_ERR err;
    OSQPost((OS_Q      *) &cfgMailbox,
            (void      *) cfg,
            (OS_MSG_SIZE) sizeof(rtxStatus_t *),
            (OS_OPT     ) OS_OPT_POST_FIFO,
            (OS_ERR    *) &err);

    /*
     * In case message queue is not empty, flush the old and unread configuration
     * and post the new one.
     */
    if(err == OS_ERR_Q_MAX)
    {
        OSQFlush((OS_Q   *) &cfgMailbox,
                 (OS_ERR *) &err);

        OSQPost((OS_Q      *) &cfgMailbox,
                (void      *) cfg,
                (OS_MSG_SIZE) sizeof(rtxStatus_t *),
                (OS_OPT     ) OS_OPT_POST_FIFO,
                (OS_ERR    *) &err);
    }
}

rtxStatus_t rtx_getCurrentStatus()
{
    return rtxStatus;
}

void rtx_taskFunc()
{
    OS_ERR err;
    OS_MSG_SIZE size;
    void *msg = OSQPend((OS_Q        *) &cfgMailbox,
                        (OS_TICK      ) 0,
                        (OS_OPT       ) OS_OPT_PEND_NON_BLOCKING,
                        (OS_MSG_SIZE *) &size,
                        (CPU_TS      *) NULL,
                        (OS_ERR      *) &err);

    /* Configuration update logic */
    if((err == OS_ERR_NONE) && (msg != NULL))
    {
        /* Try locking mutex for read access */
        OSMutexPend(cfgMutex, 0, OS_OPT_PEND_NON_BLOCKING, NULL, &err);

        if(err == OS_ERR_NONE)
        {
            /* Copy new configuration and override opStatus flags */
            uint8_t tmp = rtxStatus.opStatus;
            memcpy(&rtxStatus, msg, sizeof(rtxStatus_t));
            rtxStatus.opStatus = tmp;

            /* Done, release mutex */
            OSMutexPost(cfgMutex, OS_OPT_POST_NONE, &err);

            /* Update HW configuration */
            radio_setOpmode(rtxStatus.opMode);
            radio_setBandwidth(rtxStatus.bandwidth);
            radio_setCSS(rtxStatus.rxTone, rtxStatus.txTone);
            radio_updateCalibrationParams(&rtxStatus);

            /*
             * If currently transmitting or receiving, update VCO frequency and
             * call again enableRx/enableTx.
             * This is done because the new configuration may have changed the
             * RX and TX frequencies, requiring an update of both the VCO
             * settings and of some tuning parameters, like APC voltage, which
             * are managed by enableRx/enableTx.
             */
            if(rtxStatus.opStatus == TX)
            {
                radio_setVcoFrequency(rtxStatus.txFrequency, true);
                radio_enableTx(rtxStatus.txPower, rtxStatus.txToneEn);
            }

            if(rtxStatus.opStatus == RX)
            {
                radio_setVcoFrequency(rtxStatus.rxFrequency, false);
                radio_enableRx();
            }

            /* TODO: temporarily force to RX mode if rtx is off. */
            if(rtxStatus.opStatus == OFF) enterRx = true;
        }
    }

    /* RX logic */
    if(rtxStatus.opStatus == RX)
    {
        /*
         * RSSI-based squelch mechanism, with 15 levels from -140dBm to -70dBm
         */
        float rssi = rtx_getRssi();
        float squelch = -127.0f + rtxStatus.sqlLevel * 66.0f / 15.0f;

        if((sqlOpen == false) && (rssi > (squelch + 0.1f)))
        {
            _afCtrlSpeaker(true);
            sqlOpen = true;
        }

        if((sqlOpen == true) && (rssi < (squelch - 0.1f)))
        {
            _afCtrlSpeaker(false);
            sqlOpen = false;
        }
    }
    else if((rtxStatus.opMode == OFF) && enterRx)
    {
        radio_disableRtx();

        radio_setVcoFrequency(rtxStatus.rxFrequency, false);
        radio_enableRx();
        rtxStatus.opStatus = RX;
        enterRx = false;
    }

    /* TX logic */
    if(platform_getPttStatus() && (rtxStatus.opStatus != TX))
    {
        radio_disableRtx();

        _afCtrlMic(true);
        radio_setVcoFrequency(rtxStatus.txFrequency, true);
        radio_enableTx(rtxStatus.txPower, rtxStatus.txToneEn);

        rtxStatus.opStatus = TX;
    }

    if(!platform_getPttStatus() && (rtxStatus.opStatus == TX))
    {
        _afCtrlMic(false);
        radio_disableRtx();

        rtxStatus.opStatus = OFF;
        enterRx = true;
    }

    /* Led control logic  */
    switch(rtxStatus.opStatus)
    {
        case RX:
            if(sqlOpen)
                platform_ledOn(GREEN);
            else
                platform_ledOff(GREEN);

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

float rtx_getRssi()
{
    return radio_getRssi(rtxStatus.rxFrequency);
}
