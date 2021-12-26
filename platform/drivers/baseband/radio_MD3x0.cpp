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
#include <toneGenerator_MDx.h>
#include <interfaces/radio.h>
#include <interfaces/gpio.h>
#include <calibInfo_MDx.h>
#include <calibUtils.h>
#include <hwconfig.h>
#include <ADC1_MDx.h>
#include <algorithm>
#include "HR_C5000.h"
#include "SKY72310.h"

static const freq_t IF_FREQ = 49950000;  // Intermediate frequency: 49.95MHz

const md3x0Calib_t *calData;             // Pointer to calibration data
const rtxStatus_t  *config;              // Pointer to data structure with radio configuration

bool    isVhfBand = false;               // True if rtx stage is for VHF band
uint8_t vtune_rx  = 0;                   // Tuning voltage for RX input filter
uint8_t txpwr_lo  = 0;                   // APC voltage for TX output power control, low power
uint8_t txpwr_hi  = 0;                   // APC voltage for TX output power control, high power

enum opstatus radioStatus;               // Current operating status

HR_C5000& C5000 = HR_C5000::instance();  // HR_C5000 driver

/*
 * Parameters for RSSI voltage (mV) to input power (dBm) conversion.
 * Gain is constant, while offset values are aligned to calibration frequency
 * test points.
 * Thanks to Wojciech SP5WWP for the measurements!
 */
const float rssi_gain = 22.0f;
const float rssi_offset[] = {3277.618f, 3654.755f, 3808.191f,
                             3811.318f, 3804.936f, 3806.591f,
                             3723.882f, 3621.373f, 3559.782f};


void _setBandwidth(const enum bandwidth bw)
{
    switch(bw)
    {
        case BW_12_5:
            #ifndef MDx_ENABLE_SWD
            gpio_clearPin(WN_SW);
            #endif
            C5000.setModFactor(0x1E);
            break;

        case BW_20:
            #ifndef MDx_ENABLE_SWD
            gpio_setPin(WN_SW);
            #endif
            C5000.setModFactor(0x30);
            break;

        case BW_25:
            #ifndef MDx_ENABLE_SWD
            gpio_setPin(WN_SW);
            #endif
            C5000.setModFactor(0x3C);
            break;

        default:
            break;
    }
}

void radio_init(const rtxStatus_t *rtxState)
{
    /*
     * Load calibration data
     */
    calData = reinterpret_cast< const md3x0Calib_t * >(platform_getCalibrationData());

    config      = rtxState;
    radioStatus = OFF;
    isVhfBand   = (platform_getHwInfo()->vhf_band == 1) ? true : false;

    /*
     * Configure RTX GPIOs
     */
    gpio_setMode(PLL_PWR,   OUTPUT);
    gpio_setMode(VCOVCC_SW, OUTPUT);
    gpio_setMode(DMR_SW,    OUTPUT);
    #ifndef MDx_ENABLE_SWD
    gpio_setMode(WN_SW,     OUTPUT);
    #endif
    gpio_setMode(FM_SW,     OUTPUT);
    gpio_setMode(RF_APC_SW, OUTPUT);
    gpio_setMode(TX_STG_EN, OUTPUT);
    gpio_setMode(RX_STG_EN, OUTPUT);

    gpio_setMode(FM_MUTE,  OUTPUT);
    gpio_clearPin(FM_MUTE);

    gpio_clearPin(PLL_PWR);    // PLL off
    gpio_setPin(VCOVCC_SW);    // VCOVCC high enables RX VCO, TX VCO if low
    #ifndef MDx_ENABLE_SWD
    gpio_setPin(WN_SW);        // 25kHz bandwidth
    #endif
    gpio_clearPin(DMR_SW);     // Disconnect HR_C5000 input IF signal and audio out
    gpio_clearPin(FM_SW);      // Disconnect analog FM audio path
    gpio_clearPin(RF_APC_SW);  // Disable TX power control
    gpio_clearPin(TX_STG_EN);  // Disable TX power stage
    gpio_clearPin(RX_STG_EN);  // Disable RX input stage

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
    C5000.init();

    /*
     * Modulation bias settings, as per TYT firmware.
     */
    DAC->DHR12R2 = (calData->freqAdjustMid)*4 + 0x600;
    C5000.setModOffset(calData->freqAdjustMid);
}

