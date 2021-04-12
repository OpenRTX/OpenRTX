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
#include <toneGenerator_MDx.h>
#include <interfaces/radio.h>
#include <interfaces/gpio.h>
#include <calibInfo_MDx.h>
#include <calibUtils.h>
#include <hwconfig.h>
#include <ADC1_MDx.h>
#include <string.h>
#include <stdlib.h>
#include "HR_C5000.h"
#include "SKY72310.h"

static const freq_t IF_FREQ = 49950000;  /* Intermediate frequency: 49.95MHz   */

const md3x0Calib_t *calData;             /* Pointer to calibration data        */

uint8_t vtune_rx = 0;   /* Tuning voltage for RX input filter                  */
uint8_t txpwr_lo = 0;   /* APC voltage for TX output power control, low power  */
uint8_t txpwr_hi = 0;   /* APC voltage for TX output power control, high power */

enum opmode currOpMode; /* Current operating mode, needed for TX control       */

/*
 * Parameters for RSSI voltage (mV) to input power (dBm) conversion.
 * Gain is constant, while offset values are aligned to calibration frequency
 * test points.
 * Thanks to Wojciech SP5WWP for the measurements!
 */
float rssi_gain = 22.0f;
float rssi_offset[] = {3277.618f, 3654.755f, 3808.191f,
                       3811.318f, 3804.936f, 3806.591f,
                       3723.882f, 3621.373f, 3559.782f};

void radio_init()
{
    /*
     * Load calibration data
     */
    calData = ((const md3x0Calib_t *) platform_getCalibrationData());

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

    gpio_setMode(FM_MUTE,  OUTPUT);
    gpio_clearPin(FM_MUTE);

    gpio_clearPin(PLL_PWR);    /* PLL off                                           */
    gpio_setPin(VCOVCC_SW);    /* VCOVCC high enables RX VCO, TX VCO if low         */
    gpio_setPin(WN_SW);        /* 25kHz bandwidth                                   */
    gpio_clearPin(DMR_SW);     /* Disconnect HR_C5000 input IF signal and audio out */
    gpio_clearPin(FM_SW);      /* Disconnect analog FM audio path                   */
    gpio_clearPin(RF_APC_SW);  /* Disable RF power control                          */
    gpio_clearPin(TX_STG_EN);  /* Disable TX power stage                            */
    gpio_clearPin(RX_STG_EN);  /* Disable RX input stage                            */

    /*
     * Configure and enable DAC
     */
    gpio_setMode(APC_TV,    INPUT_ANALOG);
    gpio_setMode(MOD2_BIAS, INPUT_ANALOG);

    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR = DAC_CR_EN2 | DAC_CR_EN1;
    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;

    /*
     * Enable and configure PLL
     */
    gpio_setPin(PLL_PWR);
    SKY73210_init();

    /*
     * Configure HR_C5000
     */
    C5000_init();

    /*
     * Modulation bias settings, as per TYT firmware.
     */
    DAC->DHR12R2 = (calData->freqAdjustMid)*4 + 0x600;
    C5000_setModOffset(calData->freqAdjustMid);
}

