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
#include <interfaces/radio.h>
#include <interfaces/gpio.h>
#include <calibInfo_MDx.h>
#include <calibUtils.h>
#include <hwconfig.h>
#include <algorithm>
#include "radioUtils.h"
#include "HR_C6000.h"
#include "AT1846S.h"


const mduv3x0Calib_t *calData;  // Pointer to calibration data
const rtxStatus_t    *config;   // Pointer to data structure with radio configuration

Band    currRxBand = BND_NONE;  // Current band for RX
Band    currTxBand = BND_NONE;  // Current band for TX
uint8_t txpwr_lo   = 0;         // APC voltage for TX output power control, low power
uint8_t txpwr_hi   = 0;         // APC voltage for TX output power control, high power
uint8_t rxModBias  = 0;         // VCXO bias for RX
uint8_t txModBias  = 0;         // VCXO bias for TX

enum opstatus radioStatus;      // Current operating status

HR_C6000& C6000  = HR_C6000::instance();  // HR_C5000 driver
AT1846S& at1846s = AT1846S::instance();   // AT1846S driver

void radio_init(const rtxStatus_t *rtxState)
{
    /*
     * Load calibration data
     */
    calData = reinterpret_cast< const mduv3x0Calib_t * >(platform_getCalibrationData());

    config      = rtxState;
    radioStatus = OFF;

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

    // TODO: keep audio connected to HR_C6000, for volume control
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
    at1846s.init();
    C6000.init();
}

void radio_terminate()
{
    radio_disableRtx();
    C6000.terminate();
    at1846s.terminate();

    DAC->DHR12R1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    //TODO: this part will be implemented in the future, when proved to be
    // necessary.
    (void) vhfOffset;
    (void) uhfOffset;
}

void radio_setOpmode(const enum opmode mode)
{
    switch(mode)
    {
        case OPMODE_FM:
            at1846s.setOpMode(AT1846S_OpMode::FM);  // AT1846S in FM mode
            C6000.fmMode();                         // HR_C6000 in FM mode
            C6000.setInputGain(-3);                 // Input gain in dB, as per TYT firmware
            break;

        case OPMODE_DMR:
            at1846s.setOpMode(AT1846S_OpMode::DMR);
//             C6000.dmrMode();
            break;

        case OPMODE_M17:
            at1846s.setOpMode(AT1846S_OpMode::DMR); // AT1846S in DMR mode, disables RX filter
            C6000.fmMode();                         // HR_C6000 in FM mode
            C6000.setInputGain(+3);                 // Input gain in dB, found experimentally
            break;

        default:
            break;
    }
}

bool radio_checkRxDigitalSquelch()
{
    return at1846s.rxCtcssDetected();
}

void radio_enableRx()
{
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    DAC->DHR12R1 = 0;

    if(currRxBand == BND_NONE) return;

    C6000.setModOffset(rxModBias);
    at1846s.setFrequency(config->rxFrequency);
    at1846s.setFuncMode(AT1846S_FuncMode::RX);

    if(currRxBand == BND_VHF)
    {
        gpio_setPin(VHF_LNA_EN);
    }
    else
    {
        gpio_setPin(UHF_LNA_EN);
    }

    if(config->rxToneEn)
    {
        at1846s.enableRxCtcss(config->rxTone);
    }

    radioStatus = RX;
}

void radio_enableTx()
{
    if(config->txDisable == 1) return;

    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);

    C6000.setModOffset(txModBias);
    at1846s.setFrequency(config->txFrequency);

    // Constrain output power between 1W and 5W.
    float power  = std::max(std::min(config->txPower, 5.0f), 1.0f);
    float pwrHi  = static_cast< float >(txpwr_hi);
    float pwrLo  = static_cast< float >(txpwr_lo);
    float apc    = pwrLo + (pwrHi - pwrLo)/4.0f*(power - 1.0f);
    DAC->DHR12L1 = static_cast< uint8_t >(apc) * 0xFF;

    switch(config->opMode)
    {
        case OPMODE_FM:
        {
            FmConfig cfg = (config->bandwidth == BW_12_5) ? FmConfig::BW_12p5kHz
                                                          : FmConfig::BW_25kHz;
            C6000.startAnalogTx(TxAudioSource::MIC, cfg | FmConfig::PREEMPH_EN);
        }
            break;

        case OPMODE_M17:
            C6000.startAnalogTx(TxAudioSource::LINE_IN, FmConfig::BW_25kHz);
            break;

        default:
            break;
    }

    at1846s.setFuncMode(AT1846S_FuncMode::TX);

    gpio_setPin(PA_EN_1);

    if(currTxBand == BND_VHF)
    {
        gpio_clearPin(PA_SEL_SW);
    }
    else
    {
        gpio_setPin(PA_SEL_SW);
    }

    gpio_setPin(PA_EN_2);

    if(config->txToneEn)
    {
        at1846s.enableTxCtcss(config->txTone);
    }

    radioStatus = TX;
}

