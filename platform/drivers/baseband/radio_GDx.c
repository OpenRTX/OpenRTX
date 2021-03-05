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
#include <calibInfo_GDx.h>
#include <calibUtils.h>
#include <hwconfig.h>
#include <string.h>
#include "HR_C6000.h"
#include "AT1846S.h"

const gdxCalibration_t *calData;  /* Pointer to calibration data        */

int8_t  currRxBand = -1; /* Current band for RX                                 */
int8_t  currTxBand = -1; /* Current band for TX                                 */
uint8_t txpwr_lo = 0;    /* APC voltage for TX output power control, low power  */
uint8_t txpwr_hi = 0;    /* APC voltage for TX output power control, high power */
tone_t  tx_tone  = 0;
tone_t  rx_tone  = 0;

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
    calData = ((const gdxCalibration_t *) platform_getCalibrationData());

    /*
     * Configure RTX GPIOs
     */
    gpio_setMode(VHF_LNA_EN, OUTPUT);
    gpio_setMode(UHF_LNA_EN, OUTPUT);
    gpio_setMode(VHF_PA_EN,  OUTPUT);
    gpio_setMode(UHF_PA_EN,  OUTPUT);
    gpio_setMode(RX_AUDIO_MUX, OUTPUT);
    gpio_setMode(TX_AUDIO_MUX, OUTPUT);


    gpio_clearPin(VHF_LNA_EN);      /* Turn VHF LNA off       */
    gpio_clearPin(UHF_LNA_EN);      /* Turn UHF LNA off       */
    gpio_clearPin(VHF_PA_EN);       /* Turn VHF PA off        */
    gpio_clearPin(UHF_PA_EN);       /* Turn UHF PA off        */
    gpio_clearPin(RX_AUDIO_MUX);    /* Audio out to HR_C6000  */
    gpio_clearPin(TX_AUDIO_MUX);    /* Audio in to microphone */

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
    switch(mode)
    {
        case FM:
            gpio_setPin(RX_AUDIO_MUX);          /* Audio out to amplifier */
            gpio_clearPin(TX_AUDIO_MUX);        /* Audio in to microphone */
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
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);
    DAC0->DAT[0].DATH = 0;
    DAC0->DAT[0].DATL = 0;

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
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);

    if(currTxBand < 0) return;

    /*
     * TODO: increase granularity
     */
    uint16_t power = (txPower > 1.0f) ? txpwr_hi : txpwr_lo;
    power *= 16;
    DAC0->DAT[0].DATH = (power >> 8) & 0xFF;
    DAC0->DAT[0].DATL =  power & 0xFF;

    AT1846S_setFuncMode(AT1846S_TX);

    if(currTxBand == 0)
    {
        gpio_setPin(VHF_PA_EN);
    }
    else
    {
        gpio_setPin(UHF_PA_EN);
    }

    if(enableCss)
    {
        AT1846S_enableTxCtcss(tx_tone);
    }
}

void radio_disableRtx()
{
    gpio_clearPin(VHF_LNA_EN);
    gpio_clearPin(UHF_LNA_EN);
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);
    AT1846S_disableCtcss();
    AT1846S_setFuncMode(AT1846S_OFF);
}

void radio_updateCalibrationParams(const rtxStatus_t *rtxCfg)
{
    currRxBand = _getBandFromFrequency(rtxCfg->rxFrequency);
    currTxBand = _getBandFromFrequency(rtxCfg->txFrequency);

    if((currRxBand < 0) || (currTxBand < 0)) return;

    /*
     * Parameters dependent on RX frequency
     */
    const bandCalData_t *cal = &(calData->data[currRxBand]);

    AT1846S_setPgaGain(cal->PGA_gain);
    AT1846S_setMicGain(cal->analogMicGain);
    AT1846S_setAgcGain(cal->rxAGCgain);
    AT1846S_setRxAudioGain(cal->rxAudioGainWideband, cal->rxAudioGainNarrowband);
    AT1846S_setPaDrive(cal->PA_drv);

    if(rtxCfg->bandwidth == BW_12_5)
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

    uint8_t sqlTresh = 0;
    if(currRxBand == 0)
    {
        sqlTresh = interpCalParameter(rtxCfg->rxFrequency, calData->vhfCalPoints,
                                      cal->analogSqlThresh, 8);
    }
    else
    {
        sqlTresh = interpCalParameter(rtxCfg->rxFrequency, calData->uhfMod1CalPoints,
                                      cal->analogSqlThresh, 8);
    }

    AT1846S_setAnalogSqlThresh(sqlTresh);

    /*
     * Parameters dependent on TX frequency
     */
    uint8_t mod1Amp  = 0;

    if(currTxBand == 0)
    {
        /* VHF band */
        txpwr_lo = interpCalParameter(rtxCfg->txFrequency, calData->vhfCalPoints,
                                      calData->data[currTxBand].txLowPower, 8);

        txpwr_hi = interpCalParameter(rtxCfg->txFrequency, calData->vhfCalPoints,
                                      calData->data[currTxBand].txHighPower, 8);

        mod1Amp = interpCalParameter(rtxCfg->txFrequency, calData->vhfCalPoints,
                                     cal->mod1Amplitude, 8);
    }
    else
    {
        /* UHF band */
        txpwr_lo = interpCalParameter(rtxCfg->txFrequency, calData->uhfPwrCalPoints,
                                      calData->data[currTxBand].txLowPower, 16);

        txpwr_hi = interpCalParameter(rtxCfg->txFrequency, calData->uhfPwrCalPoints,
                                      calData->data[currTxBand].txHighPower, 16);

        mod1Amp = interpCalParameter(rtxCfg->txFrequency, calData->uhfMod1CalPoints,
                                     cal->mod1Amplitude, 8);
    }

    C6000_setModAmplitude(0, mod1Amp);
}

float radio_getRssi(const freq_t rxFreq)
{
    (void) rxFreq;

    uint16_t val = AT1846S_readRSSI();
    int8_t rssi   = -151 + (val >> 8);
    return ((float) rssi);
}