void radio_terminate()
{
    SKY73210_terminate();
    C5000.terminate();

    gpio_clearPin(PLL_PWR);    // PLL off
    gpio_clearPin(DMR_SW);     // Disconnect HR_C5000 input IF signal and audio out
    gpio_clearPin(FM_SW);      // Disconnect analog FM audio path
    gpio_clearPin(RF_APC_SW);  // Disable RF power control
    gpio_clearPin(TX_STG_EN);  // Disable TX power stage
    gpio_clearPin(RX_STG_EN);  // Disable RX input stage

    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void radio_tuneVcxo(const int16_t vhfOffset, const int16_t uhfOffset)
{
    (void) vhfOffset;

    /*
     * Adjust VCXO bias voltage acting on the value stored in MCU's DAC.
     * Data from calibration is first converted to int16_t, then the value for
     * the DAC register is computed according to which is done inside TYT's
     * firmware.
     * The signed offset is then added to this value, the result is constrained
     * in the range [0 4095], converted to uint16_t and written into the DAC
     * register.
     *
     * NOTE: we deliberately chose not to update the HR_C5000 modulation offset
     * register, as we still have to deeply understand how TYT computes
     * the values written there.
     */
    int16_t calValue  = static_cast< int16_t >(calData->freqAdjustMid);
    int16_t oscTune   = (calValue*4 + 0x600) + uhfOffset;
    oscTune           = std::max(std::min(oscTune, int16_t(4095)), int16_t(0));
    DAC->DHR12R2      = static_cast< uint16_t >(oscTune);
}

void radio_setOpmode(const enum opmode mode)
{
    switch(mode)
    {
        case OPMODE_FM:
            gpio_clearPin(DMR_SW);      // Disconnect analog paths for DMR
            gpio_setPin(FM_SW);         // Enable analog RX stage after superhet
            C5000.fmMode();             // HR_C5000 in FM mode
            C5000.setInputGain(+3);     // Input gain in dB, as per TYT firmware
            break;

        case OPMODE_DMR:
            gpio_clearPin(FM_SW);       // Disable analog RX stage after superhet
            gpio_setPin(DMR_SW);        // Enable analog paths for DMR
            //C5000_dmrMode();
            break;

        case OPMODE_M17:
            gpio_clearPin(DMR_SW);      // Disconnect analog paths for DMR
            gpio_setPin(FM_SW);         // Enable analog RX stage after superhet
            C5000.fmMode();             // HR_C5000 in FM mode
            C5000.setInputGain(-5);     // Input gain in dB, found experimentally
            break;

        default:
            break;
    }
}

bool radio_checkRxDigitalSquelch()
{
    return false;
}

void radio_enableRx()
{
    gpio_clearPin(TX_STG_EN);          // Disable TX PA

    gpio_clearPin(RF_APC_SW);          // APC/TV used for RX filter tuning
    gpio_setPin(VCOVCC_SW);            // Enable RX VCO

    // Set PLL frequency and filter tuning voltage
    float pllFreq = static_cast< float >(config->rxFrequency);
    if(isVhfBand)
    {
        pllFreq += static_cast< float >(IF_FREQ);
        pllFreq *= 2.0f;
    }
    else
    {
        pllFreq -= static_cast< float >(IF_FREQ);
    }

    SKY73210_setFrequency(pllFreq, 5);
    DAC->DHR12L1 = vtune_rx * 0xFF;

    gpio_setPin(RX_STG_EN);            // Enable RX LNA

    if(config->opMode == OPMODE_FM)
    {
        gpio_setPin(FM_MUTE);          // In FM mode, unmute audio path towards speaker
    }

    radioStatus = RX;
}

void radio_enableTx()
{
    if(config->txDisable == 1) return;

    gpio_clearPin(RX_STG_EN);   // Disable RX LNA

    gpio_setPin(RF_APC_SW);     // APC/TV in power control mode
    gpio_clearPin(VCOVCC_SW);   // Enable TX VCO

    // Set PLL frequency.
    float pllFreq = static_cast< float >(config->txFrequency);
    if(isVhfBand) pllFreq *= 2.0f;
    SKY73210_setFrequency(pllFreq, 5);

    // Set TX output power, constrain between 1W and 5W.
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
            C5000.startAnalogTx(TxAudioSource::MIC, cfg | FmConfig::PREEMPH_EN);
        }
            break;

        case OPMODE_M17:
            C5000.startAnalogTx(TxAudioSource::LINE_IN, FmConfig::BW_25kHz);
            break;

        default:
            break;
    }

    gpio_setPin(TX_STG_EN);     // Enable TX PA

    if(config->txToneEn == 1)
    {
        toneGen_toneOn();       // Enable CTSS
    }

    radioStatus = TX;
}

