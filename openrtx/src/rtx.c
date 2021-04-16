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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <interfaces/audio.h>
#include <string.h>
#include <rtx.h>
#ifdef PLATFORM_MDUV3x0
#include "../../platform/drivers/baseband/HR_C6000.h"
#endif

pthread_mutex_t *cfgMutex;  /* Mutex for incoming config messages   */

const rtxStatus_t *newCnf;  /* Pointer for incoming config messages */
rtxStatus_t rtxStatus;      /* RTX driver status                    */

bool sqlOpen;               /* Flag for squelch open/close          */
bool enterRx;               /* Flag for RX mode activation          */

float rssi;                 /* Current RSSI in dBm                  */

/*
 * Unfortunately on MD-UV3x0 radios the volume knob does not regulate
 * the amplitude of the analog signal towards the audio amplifier but it
 * rather serves to provide a digital value to be fed into the HR_C6000
 * lineout DAC gain. We thus have to place the #ifdef'd piece of code
 * below to keep the real volume level consistent with the knob position.
 * Knob position is given by an analog signal in the range 0 - 1500mV,
 * which has to be mapped in a range between 1 and 31.
 */
#ifdef PLATFORM_MDUV3x0
void _setVolume()
{
    float   level  = (platform_getVolumeLevel() / 1560.0f) * 30.0f;
    uint8_t volume = ((uint8_t) (level + 0.5f));

    // Mute volume when knob is set below 10%
    if(volume < 1)
        audio_disableAmp();
    else
    {
        audio_enableAmp();
        /* Update HR_C6000 gain only if volume changed */
        static uint8_t old_volume = 0;
        if(volume != old_volume)
        {
            // Setting HR_C6000 volume to 0 = max volume
            C6000_setDacGain(volume);
            old_volume = volume;
        }
    }
}
#endif

void rtx_init(pthread_mutex_t *m)
{
    /* Initialise mutex for configuration access */
    cfgMutex = m;
    newCnf = NULL;

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

    /*
     * Initial value for RSSI filter
     */
    rssi = radio_getRssi(rtxStatus.rxFrequency);
}

void rtx_terminate()
{
    radio_terminate();
}

void rtx_configure(const rtxStatus_t *cfg)
{
    /*
     * NOTE: an incoming configuration may overwrite a preceding one not yet
     * read by the radio task. This mechanism ensures that the radio driver
     * always gets the most recent configuration.
     */

    pthread_mutex_lock(cfgMutex);
    newCnf = cfg;
    pthread_mutex_unlock(cfgMutex);
}

rtxStatus_t rtx_getCurrentStatus()
{
    return rtxStatus;
}

void rtx_taskFunc()
{
    /* Check if there is a pending new configuration and, in case, read it. */
    bool reconfigure = false;
    if(pthread_mutex_trylock(cfgMutex) == 0)
    {
        if(newCnf != NULL)
        {
            /* Copy new configuration and override opStatus flags */
            uint8_t tmp = rtxStatus.opStatus;
            memcpy(&rtxStatus, newCnf, sizeof(rtxStatus_t));
            rtxStatus.opStatus = tmp;

            reconfigure = true;
            newCnf = NULL;
        }

        pthread_mutex_unlock(cfgMutex);
    }

    if(reconfigure)
    {
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

    /* RX logic */
    if(rtxStatus.opStatus == RX)
    {
        /*
         * RSSI-based squelch mechanism, with 15 levels from -140dBm to -70dBm.
         *
         * RSSI value is passed through a filter with a time constant of 60ms
         * (cut-off frequency of 15Hz) at an update rate of 33.3Hz.
         *
         * The low pass filter skips an update step if a new configuration has
         * just been applied. This is a workaround for the AT1846S returning a
         * full-scale RSSI value immediately after one of its parameters changed,
         * thus causing the squelch to open briefly.
         */
        if(!reconfigure)
        {
            rssi = 0.74*radio_getRssi(rtxStatus.rxFrequency) + 0.26*rssi;
        }

        float squelch = -127.0f + rtxStatus.sqlLevel * 66.0f / 15.0f;

        //if((sqlOpen == false) && (rssi > (squelch + 0.1f)))
        //{
        //    audio_enableAmp();
        //    sqlOpen = true;
        //}

        //if((sqlOpen == true) && (rssi < (squelch - 0.1f)))
        //{
        //    audio_disableAmp();
        //    sqlOpen = false;
        //}

        //#ifdef PLATFORM_MDUV3x0
        //if(sqlOpen == true)
        //{
        //    // Set output volume by changing the HR_C6000 DAC gain
        //    _setVolume();
        //}
        //#endif
    }
    else if((rtxStatus.opMode == OFF) && enterRx)
    {
        radio_disableRtx();

        radio_setVcoFrequency(rtxStatus.rxFrequency, false);
        radio_enableRx();
        rtxStatus.opStatus = RX;
        enterRx = false;

        /* Reinitialise RSSI filter state */
        rssi = radio_getRssi(rtxStatus.rxFrequency);
    }

    /* TX logic */
    if(platform_getPttStatus() && (rtxStatus.opStatus != TX))
    {
        audio_disableAmp();
        radio_disableRtx();

        audio_enableMic();
        radio_setVcoFrequency(rtxStatus.txFrequency, true);
        radio_enableTx(rtxStatus.txPower, rtxStatus.txToneEn);

        rtxStatus.opStatus = TX;
    }

    if(!platform_getPttStatus() && (rtxStatus.opStatus == TX))
    {
        audio_disableMic();
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
    return rssi;
}
