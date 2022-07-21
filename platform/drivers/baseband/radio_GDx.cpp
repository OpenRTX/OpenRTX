/***************************************************************************
 *   Copyright (C) 2021 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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

#include <hwconfig.h>

#include <algorithm>

#include "AT1846S.h"
#include "HR_C6000.h"
#include "calibration/calibInfo_GDx.h"
#include "core/utils.h"
#include "interfaces/gpio.h"
#include "interfaces/platform.h"
#include "interfaces/radio.h"
#include "radioUtils.h"

const gdxCalibration_t *calData;  // Pointer to calibration data
const rtxStatus_t      *config;   // Pointer to data structure with radio configuration

Band    currRxBand  = BND_NONE;   // Current band for RX
Band    currTxBand  = BND_NONE;   // Current band for TX
uint16_t apcVoltage = 0;          // APC voltage for TX output power control

enum opstatus radioStatus;        // Current operating status

HR_C6000& C6000  = HR_C6000::instance();  // HR_C5000 driver
AT1846S& at1846s = AT1846S::instance();   // AT1846S driver

void radio_init(const rtxStatus_t *rtxState)
{
    /*
     * Load calibration data
     */
    calData = reinterpret_cast< const gdxCalibration_t * >(platform_getCalibrationData());

    config      = rtxState;
    radioStatus = OFF;

    /*
     * Configure RTX GPIOs
     */
    gpio_setMode(VHF_LNA_EN, OUTPUT);
    gpio_setMode(UHF_LNA_EN, OUTPUT);
    gpio_setMode(VHF_PA_EN,  OUTPUT);
    gpio_setMode(UHF_PA_EN,  OUTPUT);
    gpio_setMode(RX_AUDIO_MUX, OUTPUT);
    gpio_setMode(TX_AUDIO_MUX, OUTPUT);


    gpio_clearPin(VHF_LNA_EN);      // Turn VHF LNA off
    gpio_clearPin(UHF_LNA_EN);      // Turn UHF LNA off
    gpio_clearPin(VHF_PA_EN);       // Turn VHF PA off
    gpio_clearPin(UHF_PA_EN);       // Turn UHF PA off
    gpio_clearPin(RX_AUDIO_MUX);    // Audio out to HR_C6000
    gpio_clearPin(TX_AUDIO_MUX);    // Audio in to microphone

    /*
     * Enable and configure DAC for PA drive control
     */
    SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;
    DAC0->DAT[0].DATL = 0;
    DAC0->DAT[0].DATH = 0;
    DAC0->C0   |= DAC_C0_DACRFS_MASK    // Reference voltage is Vref2
               |  DAC_C0_DACEN_MASK;    // Enable DAC

    /*
     * Enable and configure both AT1846S and HR_C6000
     */
    at1846s.init();
    C6000.init();
}

void radio_terminate()
{
    radioStatus = OFF;
    radio_disableRtx();
    C6000.terminate();

    DAC0->DAT[0].DATL = 0;
    DAC0->DAT[0].DATH = 0;
    SIM->SCGC6 &= ~SIM_SCGC6_DAC0_MASK;
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
            gpio_setPin(RX_AUDIO_MUX);              // Audio out to amplifier
            gpio_clearPin(TX_AUDIO_MUX);            // Audio in to microphone
            at1846s.setOpMode(AT1846S_OpMode::FM);
            break;

        case OPMODE_DMR:
            gpio_clearPin(RX_AUDIO_MUX);             // Audio out to HR_C6000
            gpio_setPin(TX_AUDIO_MUX);               // Audio in from HR_C6000
            at1846s.setOpMode(AT1846S_OpMode::DMR);
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
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);
    DAC0->DAT[0].DATH = 0;
    DAC0->DAT[0].DATL = 0;

    if(currRxBand == BND_NONE) return;

    // Adjust reference oscillator bias and offset.
    C6000.writeCfgRegister(0x04, calData->data[currRxBand].mod2Offset);
    C6000.setModOffset(calData->data[currRxBand].modBias);

    // Set frequency and enable AT1846S RX
    at1846s.setFrequency(config->rxFrequency);
    at1846s.setFuncMode(AT1846S_FuncMode::RX);

    // Enable RX LNA
    if(currRxBand == BND_VHF)
    {
        gpio_setPin(VHF_LNA_EN);
    }
    else
    {
        gpio_setPin(UHF_LNA_EN);
    }

    radioStatus = RX;

    if(config->rxToneEn)
    {
        at1846s.enableRxCtcss(config->rxTone);
    }
}

