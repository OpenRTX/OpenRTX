/***************************************************************************
 *   Copyright (C) 2024 by Silvano Seva IU2KWO                             *
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

#include <interfaces/nvmem.h>
#include <interfaces/radio.h>
#include <interfaces/delays.h>
#include <peripherals/gpio.h>
#include <peripherals/adc.h>
#include <calibInfo_CS7000.h>
#include <spi_bitbang.h>
#include <hwconfig.h>
#include <algorithm>
#include "HR_C6000.h"
#include "SKY72310.h"
#include "AK2365A.h"

static constexpr freq_t IF_FREQ = 49950000;    // Intermediate frequency: 49.95MHz

static const rtxStatus_t  *config;             // Pointer to data structure with radio configuration
static struct CS7000Calib calData;             // Calibration data
static uint8_t vtune_rx  = 0;                  // Tuning voltage for RX input filter
static uint8_t txpwr_lo  = 0;                  // APC voltage for TX output power control, low power
static uint8_t txpwr_hi  = 0;                  // APC voltage for TX output power control, high power

static enum opstatus radioStatus;               // Current operating status

HR_C6000 C6000((const struct spiDevice *) &c6000_spi, { C6K_CS });

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

static uint8_t interpParameter(uint32_t freq, uint32_t *calFreq, uint8_t param[8])
{
    uint8_t i;
    for(i = 6; i > 0; i--)
    {
        if(freq >= calFreq[i])
            break;
    }

    /*
     * Computations taken from original firmware V8.01.05, function at address 0x08055388.
     * Code uses a kind of Q10.2 fixed point math to handle the interpolation of calibration
     * data.
     * With respect to the original function, here the difference between current
     * frequency and the calibration point and the difference between the two calibration
     * point are divided by ten to avoid 32-bit overflow when computing the Intermediate
     * "tmp" value. Original firmware passes to the interpolation function the frequencies
     * already divided by ten.
     */
    int32_t freqLo  = calFreq[i];
    int32_t freqHi  = calFreq[i + 1];
    uint8_t paramLo = param[i];
    uint8_t paramHi = param[i + 1];

    int32_t num = ((int32_t) freq - freqLo) / 10;
    int32_t den = (freqHi - freqLo) / 10;
    int32_t tmp = ((paramHi - paramLo) * num * 4) / den;
    int32_t ret = tmp + (paramLo * 4);

    // NOTE: 1020/4 = 255
    if(ret >= 1020)
        return 0xFF;

    if(ret < 0)
        return 0;

    ret /= 4;
    if((tmp << 30) < 0)
        ret += 1;

    return ret;
}


void radio_init(const rtxStatus_t *rtxState)
{
    config      = rtxState;
    radioStatus = OFF;

    /*
     * Configure RTX GPIOs
     */
    gpioDev_set(VCOVCC_SW);         // VCOVCC high enables RX VCO, TX VCO if low
    gpioDev_clear(AF_MUTE);         // Mute FM AF output
    gpioDev_clear(CTCSS_AMP_EN);    // Power off CTCSS amplifier and filter
    gpioDev_clear(RF_APC_SW);       // Disable TX power control
    gpioDev_clear(TX_PWR_EN);       // Disable TX power stage
    gpioDev_clear(RX_PWR_EN);       // Disable RX input stage

    gpio_setMode(APC_TV,   ANALOG);
    gpio_setMode(AIN_RTX,  ANALOG);
    gpio_setMode(AIN_RSSI, ANALOG);

    /*
     * Configure and enable DAC
     */
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->DHR12R1  = 0;
    DAC->CR      |= DAC_CR_EN1;

    spiBitbang_init(&det_spi);
    spiBitbang_init(&pll_spi);
    spiBitbang_init(&c6000_spi);

    /*
     * Load calibration data
     */
    nvm_readCalibData(&calData);

    /*
     * Enable and configure PLL, wait 1ms to ensure that VCXO is stable
     */
    gpioDev_set(VCO_PWR_EN);
    SKY73210_init(&pll);

    /*
     * Configure HR_C5000
     */
    gpioDev_clear(C6K_SLEEP);        // Exit from sleep
    delayMs(10);
    C6000.init();
    C6000.setModOffset(calData.errorRate[0]);
}

void radio_terminate()
{
    gpioDev_clear(TX_PWR_EN);       // Disable TX power stage
    gpioDev_clear(RX_PWR_EN);       // Disable RX input stage
    gpioDev_clear(RF_APC_SW);       // Disable TX power control
    gpioDev_clear(CTCSS_AMP_EN);    // Power off CTCSS amplifier and filter
    gpioDev_clear(VCO_PWR_EN);      // Power off PLL and VCO
    gpioDev_clear(DET_PDN);         // Power off FM demod chip
    gpioDev_set(C6K_SLEEP);         // Power off HR_C6000

    SKY73210_terminate(&pll);
    AK2365A_terminate(&detector);
    C6000.terminate();

    DAC->DHR12R1 = 0;
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
}

