/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/radio.h"
#include "interfaces/nvmem.h"
#include "peripherals/gpio.h"
#include "calibration/calibInfo_GDx.h"
#include "hwconfig.h"
#include "drivers/SPI/spi_mk22.h"
#include <algorithm>
#include "core/utils.h"
#include "radioUtils.h"
#include "drivers/baseband/HR_C6000.h"
#include "drivers/baseband/AT1846S.h"

static const rtxStatus_t *config;                // Pointer to data structure with radio configuration

static gdxCalibration_t calData;                 // Calibration data
static Band    currRxBand  = BND_NONE;           // Current band for RX
static Band    currTxBand  = BND_NONE;           // Current band for TX
static uint16_t apcVoltage = 0;                  // APC voltage for TX output power control

static enum opstatus radioStatus;                // Current operating status

static HR_C6000 C6000(&c6000_spi, { DMR_CS });   // HR_C6000 driver
static AT1846S& at1846s = AT1846S::instance();   // AT1846S driver

void radio_init(const rtxStatus_t *rtxState)
{
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

    gpio_setMode(DMR_CLK,  OUTPUT | ALTERNATE_FUNC(2));
    gpio_setMode(DMR_MOSI, OUTPUT | ALTERNATE_FUNC(2));
    gpio_setMode(DMR_MISO, INPUT  | ALTERNATE_FUNC(2));
    spiMk22_init(&c6000_spi, 2, 3, SPI_FLAG_CPHA);

    /*
     * Enable and configure DAC for PA drive control
     */
    SIM->SCGC6 |= SIM_SCGC6_DAC0_MASK;
    DAC0->DAT[0].DATL = 0;
    DAC0->DAT[0].DATH = 0;
    DAC0->C0   |= DAC_C0_DACRFS_MASK    // Reference voltage is Vref2
               |  DAC_C0_DACEN_MASK;    // Enable DAC

    /*
     * Load calibration data
     */
    nvm_readCalibData(&calData);

    /*
     * Enable and configure both AT1846S and HR_C6000, keep AF output disabled
     * at power on.
     */
    at1846s.init();
    C6000.init();
    radio_disableAfOutput();
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
            at1846s.setBandwidth(AT1846S_BW::_12P5);
            at1846s.setTxDeviation(calData.data[currTxBand].mixGainNarrowband);
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
    // TODO: AF output management for DMR mode
    at1846s.unmuteRxOutput();
}

void radio_disableAfOutput()
{
    at1846s.muteRxOutput();
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
    C6000.writeCfgRegister(0x04, calData.data[currRxBand].mod2Offset);
    C6000.setModOffset(calData.data[currRxBand].modBias);

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
    C6000.writeCfgRegister(0x04, calData.data[currTxBand].mod2Offset);
    C6000.setModOffset(calData.data[currTxBand].modBias);

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
    gpio_clearPin(VHF_PA_EN);
    gpio_clearPin(UHF_PA_EN);

    if(radioStatus == TX)
    {
        // Set PA drive voltage to 0V
        DAC0->DAT[0].DATH = 0;
        DAC0->DAT[0].DATL = 0;
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
     * Parameters dependent on RX frequency only
     */
    const bandCalData_t *cal = &(calData.data[currRxBand]);

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
        sqlTresh = interpCalParameter(config->rxFrequency, calData.vhfCalPoints,
                                      cal->analogSqlThresh, 8);
    }
    else
    {
        sqlTresh = interpCalParameter(config->rxFrequency, calData.uhfCalPoints,
                                      cal->analogSqlThresh, 8);
    }

    at1846s.setAnalogSqlThresh(sqlTresh);

    /*
     * Parameters dependent on TX frequency only
     */
    at1846s.setPgaGain(calData.data[currTxBand].PGA_gain);
    at1846s.setMicGain(calData.data[currTxBand].analogMicGain);
    at1846s.setAgcGain(calData.data[currTxBand].rxAGCgain);
    at1846s.setPaDrive(calData.data[currTxBand].PA_drv);

    uint8_t mod1Amp  = 0;
    uint8_t txpwr_lo = 0;
    uint8_t txpwr_hi = 0;

    if(currTxBand == BND_VHF)
    {
        /* VHF band */
        txpwr_lo = interpCalParameter(config->txFrequency, calData.vhfCalPoints,
                                      calData.data[currTxBand].txLowPower, 8);

        txpwr_hi = interpCalParameter(config->txFrequency, calData.vhfCalPoints,
                                      calData.data[currTxBand].txHighPower, 8);

        mod1Amp = interpCalParameter(config->txFrequency, calData.vhfCalPoints,
                                     cal->mod1Amplitude, 8);
    }
    else
    {
        /* UHF band */
        txpwr_lo = interpCalParameter(config->txFrequency, calData.uhfPwrCalPoints,
                                      calData.data[currTxBand].txLowPower, 16);

        txpwr_hi = interpCalParameter(config->txFrequency, calData.uhfPwrCalPoints,
                                      calData.data[currTxBand].txHighPower, 16);

        mod1Amp = interpCalParameter(config->txFrequency, calData.uhfCalPoints,
                                     cal->mod1Amplitude, 8);
    }

    C6000.setModAmplitude(0, mod1Amp);

    // Calculate APC voltage, constraining output power between 1W and 5W.
    float power  = static_cast < float >(config->txPower) / 1000.0f;
          power  = std::max(std::min(power, 5.0f), 1.0f);
    float pwrHi = static_cast< float >(txpwr_hi);
    float pwrLo = static_cast< float >(txpwr_lo);
    float apc   = pwrLo + (pwrHi - pwrLo)/4.0f*(power - 1.0f);
    apcVoltage  = static_cast< uint16_t >(apc) * 16;

    // Set bandwidth, only for analog FM mode
    if(config->opMode == OPMODE_FM)
    {
        switch(config->bandwidth)
        {
            case BW_12_5:
                at1846s.setBandwidth(AT1846S_BW::_12P5);
                at1846s.setTxDeviation(calData.data[currTxBand].mixGainNarrowband);
                break;

             case BW_25:
                at1846s.setBandwidth(AT1846S_BW::_25);
                at1846s.setTxDeviation(calData.data[currTxBand].mixGainWideband);
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
    return static_cast< rssi_t > (at1846s.readRSSI());
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
