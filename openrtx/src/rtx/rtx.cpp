/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *   Frequency Scan Modifications (C) 2025 by Aritra Ghosh                 *
 *                                                                         *
 *   (file header truncated for brevity)                                   *
 ***************************************************************************/

#include "interfaces/radio.h"
#include "hwconfig.h"
#include <string.h>
#include "rtx/rtx.h"
#include "rtx/OpMode_FM.hpp"
#include "rtx/OpMode_M17.hpp"

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
    cfgMutex = m;
    newCnf   = NULL;

    rtxStatus.opMode        = OPMODE_NONE;
    rtxStatus.bandwidth     = BW_25;
    rtxStatus.txDisable     = 0;
    rtxStatus.opStatus      = OFF;
    rtxStatus.rxFrequency   = 430000000;
    rtxStatus.txFrequency   = 430000000;
    rtxStatus.txPower       = 0.0f;
    rtxStatus.sqlLevel      = 1;
    rtxStatus.rxToneEn      = 0;
    rtxStatus.rxTone        = 0;
    rtxStatus.txToneEn      = 0;
    rtxStatus.txTone        = 0;
    rtxStatus.invertRxPhase = false;
    rtxStatus.lsfOk         = false;
    rtxStatus.M17_src[0]    = '\0';
    rtxStatus.M17_dst[0]    = '\0';
    rtxStatus.M17_link[0]   = '\0';
    rtxStatus.M17_refl[0]   = '\0';
    currMode = &noMode;

    radio_init(&rtxStatus);
    radio_updateConfiguration();

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
    bool reconfigure = false;

    if(pthread_mutex_trylock(cfgMutex) == 0)
    {
        if(newCnf != NULL)
        {
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
        if(rtxStatus.opMode != OPMODE_FM)
        {
            rtxStatus.txToneEn = 0;
            rtxStatus.rxToneEn = 0;
        }

        if(currMode->getID() != rtxStatus.opMode)
        {
            radio_setOpmode(static_cast<enum opmode>(rtxStatus.opMode));

            currMode->disable();
            rtxStatus.opStatus = OFF;

            switch(rtxStatus.opMode)
            {
                case OPMODE_NONE: currMode = &noMode; break;
                case OPMODE_FM:   currMode = &fmMode; break;
#ifdef CONFIG_M17
                case OPMODE_M17:  currMode = &m17Mode; break;
#endif
                default:          currMode = &noMode; break;
            }

            currMode->enable();
        }

        radio_updateConfiguration();
    }

    if(rtxStatus.opStatus == RX)
    {
        if(!reconfigure)
        {
            if(!reinitFilter)
            {
                int32_t filt_rssi = radio_getRssi() * 0xBD70 + rssi * 0x428F;
                rssi = (filt_rssi + 32768) >> 16;
            }
            else
            {
                rssi = radio_getRssi();
                reinitFilter = false;
            }
        }
    }
    else
    {
        reinitFilter = true;
    }

    // forward periodic update - FM scan is managed inside OpMode_FM
    currMode->update(&rtxStatus, reconfigure);
}

#ifdef CONFIG_FM_SCAN
bool rtx_isScanning()
{
    if (rtxStatus.opMode == OPMODE_FM)
        return fmMode.isScanning();
    return false;
}

void rtx_startScan(uint32_t startFreq, uint32_t stopFreq, uint32_t stepHz)
{
    if (rtxStatus.opMode == OPMODE_FM)
        fmMode.startScan(startFreq, stopFreq, stepHz);
}

void rtx_stopScan()
{
    if (rtxStatus.opMode == OPMODE_FM)
        fmMode.stopScan();
}
#endif

rssi_t rtx_getRssi()
{
    return rssi;
}

bool rtx_rxSquelchOpen()
{
    return currMode->rxSquelchOpen();
}

