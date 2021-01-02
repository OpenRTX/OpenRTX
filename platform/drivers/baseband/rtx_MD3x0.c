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

#include <toneGenerator_MDx.h>
#include <calibInfo_MDx.h>
#include <calibUtils.h>
#include <datatypes.h>
#include <hwconfig.h>
#include <interfaces/platform.h>
#include <ADC1_MDx.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <interfaces/gpio.h>
#include <interfaces/rtx.h>
#include "HR-C5000_MD3x0.h"
#include "pll_MD3x0.h"

#include <stdio.h>

const freq_t IF_FREQ = 49950000;  /* Intermediate frequency: 49.95MHz   */

OS_MUTEX *cfgMutex;               /* Mutex for incoming config messages */
OS_Q cfgMailbox;                  /* Queue for incoming config messages */

const md3x0Calib_t *calData;      /* Pointer to calibration data        */

rtxStatus_t rtxStatus;            /* RTX driver status                  */
uint8_t sqlOpenTsh;               /* Squelch opening threshold          */
uint8_t sqlCloseTsh;              /* Squelch closing threshold          */

/*
 * Helper functions to reduce code mess. They all access 'rtxStatus'
 * internally.
 */
void _setBandwidth()
{
    /* Override bandwidth configuration for digital modes */
    uint8_t bandwidth = BW_12_5;
    if(rtxStatus.opMode == FM) bandwidth = rtxStatus.bandwidth;

    switch(bandwidth)
    {
        case BW_12_5:
            gpio_setPin(WN_SW);
            C5000_setModFactor(0x1E);
            break;

        case BW_20:
            gpio_clearPin(WN_SW);
            C5000_setModFactor(0x30);
            break;

        case BW_25:
            gpio_clearPin(WN_SW);
            C5000_setModFactor(0x3C);
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
            gpio_clearPin(DMR_SW);
            gpio_setPin(FM_SW);
            C5000_fmMode();
            break;

        case DMR:
            gpio_clearPin(FM_SW);
            gpio_setPin(DMR_SW);
            //C5000_dmrMode();
            break;

        default:
            break;
    }
}

void _setVcoFrequency()
{
    if(rtxStatus.opStatus == RX)
    {
        pll_setFrequency(((float) rtxStatus.rxFrequency - IF_FREQ), 5);
    }
    else if(rtxStatus.opStatus == TX)
    {
        pll_setFrequency(rtxStatus.txFrequency, 5);
    }
}

void _enableTxStage()
{
    if(rtxStatus.txDisable == 1) return;

    gpio_clearPin(RX_STG_EN);

    gpio_setPin(RF_APC_SW);
    gpio_clearPin(VCOVCC_SW);

    /*
     * Set transmit power. Initial setting is 1W, overridden to 5W if tx power
     * is greater than 1W.
     * TODO: increase granularity
     */
    const uint8_t *paramPtr = calData->txLowPower;
    if(rtxStatus.txPower > 1.0f) paramPtr = calData->txHighPower;
    uint8_t apc = interpCalParameter(rtxStatus.txFrequency, calData->txFreq,
                                     paramPtr, 9);
    DAC->DHR12L1 = apc * 0xFF;

    gpio_setPin(TX_STG_EN);
    rtxStatus.opStatus = TX;

    _setVcoFrequency();
}

void _enableRxStage()
{
    gpio_clearPin(TX_STG_EN);

    gpio_clearPin(RF_APC_SW);
    gpio_setPin(VCOVCC_SW);

    /* Set tuning voltage for input filter */
    uint8_t tuneVoltage = interpCalParameter(rtxStatus.rxFrequency,
                                             calData->rxFreq,
                                             calData->rxSensitivity, 9);
    DAC->DHR12L1 = tuneVoltage * 0xFF;

    gpio_setPin(RX_STG_EN);
    rtxStatus.opStatus = RX;

    _setVcoFrequency();
}

void _disableRtxStages()
{
    gpio_clearPin(TX_STG_EN);
    gpio_clearPin(RX_STG_EN);
    rtxStatus.opStatus = OFF;
}

void _updateC5000IQparams()
{
    const uint8_t *Ical = calData->sendIrange;
    if(rtxStatus.opMode == FM) Ical = calData->analogSendIrange;
    uint8_t I = interpCalParameter(rtxStatus.txFrequency, calData->txFreq, Ical,
                                   9);

    const uint8_t *Qcal = calData->sendQrange;
    if(rtxStatus.opMode == FM) Qcal = calData->analogSendQrange;
    uint8_t Q = interpCalParameter(rtxStatus.txFrequency, calData->txFreq, Qcal,
                                   9);

    C5000_setModAmplitude(I, Q);
}

void _setCTCSS()
{
    if((rtxStatus.opMode == FM) && (rtxStatus.txTone != 0))
    {
        float tone = ((float) rtxStatus.txTone) / 10.0f;
        toneGen_setToneFreq(tone);
    }
}

