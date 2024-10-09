/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
 ***************************************************************************/

#include <interfaces/radio.h>
#include <hwconfig.h>
#include <string.h>
#include <rtx.h>
#include <OpMode_FM.hpp>
#include <OpMode_M17.hpp>
#include <state.h>
#ifdef PLATFORM_A36PLUS
#include <platform/drivers/baseband/bk4819.h>
#endif

extern state_t state;
static pthread_mutex_t   *cfgMutex;     // Mutex for incoming config messages
static const rtxStatus_t *newCnf;       // Pointer for incoming config messages
static rtxStatus_t        rtxStatus;    // RTX driver status
static rssi_t             rssi;         // Current RSSI in dBm
static bool               reinitFilter; // Flag for RSSI filter re-initialisation

static OpMode  *currMode;               // Pointer to currently active opMode handler
static OpMode     noMode;               // Empty opMode handler for opmode::NONE
static OpMode_FM  fmMode;               // FM mode handler
#ifdef CONFIG_M17
static OpMode_M17 m17Mode;              // M17 mode handler
#endif


void rtx_init(pthread_mutex_t *m)
{
    // Initialise mutex for configuration access
    cfgMutex = m;
    newCnf   = NULL;

    /*
     * Default initialisation for rtx status
     */
    rtxStatus.opMode        = OPMODE_NONE;
    rtxStatus.bandwidth     = BW_25;
    rtxStatus.txDisable     = 0;
    rtxStatus.opStatus      = OFF;
    rtxStatus.rxFrequency   = 431025000;
    rtxStatus.txFrequency   = 431025000;
    rtxStatus.txPower       = 0.0f;
    rtxStatus.sqlLevel      = 1;
    rtxStatus.rxToneEn      = 0;
    rtxStatus.rxTone        = 0;
    rtxStatus.txToneEn      = 0;
    rtxStatus.txTone        = 0;
    rtxStatus.voxEn         = 0;
    rtxStatus.invertRxPhase = false;
    rtxStatus.lsfOk         = false;
    rtxStatus.M17_src[0]    = '\0';
    rtxStatus.M17_dst[0]    = '\0';
    rtxStatus.M17_link[0]   = '\0';
    rtxStatus.M17_refl[0]   = '\0';
    currMode = &noMode;

    /*
     * Initialise low-level platform-specific driver
     */
    radio_init(&rtxStatus);
    radio_updateConfiguration();

    /*
     * Initial value for RSSI filter
     */
    rssi         = radio_getRssi();
    reinitFilter = false;
}