void radio_terminate()
{
    SKY73210_terminate();

    gpio_clearPin(PLL_PWR);    /* PLL off                                           */
    gpio_clearPin(DMR_SW);     /* Disconnect HR_C5000 input IF signal and audio out */
    gpio_clearPin(FM_SW);      /* Disconnect analog FM audio path                   */
    gpio_clearPin(RF_APC_SW);  /* Disable RF power control                          */
    gpio_clearPin(TX_STG_EN);  /* Disable TX power stage                            */
    gpio_clearPin(RX_STG_EN);  /* Disable RX input stage                            */

    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void radio_setBandwidth(const enum bandwidth bw)
{
    switch(bw)
    {
        case BW_12_5:
            gpio_clearPin(WN_SW);
            C5000_setModFactor(0x1E);
            break;

        case BW_20:
            gpio_setPin(WN_SW);
            C5000_setModFactor(0x30);
            break;

        case BW_25:
            gpio_setPin(WN_SW);
            C5000_setModFactor(0x3C);
            break;

        default:
            break;
    }
}

void radio_setOpmode(const enum opmode mode)
{
    currOpMode = mode;
    switch(mode)
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

void radio_setVcoFrequency(const freq_t frequency, const bool isTransmitting)
{
    float freq = ((float) frequency);

    if(!isTransmitting)
    {
        freq = freq - IF_FREQ;
    }

    SKY73210_setFrequency(freq, 5);
}

void radio_setCSS(const tone_t rxCss, const tone_t txCss)
{
    (void) rxCss;
    float tone = ((float) txCss) / 10.0f;
    toneGen_setToneFreq(tone);
}

bool radio_checkRxDigitalSquelch()
{
    return true;
}

void radio_enableRx()
{
    gpio_clearPin(TX_STG_EN);

    gpio_clearPin(RF_APC_SW);
    gpio_setPin(VCOVCC_SW);

    DAC->DHR12L1 = vtune_rx * 0xFF;

    gpio_setPin(RX_STG_EN);

    if(currOpMode == FM)
    {
        gpio_setPin(FM_MUTE);
    }
}

void radio_enableTx(const float txPower, const bool enableCss)
{
    gpio_clearPin(RX_STG_EN);

    gpio_setPin(RF_APC_SW);
    gpio_clearPin(VCOVCC_SW);

    /*
     * TODO: increase granularity
     */
    uint8_t apc = (txPower > 1.0f) ? txpwr_hi : txpwr_lo;
    DAC->DHR12L1 = apc * 0xFF;

    if(currOpMode == FM)
    {
        C5000_startAnalogTx();
    }

    gpio_setPin(TX_STG_EN);

    if(enableCss)
    {
        toneGen_toneOn();
    }
}

void radio_disableRtx()
{
    /* If we are currently transmitting, stop tone and C5000 TX */
    if(gpio_readPin(TX_STG_EN) == 1)
    {
        toneGen_toneOff();
        C5000_stopAnalogTx();
    }

    gpio_clearPin(TX_STG_EN);
    gpio_clearPin(RX_STG_EN);
    gpio_clearPin(FM_MUTE);
}

void radio_updateCalibrationParams(const rtxStatus_t* rtxCfg)
{
    /* Tuning voltage for RX input filter */
    vtune_rx = interpCalParameter(rtxCfg->rxFrequency, calData->rxFreq,
                                  calData->rxSensitivity, 9);

    /* APC voltage for TX output power control */
    txpwr_lo = interpCalParameter(rtxCfg->txFrequency, calData->txFreq,
                                  calData->txLowPower, 9);

    txpwr_hi = interpCalParameter(rtxCfg->txFrequency, calData->txFreq,
                                  calData->txHighPower, 9);

    /* HR_C5000 modulation amplitude */
    const uint8_t *Ical = calData->sendIrange;
    const uint8_t *Qcal = calData->sendQrange;

    if(rtxCfg->opMode == FM)
    {
        Ical = calData->analogSendIrange;
        Qcal = calData->analogSendQrange;
    }

    uint8_t I = interpCalParameter(rtxCfg->txFrequency, calData->txFreq, Ical, 9);
    uint8_t Q = interpCalParameter(rtxCfg->txFrequency, calData->txFreq, Qcal, 9);

    C5000_setModAmplitude(I, Q);
}

float radio_getRssi(const freq_t rxFreq)
{
    /*
     * On MD3x0 devices, RSSI value is get by reading the analog RSSI output
     * from second IF stage (GT3136 IC).
     * The corresponding power value is obtained through the linear correlation
     * existing between measured voltage in mV and power in dBm. While gain is
     * constant, offset depends from the rx frequency.
     */

    uint32_t offset_index = (rxFreq - 400035000)/10000000;

    if(rxFreq < 401035000) offset_index = 0;
    if(rxFreq > 479995000) offset_index = 8;

    float rssi_mv  = adc1_getMeasurement(ADC_RSSI_CH);
    float rssi_dbm = (rssi_mv - rssi_offset[offset_index]) / rssi_gain;
    return rssi_dbm;
}