void radio_disableRtx()
{
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(PA_EN_1);
    gpio_clearPin(PA_EN_2);
    DAC->DHR12L1 = 0;

    // If we are currently transmitting, stop tone and C6000 TX
    if(radioStatus == TX)
    {
        C6000.stopAnalogTx();
    }

    at1846s.disableCtcss();
    at1846s.setFuncMode(AT1846S_FuncMode::OFF);
    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    currRxBand = getBandFromFrequency(config->rxFrequency);
    currTxBand = getBandFromFrequency(config->txFrequency);

    if((currRxBand == BND_NONE) || (currTxBand == BND_NONE)) return;

    /*
     * VCXO bias voltage, separated values for TX and RX to allow for cross-band
     * operation.
     */
    txModBias = calData->vhfCal.freqAdjustMid;
    rxModBias = calData->vhfCal.freqAdjustMid;
    if(currRxBand == BND_UHF) rxModBias = calData->uhfCal.freqAdjustMid;
    if(currTxBand == BND_UHF) txModBias = calData->uhfCal.freqAdjustMid;

    /*
     * Discarding "const" qualifier to suppress compiler warnings.
     * This operation is safe anyway because calibration data is only read.
     */
    mduv3x0Calib_t *cal  = const_cast< mduv3x0Calib_t * >(calData);
    uint8_t calPoints    = 5;
    freq_t  *txCalPoints = cal->vhfCal.txFreq;
    uint8_t *loPwrCal    = cal->vhfCal.txLowPower;
    uint8_t *hiPwrCal    = cal->vhfCal.txHighPower;
    uint8_t *qRangeCal   = (config->opMode == OPMODE_FM)
                         ? cal->vhfCal.analogSendQrange
                         : cal->vhfCal.sendQrange;

    if(currTxBand == BND_UHF)
    {
        calPoints   = 9;
        txCalPoints = cal->uhfCal.txFreq;
        loPwrCal    = cal->uhfCal.txLowPower;
        hiPwrCal    = cal->uhfCal.txHighPower;
        qRangeCal   = (config->opMode == OPMODE_FM)
                    ? cal->uhfCal.analogSendQrange
                    : cal->uhfCal.sendQrange;
    }

    // APC voltage for TX output power control
    txpwr_lo = interpCalParameter(config->txFrequency, txCalPoints, loPwrCal,
                                                                    calPoints);
    txpwr_hi = interpCalParameter(config->txFrequency, txCalPoints, hiPwrCal,
                                                                    calPoints);

    // HR_C6000 modulation amplitude
    uint8_t Q = interpCalParameter(config->txFrequency, txCalPoints, qRangeCal,
                                                                     calPoints);
    C6000.setModAmplitude(0, Q);

    // Set bandwidth, force 12.5kHz for DMR mode
    if((config->bandwidth == BW_12_5) || (config->opMode == DMR))
    {
        at1846s.setBandwidth(AT1846S_BW::_12P5);
    }
    else
    {
        at1846s.setBandwidth(AT1846S_BW::_25);
    }

    /*
     * Update VCO frequency and tuning parameters if current operating status
     * is different from OFF.
     * This is done by calling again the corresponding functions, which is safe
     * to do and avoids code duplication.
     */
    if(radioStatus == RX) radio_enableRx();
    if(radioStatus == TX) radio_enableTx();
}

float radio_getRssi()
{
    return static_cast< float > (at1846s.readRSSI());
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
