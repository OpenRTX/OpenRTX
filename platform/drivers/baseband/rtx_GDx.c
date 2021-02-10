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
#include <interfaces/gpio.h>
#include <interfaces/rtx.h>
#include <calibInfo_GDx.h>
#include <calibUtils.h>
#include <datatypes.h>
#include <hwconfig.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "HR_C6000.h"
#include "AT1846S.h"

OS_MUTEX *cfgMutex;               /* Mutex for incoming config messages */
OS_Q cfgMailbox;                  /* Queue for incoming config messages */

const gdxCalibration_t *calData;  /* Pointer to calibration data        */

rtxStatus_t rtxStatus;            /* RTX driver status                  */

/*
 * Helper functions to reduce code mess. They all access 'rtxStatus'
 * internally.
 */

int8_t _getBandFromFrequency(freq_t freq)
{
    if((freq >= FREQ_LIMIT_VHF_LO) && (freq <= FREQ_LIMIT_VHF_HI)) return 0;
    if((freq >= FREQ_LIMIT_UHF_LO) && (freq <= FREQ_LIMIT_UHF_HI)) return 1;
    return -1;
}

void _setBandwidth()
{
    /* Override bandwidth configuration for digital modes */
    uint8_t bandwidth = BW_12_5;
    if(rtxStatus.opMode == FM) bandwidth = rtxStatus.bandwidth;

    switch(bandwidth)
    {
        case BW_12_5:
            AT1846S_setBandwidth(AT1846S_BW_12P5);
            break;

        case BW_20:
        case BW_25:
            AT1846S_setBandwidth(AT1846S_BW_25);
            break;

        default:
            break;
    }
}

void _setOpMode()
{
    switch(rtxStatus.opMode)
    {
        case FM:
            gpio_setPin(RX_AUDIO_MUX);          /* Audio out to amplifier */
            gpio_setPin(TX_AUDIO_MUX);          /* Audio in from HR_C6000 */
            AT1846S_setOpMode(AT1846S_OP_FM);
            break;

        case DMR:
            gpio_clearPin(RX_AUDIO_MUX);        /* Audio out to HR_C6000  */
            gpio_setPin(TX_AUDIO_MUX);          /* Audio in from HR_C6000 */
            AT1846S_setOpMode(AT1846S_OP_FM);
            break;

        default:
            break;
    }
}

void _setVcoFrequency()
{
    freq_t freq = rtxStatus.rxFrequency;
    if(rtxStatus.opStatus == TX) freq = rtxStatus.txFrequency;
    AT1846S_setFrequency(freq);
}

void _enableTxStage()
{
    if(rtxStatus.txDisable == 1) return;

    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);

    int8_t band = _getBandFromFrequency(rtxStatus.txFrequency);
    if(band < 0) return;

    /*
     * Set transmit power. Initial setting is 1W, overridden to 5W if tx power
     * is greater than 1W.
     * TODO: increase granularity
     */
    const uint8_t *paramPtr = calData->data[band].txLowPower;
    if(rtxStatus.txPower > 1.0f) paramPtr = calData->data[band].txHighPower;

    uint16_t pwr = 0;
    if(band == 0)
    {
        /* VHF band */
        pwr = interpCalParameter(rtxStatus.txFrequency, calData->vhfCalPoints,
                                 paramPtr, 8);
    }
    else
    {
        /* UHF band */
        pwr = interpCalParameter(rtxStatus.txFrequency, calData->uhfPwrCalPoints,
                                 paramPtr, 16);
    }

    pwr *= 4;
    DAC0->DAT[0].DATH = (pwr >> 8) & 0xFF;
    DAC0->DAT[0].DATL = pwr & 0xFF;

    _setVcoFrequency();
    AT1846S_setFuncMode(AT1846S_TX);

    if(band == 0)
    {
        gpio_setPin(VHF_PA_EN);
    }
    else
    {
        gpio_setPin(UHF_PA_EN);
    }

    rtxStatus.opStatus = TX;
}

void _enableRxStage()
{
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);

    int8_t band = _getBandFromFrequency(rtxStatus.rxFrequency);
    if(band < 0) return;

    _setVcoFrequency();
    AT1846S_setFuncMode(AT1846S_RX);

    if(band == 0)
    {
        gpio_setPin(VHF_LNA_EN);
    }
    else
    {
        gpio_setPin(UHF_LNA_EN);
    }

    rtxStatus.opStatus = RX;
}

void _disableRtxStages()
{
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);
    AT1846S_setFuncMode(AT1846S_OFF);
    rtxStatus.opStatus = OFF;
}