void radio_disableRtx()
{
    // If we are currently transmitting, stop tone and C5000 TX
    if(radioStatus == TX)
    {
        toneGen_toneOff();
        C5000.stopAnalogTx();
    }

    gpio_clearPin(TX_STG_EN);   // Disable TX PA
    gpio_clearPin(RX_STG_EN);   // Disable RX LNA
    gpio_clearPin(FM_MUTE);     // Mute analog path towards the audio amplifier

    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    // Tuning voltage for RX input filter
    vtune_rx = interpCalParameter(config->rxFrequency, calData->rxFreq,
                                  calData->rxSensitivity, 9);

    // APC voltage for TX output power control
    txpwr_lo = interpCalParameter(config->txFrequency, calData->txFreq,
                                  calData->txLowPower, 9);

    txpwr_hi = interpCalParameter(config->txFrequency, calData->txFreq,
                                  calData->txHighPower, 9);

    // HR_C5000 modulation amplitude
    const uint8_t *Ical = calData->sendIrange;
    const uint8_t *Qcal = calData->sendQrange;

    if(config->opMode == OPMODE_FM)
    {
        Ical = calData->analogSendIrange;
        Qcal = calData->analogSendQrange;
    }

    uint8_t I = interpCalParameter(config->txFrequency, calData->txFreq, Ical, 9);
    uint8_t Q = interpCalParameter(config->txFrequency, calData->txFreq, Qcal, 9);

    C5000.setModAmplitude(I, Q);

    // Set bandwidth, force 12.5kHz for DMR mode
    enum bandwidth bandwidth = static_cast< enum bandwidth >(config->bandwidth);
    if(config->opMode == OPMODE_DMR) bandwidth = BW_12_5;
    _setBandwidth(bandwidth);

    // Set CTCSS tone
    float tone = static_cast< float >(config->txTone) / 10.0f;
    toneGen_setToneFreq(tone);

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
    /*
     * On MD3x0 devices, RSSI value is get by reading the analog RSSI output
     * from second IF stage (GT3136 IC).
     * The corresponding power value is obtained through the linear correlation
     * existing between measured voltage in mV and power in dBm. While gain is
     * constant, offset depends from the rx frequency.
     */

    freq_t rxFreq = config->rxFrequency;
    uint32_t offset_index = (rxFreq - 400035000)/10000000;

    if(rxFreq < 401035000) offset_index = 0;
    if(rxFreq > 479995000) offset_index = 8;

    float rssi_mv  = ((float) adc1_getMeasurement(ADC_RSSI_CH));
    float rssi_dbm = (rssi_mv - rssi_offset[offset_index]) / rssi_gain;
    return rssi_dbm;
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