void radio_setOpmode(const enum opmode mode)
{
    switch(mode)
    {
        case OPMODE_FM:
            C6000.fmMode();             // HR_C5000 in FM mode
            C6000.setInputGain(+3);     // Input gain in dB, as per TYT firmware
            break;

        case OPMODE_M17:
            C6000.fmMode();             // HR_C5000 in FM mode
            C6000.setInputGain(+9);     // Input gain in dB, found experimentally
            C6000.setModFactor(0x25);
            break;

        default:
            break;
    }
}

bool radio_checkRxDigitalSquelch()
{
    return false;
}

void radio_enableAfOutput()
{
    gpioDev_set(AF_MUTE);
}

void radio_disableAfOutput()
{
    gpioDev_clear(AF_MUTE);
}

void radio_enableRx()
{
    gpioDev_clear(TX_PWR_EN);     // Disable TX PA
    gpioDev_clear(RF_APC_SW);     // APC/TV used for RX filter tuning
    gpioDev_set(VCOVCC_SW);       // Enable RX VCO
    gpioDev_set(CTCSS_AMP_EN);    // Enable CTCSS filter/amplifier
    gpioDev_set(DET_PDN);         // Enable FM detector

    // Set PLL frequency
    uint32_t pllFreq = config->rxFrequency - IF_FREQ;
    SKY73210_setFrequency(&pll, pllFreq, 3);

    // Set input filter tune voltage
    DAC->DHR8R1  = vtune_rx;
    DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;

    // Configure FM detector
    AK2365A_init(&detector);
    AK2365A_setFilterBandwidth(&detector, AK2365A_BPF_6);

    gpioDev_set(RX_PWR_EN);   // Enable RX LNA

    radioStatus = RX;
}

void radio_enableTx()
{
    if(config->txDisable == 1)
        return;

    gpioDev_clear(RX_PWR_EN);   // Disable RX LNA
    gpioDev_set(RF_APC_SW);     // APC/TV in power control mode
    gpioDev_clear(VCOVCC_SW);   // Enable TX VCO

    // Set PLL frequency.
    SKY73210_setFrequency(&pll, config->txFrequency, 3);

    // Set TX output power, constrain between 1W and 5W.
    float power = static_cast < float >(config->txPower) / 1000.0f;
          power = std::max(std::min(power, 5.0f), 1.0f);
    float pwrHi = static_cast< float >(txpwr_hi);
    float pwrLo = static_cast< float >(txpwr_lo);
    float apc   = pwrLo + (pwrHi - pwrLo)/4.0f*(power - 1.0f);
    DAC->DHR8R1 = static_cast< uint8_t >(apc);
    DAC->SWTRIGR = DAC_SWTRIGR_SWTRIG1;

    switch(config->opMode)
    {
        case OPMODE_FM:
        {
            TxAudioSource source = TxAudioSource::MIC;
            FmConfig cfg = (config->bandwidth == BW_12_5) ? FmConfig::BW_12p5kHz
                                                          : FmConfig::BW_25kHz;
            C6000.startAnalogTx(source, cfg | FmConfig::PREEMPH_EN);
        }
            break;

        case OPMODE_M17:
            C6000.startAnalogTx(TxAudioSource::LINE_IN, FmConfig::BW_25kHz);
            break;

        default:
            break;
    }

    gpioDev_set(TX_PWR_EN);     // Enable TX PA

    radioStatus = TX;
}

void radio_disableRtx()
{
    gpioDev_clear(TX_PWR_EN);   // Disable TX PA
    gpioDev_clear(RX_PWR_EN);   // Disable RX LNA

    if(radioStatus == TX)
        C6000.stopAnalogTx();   // Stop HR_C6000 Tx

    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    // Tuning voltage for RX input filter
    vtune_rx = interpParameter(config->rxFrequency, calData.rxCalFreq, calData.rxSensitivity);

    // APC voltage for TX output power control
    txpwr_lo = interpParameter(config->txFrequency, calData.txCalFreq, calData.txMiddlePwr);
    txpwr_hi = interpParameter(config->txFrequency, calData.txCalFreq, calData.txHighPwr);

    // HR_C6000 modulation amplitude
    uint8_t qAmp = interpParameter(config->txFrequency, calData.txCalFreq, calData.txDigitalPathQ);
    uint8_t iAmp = interpParameter(config->txFrequency, calData.txCalFreq, calData.txAnalogPathI);
    C6000.writeCfgRegister(0x45, qAmp);   // Adjustment of Mod2 amplitude
    C6000.writeCfgRegister(0x46, iAmp);   // Adjustment of Mod1 amplitude

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

    float rssi_mv  = ((float) adc_getVoltage(&adc1, ADC_RSSI_CH)) / 1000.0f;
    float rssi_dbm = (rssi_mv - rssi_offset[offset_index]) / rssi_gain;
    return static_cast< rssi_t >(rssi_dbm);
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