void _updateTuningParams()
{
    int8_t band = _getBandFromFrequency(rtxStatus.rxFrequency);
    if(band < 0) return;

    const bandCalData_t *cal = &(calData->data[band]);

    AT1846S_setPgaGain(cal->PGA_gain);
    AT1846S_setMicGain(cal->analogMicGain);
    AT1846S_setAgcGain(cal->rxAGCgain);
    AT1846S_setRxAudioGain(cal->rxAudioGainWideband, cal->rxAudioGainNarrowband);
    AT1846S_setPaDrive(cal->PA_drv);

    if(rtxStatus.bandwidth == BW_12_5)
    {
        AT1846S_setTxDeviation(cal->mixGainNarrowband);
        AT1846S_setNoise1Thresholds(cal->noise1_HighTsh_Nb, cal->noise1_LowTsh_Nb);
        AT1846S_setNoise2Thresholds(cal->noise2_HighTsh_Nb, cal->noise2_LowTsh_Nb);
        AT1846S_setRssiThresholds(cal->rssi_HighTsh_Nb, cal->rssi_LowTsh_Nb);
    }
    else
    {
        AT1846S_setTxDeviation(cal->mixGainWideband);
        AT1846S_setNoise1Thresholds(cal->noise1_HighTsh_Wb, cal->noise1_LowTsh_Wb);
        AT1846S_setNoise2Thresholds(cal->noise2_HighTsh_Wb, cal->noise2_LowTsh_Wb);
        AT1846S_setRssiThresholds(cal->rssi_HighTsh_Wb, cal->rssi_LowTsh_Wb);
    }

    C6000_setDacRange(cal->dacDataRange);
    C6000_setMod2Bias(cal->mod2Offset);
    C6000_setModOffset(cal->mod1Bias);

    uint8_t mod1Amp  = 0;
    uint8_t sqlTresh = 0;
    if(band == 0)
    {
        /* VHF band */
        mod1Amp = interpCalParameter(rtxStatus.txFrequency, calData->vhfCalPoints,
                                     cal->mod1Amplitude, 8);

        sqlTresh = interpCalParameter(rtxStatus.rxFrequency, calData->vhfCalPoints,
                                     cal->analogSqlThresh, 8);
    }
    else
    {
        /* UHF band */
        mod1Amp = interpCalParameter(rtxStatus.txFrequency, calData->uhfMod1CalPoints,
                                     cal->mod1Amplitude, 8);

        sqlTresh = interpCalParameter(rtxStatus.rxFrequency, calData->uhfMod1CalPoints,
                                     cal->analogSqlThresh, 8);
    }

    C6000_setMod1Amplitude(mod1Amp);
    AT1846S_setAnalogSqlThresh(sqlTresh);
}

void _setCTCSS()
{
    /* TODO */
}

void _updateSqlThresholds()
{
    /* TODO */
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
     * Configure RTX GPIOs
     */
    gpio_setMode(VHF_LNA_EN, OUTPUT);
    gpio_setMode(UHF_LNA_EN, OUTPUT);
    gpio_setMode(VHF_PA_EN,  OUTPUT);
    gpio_setMode(UHF_PA_EN,  OUTPUT);

    gpio_clearPin(VHF_LNA_EN);  /* Turn VHF LNA off */
    gpio_clearPin(UHF_LNA_EN);  /* Turn UHF LNA off */
    gpio_clearPin(VHF_PA_EN);   /* Turn VHF PA off  */
    gpio_clearPin(UHF_PA_EN);   /* Turn UHF PA off  */

    /*
     * Configure audio control GPIOs
     */
    gpio_setMode(AUDIO_AMP_EN, OUTPUT);
    gpio_setMode(RX_AUDIO_MUX, OUTPUT);
    gpio_setMode(TX_AUDIO_MUX, OUTPUT);

    gpio_clearPin(AUDIO_AMP_EN);
    gpio_clearPin(RX_AUDIO_MUX);
    gpio_clearPin(TX_AUDIO_MUX);

    /*
     * Load calibration data
     */
    calData = ((const gdxCalibration_t *) platform_getCalibrationData());

    /*
     * Enable and configure DAC for PA drive control
     */
    SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;
    DAC0->DAT[0].DATL = 0;
    DAC0->DAT[0].DATH = 0;
    DAC0->C0   |= DAC_C0_DACRFS_MASK    /* Reference voltage is Vref2 */
               |  DAC_C0_DACEN_MASK;    /* Enable DAC                 */

    /*
     * Enable and configure both AT1846S and HR_C6000
     */
    AT1846S_init();
    C6000_init();

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
}

void rtx_terminate()
{
    _disableRtxStages();
    gpio_clearPin(AUDIO_AMP_EN);
    C6000_terminate();
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
            _setOpMode();
            _setBandwidth();
            _updateTuningParams();
            _setCTCSS();
            _updateSqlThresholds();
            _setVcoFrequency();

            /* TODO: temporarily force to RX mode if rtx is off. */
            if(rtxStatus.opStatus == OFF) _enableRxStage();
        }
    }

    if(rtxStatus.opStatus == RX)
    {
        float sqlLevel  = (rtx_getRssi() + 127.0f)/6.0f;

        float sqlThresh = 7.0f;
        if(rtxStatus.sqlLevel > 0) sqlThresh = 3.0f;

        if((gpio_readPin(AUDIO_AMP_EN) == 0) && (sqlLevel > (sqlThresh + 0.1f)))
        {
            gpio_setPin(AUDIO_AMP_EN);
            platform_ledOn(GREEN);
        }

        if((gpio_readPin(AUDIO_AMP_EN) == 1) && (sqlLevel < sqlThresh))
        {
            gpio_clearPin(AUDIO_AMP_EN);
            platform_ledOff(GREEN);
        }
    }

    if(platform_getPttStatus() && (rtxStatus.opStatus != TX))
    {
        _disableRtxStages();
        _enableTxStage();
        platform_ledOn(RED);
    }

    if(!platform_getPttStatus() && (rtxStatus.opStatus == TX))
    {
        _disableRtxStages();
        _enableRxStage();
        platform_ledOff(RED);
    }
}

float rtx_getRssi()
{
    uint16_t val = AT1846S_readRSSI();
    int8_t rssi   = -151 + (val >> 8);
    return ((float) rssi);
}