void rtx_terminate()
{
    rtxStatus.opStatus = OFF;
    rtxStatus.opMode   = OPMODE_NONE;
    currMode->disable();
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

void rtx_task()
{
    // Check if there is a pending new configuration and, in case, read it.
    bool reconfigure = false;
    if(pthread_mutex_trylock(cfgMutex) == 0)
    {
        if(newCnf != NULL)
        {
            // Copy new configuration and override opStatus flags
            uint8_t tmp = rtxStatus.opStatus;
            memcpy(&rtxStatus, newCnf, sizeof(rtxStatus_t));
            rtxStatus.opStatus = tmp;

            reconfigure = true;
            newCnf = NULL;
        }

        pthread_mutex_unlock(cfgMutex);
    }

        /* Spectrum update block, run when in SPECTRUM mode.
    *
    * The spectrum mode is a special mode where the radio is in RX mode but
    * the audio path is disabled. This allows to display the RSSI level of the
    * received signals across a frequency range in a waterfall-like display.
    * 
    * This block writes the received RSSI levels to the spectrum buffer.
    */
    #ifdef PLATFORM_A36PLUS
    if(state.rtxStatus == RTX_SPECTRUM)
    {
        //state.spectrum_peakRssi = -160;
        #define SPECTRUM_WF_LINES 64 // must match NUMBER_BARS in ui_menu.c!
        uint32_t spectrumStep = freq_steps[state.settings.spectrum_step]/10;
        // Get the current RSSI level

        uint8_t peakIndex;
        // Write the RSSI level to the spectrum buffer
        for (int i = 0; i < SPECTRUM_WF_LINES; i++)
        {
            bk4819_set_freq((state.spectrum_startFreq + i * spectrumStep));
            rssi = radio_getRssi();
            // uint8_t height = (rssi + 160) / 2;
            // Macro for log2, not using the math library
            #define log2(x) (31 - __builtin_clz(x))
            uint8_t height = ((rssi + 160)*log2(22 - (rssi>>1) )) >> 3;
            state.spectrum_data[i] = height;
            // set peak value
            if(rssi > state.spectrum_peakRssi)
            {
                state.spectrum_peakRssi = rssi;
                state.spectrum_peakFreq = state.spectrum_startFreq + i * spectrumStep;
                state.spectrum_peakIndex = i;
            }
            // stop scanning if the rssi is greater than the current squelch rssi,
            // and listen to that frequency
            if(radio_getRssi() > (-127 + (state.settings.sqlLevel * 66) / 15))
            {
                // turn the speaker on
                radio_enableAfOutput();
                rssi = radio_getRssi();
                while(rssi > (-127 + (state.settings.sqlLevel * 66) / 15))
                {
                    // allow us to exit the loop if the spectrum has been exited,
                    // or the frequency has changed
                    if(state.rtxStatus != RTX_SPECTRUM ||
                       state.spectrum_peakFreq != state.spectrum_startFreq + i * spectrumStep)
                    {
                        state.spectrum_data[i] = 0;
                        break;
                    }
                    rssi = radio_getRssi();
                    height = ((rssi + 160)*log2(22 - (rssi>>1) )) >> 3;
                    state.spectrum_data[i] = height;
                    // give the UI a chance to refresh
                    state.spectrum_shouldRefresh = true;
                    sleepFor(0, 150);
                }
                // turn the speaker off
                radio_disableAfOutput();
            }
        }
        state.spectrum_shouldRefresh = true;
        //state.spectrum_peakIndex = peakIndex;
    }
    #endif

    if(reconfigure)
    {
        // Force TX and RX tone squelch to off for OpModes different from FM.
        if(rtxStatus.opMode != OPMODE_FM)
        {
            rtxStatus.txToneEn = 0;
            rtxStatus.rxToneEn = 0;
        }

        /*
         * Handle change of opMode:
         * - deactivate current opMode and switch operating status to "OFF";
         * - update pointer to current mode handler to the OpMode object for the
         *   selected mode;
         * - enable the new mode handler
         */
        if(currMode->getID() != rtxStatus.opMode)
        {
            // Forward opMode change also to radio driver
            radio_setOpmode(static_cast< enum opmode >(rtxStatus.opMode));

            currMode->disable();
            rtxStatus.opStatus = OFF;

            switch(rtxStatus.opMode)
            {
                case OPMODE_NONE: currMode = &noMode;  break;
                case OPMODE_FM:   currMode = &fmMode;  break;
                #ifdef CONFIG_M17
                case OPMODE_M17:  currMode = &m17Mode; break;
                #endif
                default:   currMode = &noMode;
            }

            currMode->enable();
        }

        // Tell radio driver that there was a change in its configuration.
        radio_updateConfiguration();
    }

    /*
     * RSSI update block, run only when radio is in RX mode.
     *
     * RSSI value is passed through a filter with a time constant of 60ms
     * (cut-off frequency of 15Hz) at an update rate of 33.3Hz.
     *
     * The low pass filter skips an update step if a new configuration has
     * just been applied. This is a workaround for the AT1846S returning a
     * full-scale RSSI value immediately after one of its parameters changed,
     * thus causing the squelch to open briefly.
     *
     * Also, the RSSI filter is re-initialised every time radio stage is
     * switched back from TX/OFF to RX. This provides a workaround for some
     * radios reporting a full-scale RSSI value when transmitting.
     */
    if(rtxStatus.opStatus == RX)
    {
        if(state.rtxStatus != RTX_SPECTRUM)
        {
            if(!reconfigure)
            {
                if(!reinitFilter)
                {
                    /*
                    * Filter RSSI value using 15.16 fixed point math. Equivalent
                    * floating point code is: rssi = 0.74*radio_getRssi() + 0.26*rssi
                    */
                    int32_t filt_rssi = radio_getRssi() * 0xBD70    // 0.74 * radio_getRssi
                                    + rssi            * 0x428F;   // 0.26 * rssi
                    rssi = (filt_rssi + 32768) >> 16;               // Round to nearest
                }
                else
                {
                    rssi = radio_getRssi();
                    reinitFilter = false;
                }
            }
        }
    }
    else
    {
        // Reinit required if current operating status is TX or OFF
        reinitFilter = true;
    }

    /*
     * Forward the periodic update step to the currently active opMode handler.
     * Call is placed after RSSI update to allow handler's code have a fresh
     * version of the RSSI level.
     */
    currMode->update(&rtxStatus, reconfigure);
}

rssi_t rtx_getRssi()
{
    return rssi;
}

bool rtx_rxSquelchOpen()
{
    return currMode->rxSquelchOpen();
}
