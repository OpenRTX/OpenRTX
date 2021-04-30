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

/**
 * \internal
 * On MD-UV3x0 radios the volume knob does not regulate the amplitude of the
 * analog signal towards the audio amplifier but it rather serves to provide a
 * digital value to be fed into the HR_C6000 lineout DAC gain. We thus have to
 * provide the helper function below to keep the real volume level consistent
 * with the knob position.
 * Current knob position corresponds to an analog signal in the range 0 - 1500mV,
 * which has to be mapped in a range between 1 and 31.
 */
#ifdef PLATFORM_MDUV3x0
void _setVolume()
{
    float   level  = (platform_getVolumeLevel() / 1560.0f) * 30.0f;
    uint8_t volume = ((uint8_t) (level + 0.5f));

    // Mute volume when knob is set below 10%
    if(volume < 1)
    {
        audio_disableAmp();
    }
    else
    {
        audio_enableAmp();

        // Update HR_C6000 gain only if volume changed
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

OpMode_FM::OpMode_FM() : sqlOpen(false), enterRx(true)
{
}

OpMode_FM::~OpMode_FM()
{
}

void OpMode_FM::enable()
{
    // When starting, close squelch and prepare for entering in RX mode.
    sqlOpen = false;
    enterRx = true;
}

void OpMode_FM::disable()
{
    // Clean shutdown.
    audio_disableAmp();
    audio_disableMic();
    radio_disableRtx();
    sqlOpen = false;
    enterRx = false;
}

void OpMode_FM::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // RX logic
    if(status->opStatus == RX)
    {
        float squelch = -127.0f + status->sqlLevel * 66.0f / 15.0f;
        float rssi    = rtx_getRssi();

        if((sqlOpen == false) && (rssi > (squelch + 0.1f)))
        {
            audio_enableAmp();
            sqlOpen = true;
        }

        if((sqlOpen == true) && (rssi < (squelch - 0.1f)))
        {
            audio_disableAmp();
            sqlOpen = false;
        }

        #ifdef PLATFORM_MDUV3x0
        if(sqlOpen == true)
        {
            // Set output volume by changing the HR_C6000 DAC gain
            _setVolume();
        }
        #endif
    }
    else if((status->opStatus == OFF) && enterRx)
    {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        enterRx = false;
    }

    /* TX logic */
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
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:
            if(sqlOpen)
            {
                platform_ledOn(GREEN);
            }
            else
            {
                platform_ledOff(GREEN);
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
