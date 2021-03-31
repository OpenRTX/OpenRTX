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
#include <interfaces/gpio.h>
#include <calibInfo_MDx.h>
#include <calibUtils.h>
#include <hwconfig.h>
#include "HR_C6000.h"
#include "AT1846S.h"

const mduv3x0Calib_t *calData; /* Pointer to calibration data                   */

int8_t  currRxBand = -1; /* Current band for RX                                 */
int8_t  currTxBand = -1; /* Current band for TX                                 */
uint8_t txpwr_lo = 0;    /* APC voltage for TX output power control, low power  */
uint8_t txpwr_hi = 0;    /* APC voltage for TX output power control, high power */
tone_t  tx_tone  = 0;
tone_t  rx_tone  = 0;

enum opmode currOpMode;  /* Current operating mode, needed for TX control       */

/**
 * \internal
 * Function to identify the current band (VHF or UHF), given an input frequency.
 *
 * @param freq frequency in Hz.
 * @return 0 if the frequency is in the VHF band,
 *         1 if the frequency is in the UHF band,
 *        -1 if the band to which the frequency belongs is neither VHF nor UHF.
 */
int8_t _getBandFromFrequency(freq_t freq)
{
    if((freq >= FREQ_LIMIT_VHF_LO) && (freq <= FREQ_LIMIT_VHF_HI)) return 0;
    if((freq >= FREQ_LIMIT_UHF_LO) && (freq <= FREQ_LIMIT_UHF_HI)) return 1;
    return -1;
}

void radio_init()
{
    /*
     * Load calibration data
     */
    calData = ((const mduv3x0Calib_t *) platform_getCalibrationData());

    /*
     * Configure RTX GPIOs
     */
    gpio_setMode(VHF_LNA_EN,   OUTPUT);
    gpio_setMode(UHF_LNA_EN,   OUTPUT);
    gpio_setMode(PA_EN_1,      OUTPUT);
    gpio_setMode(PA_EN_2,      OUTPUT);
    gpio_setMode(PA_SEL_SW,    OUTPUT);

    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);
    gpio_clearPin(PA_SEL_SW);

    /* TODO: keep audio connected to HR_C6000, for volume control */
    gpio_setMode(RX_AUDIO_MUX, OUTPUT);
    gpio_setPin(RX_AUDIO_MUX);

    /*
     * Configure and enable DAC
     */
    gpio_setMode(APC_REF, INPUT_ANALOG);

    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR = DAC_CR_EN1;
    DAC->DHR12R1 = 0;

    /*
     * Configure AT1846S and HR_C6000
     */
    AT1846S_init();
    C6000_init();
}

void radio_terminate()
{
    radio_disableRtx();
    C6000_terminate();
}

void radio_setBandwidth(const enum bandwidth bw)
{
    switch(bw)
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

void radio_setOpmode(const enum opmode mode)
{
    currOpMode = mode;
    switch(mode)
    {
        case FM:
            AT1846S_setOpMode(AT1846S_OP_FM);
            C6000_fmMode();
            break;

        case DMR:
            AT1846S_setOpMode(AT1846S_OP_DMR);
            C6000_dmrMode();
            break;

        default:
            break;
    }
}

void radio_setVcoFrequency(const freq_t frequency, const bool isTransmitting)
{
    (void) isTransmitting;
    AT1846S_setFrequency(frequency);
}

void radio_setCSS(const tone_t rxCss, const tone_t txCss)
{
    rx_tone = rxCss;
    tx_tone = txCss;
}

bool radio_checkRxDigitalSquelch()
{
    return true;
}

void radio_enableRx()
{
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    DAC->DHR12R1 = 0;

    if(currRxBand < 0) return;

    AT1846S_setFuncMode(AT1846S_RX);

    if(currRxBand == 0)
    {
        gpio_setPin(VHF_LNA_EN);
    }
    else
    {
        gpio_setPin(UHF_LNA_EN);
    }
}

void radio_enableTx(const float txPower, const bool enableCss)
{
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);

    if(currTxBand < 0) return;

    /*
     * TODO: increase granularity
     */
    uint8_t power = (txPower > 1.0f) ? txpwr_hi : txpwr_lo;
    DAC->DHR12L1 = power * 0xFF;

    if(currOpMode == FM)
    {
        C6000_startAnalogTx();
    }

    AT1846S_setFuncMode(AT1846S_TX);

    gpio_setPin(PA_EN_1);

    if(currTxBand == 0)
    {
        gpio_clearPin(PA_SEL_SW);
    }
    else
    {
        gpio_setPin(PA_SEL_SW);
    }

    gpio_setPin(PA_EN_2);

    if(enableCss)
    {
        AT1846S_enableTxCtcss(tx_tone);
    }
}

void radio_disableRtx()
{
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);
    DAC->DHR12L1 = 0;
    AT1846S_disableCtcss();
    AT1846S_setFuncMode(AT1846S_OFF);
    C6000_stopAnalogTx();
}

void radio_updateCalibrationParams(const rtxStatus_t *rtxCfg)
{
    currRxBand = _getBandFromFrequency(rtxCfg->rxFrequency);
    currTxBand = _getBandFromFrequency(rtxCfg->txFrequency);

    if((currRxBand < 0) || (currTxBand < 0)) return;

    /* TCXO bias voltage */
    uint8_t modBias = calData->vhfCal.freqAdjustMid;
    if(currRxBand > 0) modBias = calData->uhfCal.freqAdjustMid;
    C6000_setModOffset(modBias);

    /*
     * Discarding "const" qualifier to suppress compiler warnings.
     * This operation is safe anyway because calibration data is only read.
     */
    mduv3x0Calib_t *cal  = ((mduv3x0Calib_t *) calData);
    freq_t  *txCalPoints = cal->vhfCal.txFreq;
    uint8_t *loPwrCal    = cal->vhfCal.txLowPower;
    uint8_t *hiPwrCal    = cal->vhfCal.txHighPower;
    uint8_t *qRangeCal   = (rtxCfg->opMode == FM) ? cal->vhfCal.analogSendQrange
                                                  : cal->vhfCal.sendQrange;

    if(currTxBand > 0)
    {
        txCalPoints = cal->uhfCal.txFreq;
        loPwrCal    = cal->uhfCal.txLowPower;
        hiPwrCal    = cal->uhfCal.txHighPower;
        qRangeCal   = (rtxCfg->opMode == FM) ? cal->uhfCal.analogSendQrange
                                             : cal->uhfCal.sendQrange;
    }

    /* APC voltage for TX output power control */
    txpwr_lo = interpCalParameter(rtxCfg->txFrequency, txCalPoints, loPwrCal, 9);
    txpwr_hi = interpCalParameter(rtxCfg->txFrequency, txCalPoints, hiPwrCal, 9);

    /* HR_C6000 modulation amplitude */
    uint8_t Q = interpCalParameter(rtxCfg->txFrequency, txCalPoints, qRangeCal, 9);
    C6000_setModAmplitude(0, Q);
}

float radio_getRssi(const freq_t rxFreq)
{
    (void) rxFreq;

    /*
     * RSSI and SNR are packed in a 16-bit value, with RSSI being the upper
     * eight bits.
     */
    uint16_t val = (AT1846S_readRSSI() >> 8);
    int16_t rssi   = -151 + ((int16_t) val);
    return ((float) rssi);
}
