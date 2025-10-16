/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/tones/toneGenerator_MDx.h"
#include "interfaces/nvmem.h"
#include "interfaces/radio.h"
#include "peripherals/gpio.h"
#include "calibration/calibInfo_MDx.h"
#include "drivers/SPI/spi_bitbang.h"
#include "drivers/ADC/adc_stm32.h"
#include "hwconfig.h"
#include <algorithm>
#include "core/utils.h"
#include "drivers/baseband/HR_C5000.h"
#include "drivers/baseband/SKY72310.h"

static const freq_t IF_FREQ = 49950000;         // Intermediate frequency: 49.95MHz

static const rtxStatus_t  *config;              // Pointer to data structure with radio configuration

static md3x0Calib_t calData;                    // Calibration data
static bool    isVhfBand = false;               // True if rtx stage is for VHF band
static uint8_t vtune_rx  = 0;                   // Tuning voltage for RX input filter
static uint8_t txpwr_lo  = 0;                   // APC voltage for TX output power control, low power
static uint8_t txpwr_hi  = 0;                   // APC voltage for TX output power control, high power

static enum opstatus radioStatus;               // Current operating status

static HR_C5000 C5000((const struct spiDevice *) &c5000_spi, { DMR_CS });

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
    gpio_setMode(FM_MUTE,   OUTPUT);

    gpio_clearPin(PLL_PWR);    // PLL off
    gpio_setPin(VCOVCC_SW);    // VCOVCC high enables RX VCO, TX VCO if low
    #ifndef MDx_ENABLE_SWD
    gpio_setPin(WN_SW);        // 25kHz bandwidth
    #endif
    gpio_clearPin(FM_MUTE);    // Mute FM AF output
    gpio_clearPin(DMR_SW);     // Disconnect HR_C5000 input IF signal and audio out
    gpio_clearPin(FM_SW);      // Disconnect analog FM audio path
    gpio_clearPin(RF_APC_SW);  // Disable TX power control
    gpio_clearPin(TX_STG_EN);  // Disable TX power stage
    gpio_clearPin(RX_STG_EN);  // Disable RX input stage

    /*
     * Configure and enable DAC
     */
    gpio_setMode(APC_TV,    ANALOG);
    gpio_setMode(MOD2_BIAS, ANALOG);
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR = DAC_CR_EN2 | DAC_CR_EN1;
    DAC->DHR12R2 = 0;
    DAC->DHR12R1 = 0;

    /*
     * Load calibration data
     */
    nvm_readCalibData(&calData);

    /*
     * Enable and configure PLL and HR_C5000
     */
    spiBitbang_init(&pll_spi);
    spiBitbang_init(&c5000_spi);

    gpio_setPin(PLL_PWR);
    SKY73210_init(&pll);
    C5000.init();

    /*
     * Modulation bias settings, as per TYT firmware.
     */
    DAC->DHR12R2 = (calData.freqAdjustMid)*4 + 0x600;
    C5000.setModOffset(calData.freqAdjustMid);
}

void radio_terminate()
{
    SKY73210_terminate(&pll);
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
            _setBandwidth(BW_12_5);     // Set bandwidth to 12.5kHz
            //C5000_dmrMode();
            break;

        case OPMODE_M17:
            gpio_clearPin(DMR_SW);      // Disconnect analog paths for DMR
            gpio_setPin(FM_SW);         // Enable analog RX stage after superhet
            C5000.fmMode();             // HR_C5000 in FM mode
            C5000.setInputGain(-3);     // Input gain in dB, found experimentally
            _setBandwidth(BW_25);       // Set bandwidth to 25kHz for proper deviation
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
    // TODO: AF output management for DMR mode
    gpio_setPin(FM_MUTE);
}

void radio_disableAfOutput()
{
    gpio_clearPin(FM_MUTE);
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

    SKY73210_setFrequency(&pll, pllFreq, 5);
    DAC->DHR12L1 = vtune_rx * 0xFF;

    gpio_setPin(RX_STG_EN);            // Enable RX LNA
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
    SKY73210_setFrequency(&pll, pllFreq, 5);

    // Set TX output power, constrain between 1W and 5W.
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
            TxAudioSource source = TxAudioSource::MIC;
            FmConfig cfg = (config->bandwidth == BW_12_5) ? FmConfig::BW_12p5kHz
                                                          : FmConfig::BW_25kHz;

            // Setup 1750Hz tone, if enabled
            if(config->toneEn)
            {
                source = TxAudioSource::LINE_IN;    // HR_C5000 audio input is tone generator
                C5000.setModFactor(0x1E);           // Set correct modulation factor
                gpio_setMode(BEEP_OUT, ALTERNATE | ALTERNATE_FUNC(2));
                toneGen_beepOn(1750.0f, 255, 0);    // Enable 1750Hz tone, always on
            }

            C5000.startAnalogTx(source, cfg | FmConfig::PREEMPH_EN);
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
    if(radioStatus == TX)
    {
        C5000.stopAnalogTx();   // Stop HR_C5000 tx
        toneGen_toneOff();      // Stop CTCSS tone

        // Stop 1750Hz tone
        if(config->toneEn)
        {
            toneGen_beepOff();
            gpio_setMode(BEEP_OUT, INPUT);
        }
    }

    gpio_clearPin(TX_STG_EN);   // Disable TX PA
    gpio_clearPin(RX_STG_EN);   // Disable RX LNA
    gpio_clearPin(FM_MUTE);     // Mute analog path towards the audio amplifier

    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    // Tuning voltage for RX input filter
    vtune_rx = interpCalParameter(config->rxFrequency, calData.rxFreq,
                                  calData.rxSensitivity, 9);

    // APC voltage for TX output power control
    txpwr_lo = interpCalParameter(config->txFrequency, calData.txFreq,
                                  calData.txLowPower, 9);

    txpwr_hi = interpCalParameter(config->txFrequency, calData.txFreq,
                                  calData.txHighPower, 9);

    // HR_C5000 modulation amplitude
    const uint8_t *Ical = calData.sendIrange;
    const uint8_t *Qcal = calData.sendQrange;

    if(config->opMode == OPMODE_FM)
    {
        Ical = calData.analogSendIrange;
        Qcal = calData.analogSendQrange;
    }

    uint8_t I = interpCalParameter(config->txFrequency, calData.txFreq, Ical, 9);
    uint8_t Q = interpCalParameter(config->txFrequency, calData.txFreq, Qcal, 9);

    C5000.setModAmplitude(I, Q);

    // Set bandwidth, only for analog FM mode
    if(config->opMode == OPMODE_FM)
    {
        enum bandwidth bw = static_cast< enum bandwidth >(config->bandwidth);
        _setBandwidth(bw);
    }

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
