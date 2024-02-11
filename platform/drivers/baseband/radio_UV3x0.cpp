/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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

#include <interfaces/delays.h>
#include <interfaces/nvmem.h>
#include <interfaces/radio.h>
#include <peripherals/gpio.h>
#include <calibInfo_MDx.h>
#include <hwconfig.h>
#include <algorithm>
#include <utils.h>
#include "radioUtils.h"
#include "HR_C6000.h"
#include "AT1846S.h"


static const rtxStatus_t *config;                // Pointer to data structure with radio configuration

static mduv3x0Calib_t calData;                   // Calibration data
static Band    currRxBand = BND_NONE;            // Current band for RX
static Band    currTxBand = BND_NONE;            // Current band for TX
static uint8_t txpwr_lo   = 0;                   // APC voltage for TX output power control, low power
static uint8_t txpwr_hi   = 0;                   // APC voltage for TX output power control, high power
static uint8_t rxModBias  = 0;                   // VCXO bias for RX
static uint8_t txModBias  = 0;                   // VCXO bias for TX

static enum opstatus radioStatus;                // Current operating status

static HR_C6000& C6000  = HR_C6000::instance();  // HR_C5000 driver
static AT1846S& at1846s = AT1846S::instance();   // AT1846S driver

void radio_init(const rtxStatus_t *rtxState)
{
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
     * Load calibration data
     */
    nvm_readCalibData(&calData);

    /*
     * Configure AT1846S and HR_C6000, keep AF output disabled at power on.
     */
    at1846s.init();
    C6000.init();
    radio_disableAfOutput();
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
            at1846s.setBandwidth(AT1846S_BW::_12P5);
//             C6000.dmrMode();
            break;

        case OPMODE_M17:
            at1846s.setOpMode(AT1846S_OpMode::DMR); // AT1846S in DMR mode, disables RX filter
            at1846s.setBandwidth(AT1846S_BW::_25);  // Set bandwidth to 25kHz for proper deviation
            C6000.fmMode();                         // HR_C6000 in FM mode
            C6000.setInputGain(+6);                 // Input gain in dB, found experimentally
            break;

        default:
            break;
    }
}

bool radio_checkRxDigitalSquelch()
{
    return at1846s.rxCtcssDetected();
}

void radio_enableAfOutput()
{
    // Bit 2 of register 0x36: enable voice channel in FM mode
    // TODO: AF output management for DMR mode
    C6000.writeCfgRegister(0x36, 0x02);
}

void radio_disableAfOutput()
{
    C6000.writeCfgRegister(0x36, 0x00);
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

    /*
     * Force silencing of audio output when RX is enabled with M17 operating
     * mode selected. Avoids the spillover of baseband signal towards the
     * speaker.
     *
     * TODO: improve this solution.
     */
    if(config->opMode == OPMODE_M17)
    {
        C6000.writeCfgRegister(0xE0, 0x00);
    }

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
    float power  = static_cast < float >(config->txPower) / 1000.0f;
          power  = std::max(std::min(power, 5.0f), 1.0f);
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

    //
    // FIXME: workaround to fix a small carrier-only gap which appears at the
    // beginning of each transmission. This problem is particularly evident in
    // M17 mode because it causes the truncation of the preamble sequence.
    //
    sleepFor(0, 50);

    at1846s.setFuncMode(AT1846S_FuncMode::TX);


    if(currTxBand == BND_VHF)
    {
        gpio_clearPin(PA_SEL_SW);
    }
    else
    {
        gpio_setPin(PA_SEL_SW);
    }

    gpio_setPin(PA_EN_1);
    gpio_setPin(PA_EN_2);

    if(config->txToneEn)
    {
        at1846s.enableTxCtcss(config->txTone);
    }

    if (config->toneEn)
    {
        at1846s.enableTone(17500);
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

    at1846s.disableTone();
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
    txModBias = calData.vhfCal.freqAdjustMid;
    rxModBias = calData.vhfCal.freqAdjustMid;
    if(currRxBand == BND_UHF) rxModBias = calData.uhfCal.freqAdjustMid;
    if(currTxBand == BND_UHF) txModBias = calData.uhfCal.freqAdjustMid;

    uint8_t calPoints    = 5;
    freq_t  *txCalPoints = calData.vhfCal.txFreq;
    uint8_t *loPwrCal    = calData.vhfCal.txLowPower;
    uint8_t *hiPwrCal    = calData.vhfCal.txHighPower;
    uint8_t *qRangeCal   = (config->opMode == OPMODE_FM)
                         ? calData.vhfCal.analogSendQrange
                         : calData.vhfCal.sendQrange;

    if(currTxBand == BND_UHF)
    {
        calPoints   = 9;
        txCalPoints = calData.uhfCal.txFreq;
        loPwrCal    = calData.uhfCal.txLowPower;
        hiPwrCal    = calData.uhfCal.txHighPower;
        qRangeCal   = (config->opMode == OPMODE_FM)
                    ? calData.uhfCal.analogSendQrange
                    : calData.uhfCal.sendQrange;
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

    // Set bandwidth, only for analog FM mode
    if(config->opMode == OPMODE_FM)
    {
        switch(config->bandwidth)
        {
            case BW_12_5:
                at1846s.setBandwidth(AT1846S_BW::_12P5);
                break;

             case BW_25:
                at1846s.setBandwidth(AT1846S_BW::_25);
                break;

             default:
                 break;
        }
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

rssi_t radio_getRssi()
{
    return static_cast< rssi_t >(at1846s.readRSSI());
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