void _updateSqlThresholds()
{
    /*
     * TODO:
     * - check why openSql9 is less than openSql1, maybe parameters are swapped
     * - squelch levels 1 - 9
     */
    uint8_t sql1OpenTsh = interpCalParameter(rtxStatus.rxFrequency, calData->rxFreq,
                                             calData->openSql9, 9);

    uint8_t sql1CloseTsh = interpCalParameter(rtxStatus.rxFrequency, calData->rxFreq,
                                              calData->closeSql9, 9);

    sqlOpenTsh = sql1OpenTsh;
    sqlCloseTsh = sql1CloseTsh;

/*    uint8_t sql9OpenTsh = interpCalParameter(rtxStatus.rxFrequency, calData->rxFreq,
                                             calData->openSql1, 9);

    uint8_t sql9CloseTsh = interpCalParameter(rtxStatus.rxFrequency, calData->rxFreq,
                                              calData->closeSql1, 9);

    sqlOpenTsh = sql1OpenTsh + ((rtxStatus.sqlLevel - 1) *
                                (sql9OpenTsh - sql1OpenTsh))/8;

    sqlCloseTsh = sql1CloseTsh + ((rtxStatus.sqlLevel - 1) *
                                  (sql9CloseTsh - sql1CloseTsh))/8; */
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
    gpio_setMode(PLL_PWR,   OUTPUT);
    gpio_setMode(VCOVCC_SW, OUTPUT);
    gpio_setMode(DMR_SW,    OUTPUT);
    gpio_setMode(WN_SW,     OUTPUT);
    gpio_setMode(FM_SW,     OUTPUT);
    gpio_setMode(RF_APC_SW, OUTPUT);
    gpio_setMode(TX_STG_EN, OUTPUT);
    gpio_setMode(RX_STG_EN, OUTPUT);

    gpio_clearPin(PLL_PWR);    /* PLL off                                           */
    gpio_setPin(VCOVCC_SW);    /* VCOVCC high enables RX VCO, TX VCO if low         */
    gpio_clearPin(WN_SW);      /* 25kHz band (?)                                    */
    gpio_clearPin(DMR_SW);     /* Disconnect HR_C5000 input IF signal and audio out */
    gpio_clearPin(FM_SW);      /* Disconnect analog FM audio path                   */
    gpio_clearPin(RF_APC_SW);  /* Disable RF power control                          */
    gpio_clearPin(TX_STG_EN);  /* Disable TX power stage                            */
    gpio_clearPin(RX_STG_EN);  /* Disable RX input stage                            */

    /*
     * Configure audio control GPIOs
     */
    gpio_setMode(SPK_MUTE, OUTPUT);
    gpio_setMode(AMP_EN,   OUTPUT);
    gpio_setMode(FM_MUTE,  OUTPUT);
    gpio_setMode(MIC_PWR,  OUTPUT);

    gpio_setPin(MIC_PWR);       /* Turn on microphone                   */
    gpio_setPin(AMP_EN);        /* Turn on audio amplifier              */
    gpio_setPin(FM_MUTE);       /* Unmute path from AF_out to amplifier */
    gpio_setPin(SPK_MUTE);      /* Mute speaker                         */

    /*
     * Configure and enble DAC
     */
    gpio_setMode(APC_TV,    INPUT_ANALOG);
    gpio_setMode(MOD2_BIAS, INPUT_ANALOG);

    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR = DAC_CR_EN2 | DAC_CR_EN1;
    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;

    /*
     * Load calibration data
     */
    calData = ((const md3x0Calib_t *) platform_getCalibrationData());

    /*
     * Enable and configure PLL
     */
    gpio_setPin(PLL_PWR);
    pll_init();

    /*
     * Configure HR_C5000
     */
    C5000_init();

    /*
     * Modulation bias settings
     */
    DAC->DHR12R2 = (calData->freqAdjustMid)*4 + 0x600; /* Original FW does this */
    C5000_setModOffset(calData->freqAdjustMid);

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
    rtxStatus.rxTone      = 0;
    rtxStatus.txTone      = 0;
}

void rtx_terminate()
{
    pll_terminate();

    gpio_clearPin(PLL_PWR);    /* PLL off                                           */
    gpio_clearPin(DMR_SW);     /* Disconnect HR_C5000 input IF signal and audio out */
    gpio_clearPin(FM_SW);      /* Disconnect analog FM audio path                   */
    gpio_clearPin(RF_APC_SW);  /* Disable RF power control                          */
    gpio_clearPin(TX_STG_EN);  /* Disable TX power stage                            */
    gpio_clearPin(RX_STG_EN);  /* Disable RX input stage                            */

    gpio_clearPin(MIC_PWR);    /* Turn off microphone                */
    gpio_clearPin(AMP_EN);     /* Turn off audio amplifier           */
    gpio_clearPin(FM_MUTE);    /* Mute path from AF_out to amplifier */

    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;

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
            _updateC5000IQparams();
            _setCTCSS();
            _updateSqlThresholds();
            _setVcoFrequency();

            /* TODO: temporarily force to RX mode if rtx is off. */
            if(rtxStatus.opStatus == OFF) _enableRxStage();
        }
    }

    if(rtxStatus.opStatus == RX)
    {
        /* Convert back voltage to ADC counts */
        float sqlValue = (adc1_getMeasurement(1) * 4096.0f)/3300.0f;
        uint16_t sqlLevel = ((uint16_t) sqlValue) >> 6;

        if((gpio_readPin(SPK_MUTE) == 1) && (sqlLevel > sqlOpenTsh))
        {
            gpio_clearPin(SPK_MUTE);
            platform_ledOn(GREEN);
        }

        if((gpio_readPin(SPK_MUTE) == 0) && (sqlLevel < sqlCloseTsh))
        {
            gpio_setPin(SPK_MUTE);
            platform_ledOff(GREEN);
        }
    }

    if(platform_getPttStatus() && (rtxStatus.opStatus != TX))
    {
        _disableRtxStages();
        toneGen_toneOn();
        gpio_setPin(MIC_PWR);
        C5000_startAnalogTx();
        _enableTxStage();
        platform_ledOn(RED);
    }

    if(!platform_getPttStatus() && (rtxStatus.opStatus == TX))
    {
        _disableRtxStages();
        gpio_clearPin(MIC_PWR);
        toneGen_toneOff();
        C5000_stopAnalogTx();
        _enableRxStage();
        platform_ledOff(RED);
    }
}

float rtx_getRssi()
{
    return adc1_getMeasurement(1);
}