void radio_enableTx()
{
    if(config->txDisable == 1) return;

    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);

    if(currTxBand == BND_NONE) return;

    // Adjust reference oscillator bias and offset.
    C6000.writeCfgRegister(0x04, calData->data[currTxBand].mod2Offset);
    C6000.setModOffset(calData->data[currTxBand].modBias);

    // Set frequency and enable AT1846S TX
    at1846s.setFrequency(config->txFrequency);
    at1846s.setFuncMode(AT1846S_FuncMode::TX);

    // Set APC voltage
    DAC0->DAT[0].DATH = (apcVoltage >> 8) & 0xFF;
    DAC0->DAT[0].DATL =  apcVoltage & 0xFF;

    // Enable TX PA
    if(currTxBand == BND_VHF)
    {
        gpio_setPin(VHF_PA_EN);
    }
    else
    {
        gpio_setPin(UHF_PA_EN);
    }

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
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);

    if(radioStatus == TX)
    {
        // Set PA drive voltage to 0V
        DAC0->DAT[0].DATH = 0;
        DAC0->DAT[0].DATL = 0;
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
     * Parameters dependent on RX frequency only
     */
    const bandCalData_t *cal = &(calData->data[currRxBand]);

    at1846s.setRxAudioGain(cal->rxDacGain, cal->rxVoiceGain);

    if(config->bandwidth == BW_12_5)
    {
        at1846s.setNoise1Thresholds(cal->noise1_HighTsh_Nb, cal->noise1_LowTsh_Nb);
        at1846s.setNoise2Thresholds(cal->noise2_HighTsh_Nb, cal->noise2_LowTsh_Nb);
        at1846s.setRssiThresholds(cal->rssi_HighTsh_Nb, cal->rssi_LowTsh_Nb);
    }
    else
    {
        at1846s.setNoise1Thresholds(cal->noise1_HighTsh_Wb, cal->noise1_LowTsh_Wb);
        at1846s.setNoise2Thresholds(cal->noise2_HighTsh_Wb, cal->noise2_LowTsh_Wb);
        at1846s.setRssiThresholds(cal->rssi_HighTsh_Wb, cal->rssi_LowTsh_Wb);
    }

    C6000.writeCfgRegister(0x37, cal->digAudioGain);    // DACDATA gain

    uint8_t sqlTresh = 0;
    if(currRxBand == BND_VHF)
    {
        sqlTresh = interpCalParameter(config->rxFrequency, calData->vhfCalPoints,
                                      cal->analogSqlThresh, 8);
    }
    else
    {
        sqlTresh = interpCalParameter(config->rxFrequency, calData->uhfCalPoints,
                                      cal->analogSqlThresh, 8);
    }

    at1846s.setAnalogSqlThresh(sqlTresh);

    /*
     * Parameters dependent on TX frequency only
     */
    at1846s.setPgaGain(calData->data[currTxBand].PGA_gain);
    at1846s.setMicGain(calData->data[currTxBand].analogMicGain);
    at1846s.setAgcGain(calData->data[currTxBand].rxAGCgain);
    at1846s.setPaDrive(calData->data[currTxBand].PA_drv);

    uint8_t mod1Amp  = 0;
    uint8_t txpwr_lo = 0;
    uint8_t txpwr_hi = 0;

    if(currTxBand == BND_VHF)
    {
        /* VHF band */
        txpwr_lo = interpCalParameter(config->txFrequency, calData->vhfCalPoints,
                                      calData->data[currTxBand].txLowPower, 8);

        txpwr_hi = interpCalParameter(config->txFrequency, calData->vhfCalPoints,
                                      calData->data[currTxBand].txHighPower, 8);

        mod1Amp = interpCalParameter(config->txFrequency, calData->vhfCalPoints,
                                     cal->mod1Amplitude, 8);
    }
    else
    {
        /* UHF band */
        txpwr_lo = interpCalParameter(config->txFrequency, calData->uhfPwrCalPoints,
                                      calData->data[currTxBand].txLowPower, 16);

        txpwr_hi = interpCalParameter(config->txFrequency, calData->uhfPwrCalPoints,
                                      calData->data[currTxBand].txHighPower, 16);

        mod1Amp = interpCalParameter(config->txFrequency, calData->uhfCalPoints,
                                     cal->mod1Amplitude, 8);
    }

    C6000.setModAmplitude(0, mod1Amp);

    // Calculate APC voltage, constraining output power between 1W and 5W.
    float power = std::max(std::min(config->txPower, 5.0f), 1.0f);
    float pwrHi = static_cast< float >(txpwr_hi);
    float pwrLo = static_cast< float >(txpwr_lo);
    float apc   = pwrLo + (pwrHi - pwrLo)/4.0f*(power - 1.0f);
    apcVoltage  = static_cast< uint16_t >(apc) * 16;

    // Set bandwidth and TX deviation, force 12.5kHz for DMR mode
    if((config->bandwidth == BW_12_5) || (config->opMode == OPMODE_DMR))
    {
        at1846s.setBandwidth(AT1846S_BW::_12P5);
        at1846s.setTxDeviation(calData->data[currTxBand].mixGainNarrowband);
    }
    else
    {
        at1846s.setBandwidth(AT1846S_BW::_25);
        at1846s.setTxDeviation(calData->data[currTxBand].mixGainWideband);
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
    return static_cast< float >(at1846s.readRSSI());
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
