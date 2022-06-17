/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <interfaces/audio.h>
#include <OpMode_FM.h>
#include <rtx.h>

#ifdef PLATFORM_MDUV3x0
#include "../../../drivers/baseband/HR_C6000.h"
#endif

/**
 * \internal
 * On MD-UV3x0 radios the volume knob does not regulate the amplitude of the
 * analog signal towards the audio amplifier but it rather serves to provide a
 * digital value to be fed into the HR_C6000 lineout DAC gain. We thus have to
 * provide the helper function below to keep the real volume level consistent
 * with the knob position.
 */
#ifdef PLATFORM_MDUV3x0
void _setVolume()
{
    // Volume level range is 0 - 255, by right shifting by 3 we get a value in
    // range 0 - 31.
    uint8_t volume = platform_getVolumeLevel();
    volume >>= 3;

    // Mute volume when knob is set below 10%
    if(volume < 1)
    {
        audio_disableAmp();
    }
    else
    {
        audio_enableAmp();
        // Setting HR_C6000 volume to 0 = max volume
        HR_C6000::instance().setDacGain(volume);
    }
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
    audio_disableAmp();
    audio_disableMic();
    radio_disableRtx();
    rfSqlOpen = false;
    sqlOpen   = false;
    enterRx   = false;
}

void OpMode_FM::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // RX logic
    if(status->opStatus == RX)
    {
        // RF squelch mechanism
        // This turns squelch (0 to 15) into RSSI (-127.0dbm to -61dbm)
        float squelch = -127.0f + status->sqlLevel * 66.0f / 15.0f;
        float rssi    = rtx_getRssi();

        // Provide a bit of hysteresis, only change state if the RSSI has
        // moved more than .1dbm on either side of the current squelch setting.
        if((rfSqlOpen == false) && (rssi > (squelch + 0.1f))) rfSqlOpen = true;
        if((rfSqlOpen == true)  && (rssi < (squelch - 0.1f))) rfSqlOpen = false;

        // Local flags for current RF and tone squelch status
        bool rfSql   = ((status->rxToneEn == 0) && (rfSqlOpen == true));
        bool toneSql = ((status->rxToneEn == 1) && radio_checkRxDigitalSquelch());

        // Audio control
        if((sqlOpen == false) && (rfSql || toneSql))
        {
            audio_enableAmp();
            sqlOpen = true;
        }

        if((sqlOpen == true) && (rfSql == false) && (toneSql == false))
        {
            audio_disableAmp();
            sqlOpen = false;
        }

        #ifdef PLATFORM_MDUV3x0
        // Set output volume by changing the HR_C6000 DAC gain
        if(sqlOpen == true) _setVolume();
        #endif

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
        audio_disableAmp();
        radio_disableRtx();

        audio_enableMic();
        radio_enableTx();

        status->opStatus = TX;
    }

    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        audio_disableMic();
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
