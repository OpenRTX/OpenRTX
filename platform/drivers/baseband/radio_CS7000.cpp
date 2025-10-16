/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/nvmem.h"
#include "interfaces/radio.h"
#include "interfaces/delays.h"
#include "peripherals/gpio.h"
#include "peripherals/adc.h"
#include "calibration/calibInfo_CS7000.h"
#include "drivers/SPI/spi_bitbang.h"
#include "core/ctcssDetector.hpp"
#include "drivers/audio/stm32_adc.h"
#include "hwconfig.h"
#include <algorithm>
#include "core/utils.h"
#include "drivers/baseband/HR_C6000.h"
#include "drivers/baseband/SKY72310.h"
#include "drivers/baseband/AK2365A.h"

#ifdef PLATFORM_CS7000P
#define DAC     DAC1
#endif

static constexpr uint32_t CTCSS_SAMPLE_RATE = 2000;
static constexpr freq_t IF_FREQ = 49950000;    // Intermediate frequency: 49.95MHz

static const rtxStatus_t  *config;             // Pointer to data structure with radio configuration
static struct CS7000Calib calData;             // Calibration data
static uint8_t vtune_rx  = 0;                  // Tuning voltage for RX input filter
static uint8_t txpwr_lo  = 0;                  // APC voltage for TX output power control, low power
static uint8_t txpwr_hi  = 0;                  // APC voltage for TX output power control, high power
static struct rssiParams rssi;                 // RSSI curve parameters

static enum opstatus radioStatus;               // Current operating status

static int16_t __attribute__((section(".bss2"))) ctcssSamples[128];
static streamCtx ctcssCtx;
static int16_t *prevCtcssBuf;
static CtcssDetector ctcss(ctcssCoeffs2k, (CTCSS_SAMPLE_RATE / 4), 20.0f);

/*
 * Parameters for RSSI voltage (mV) to input power (dBm) conversion.
 * Measurements have been taked in the RX calibration points with input signal
 * going from -121dBm to -63dBm.
 * Thanks to Wojciech SP5WWP for the measurements!
 *
 * NOTE: there are seven calibration points over eight RX frequencies.
 */
static const struct rssiParams rssiCal[] =
{   //  slope        offset     rxFreq
    {0.0370f, -138.76814f, 400250000 },    // 400.250MHz
    {0.0371f, -135.07381f, 425050000 },    // 425.050MHz
    {0.0372f, -136.61596f, 449950000 },    // 449.950MHz
    {0.0375f, -136.87895f, 460050000 },    // 460.050MHz
    {0.0374f, -136.56000f, 470050000 },    // 470.050MHz
    {0.0374f, -136.34097f, 478985000 },    // 478.985MHz
    {0.0372f, -135.62165f, 479050000 }     // 479.050MHz
};

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

static struct rssiParams interpRssi(const uint32_t freq, const struct rssiParams cal[7])
{
    if(freq < cal[0].rxFreq)
        return cal[0];

    if(freq > cal[6].rxFreq)
        return cal[6];

    uint8_t idx;
    for(idx = 5; idx > 0; idx--)
    {
        if(freq >= cal[idx].rxFreq)
            break;
    }

    const struct rssiParams *calLo = &cal[idx];
    const struct rssiParams *calHi = &cal[idx + 1];

    float num   = ((float)(freq - calLo->rxFreq));
    float den   = ((float)(calHi->rxFreq - calLo->rxFreq));
    float offs  = calHi->offset - calLo->offset;
    float slope = calHi->slope - calLo->slope;

    struct rssiParams result;
    result.offset = calLo->offset + ((offs * num) / den);
    result.slope  = calLo->slope  + ((slope * num) / den);

    return result;
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
    gpio_setMode(AIN_CTCSS,ANALOG);

    /*
     * Configure ADC3 stream, used for CTCSS detection
     */
    ctcssCtx.buffer = ctcssSamples;
    ctcssCtx.bufSize = ARRAY_SIZE(ctcssSamples);
    ctcssCtx.bufMode = BUF_CIRC_DOUBLE;
    ctcssCtx.sampleRate = CTCSS_SAMPLE_RATE;
    ctcssCtx.running = 0;
    stm32adc_init(STM32_ADC_ADC3);

    /*
     * Configure and enable DAC
     */
#ifdef PLATFORM_CS7000P
    RCC->APB1LENR |= RCC_APB1LENR_DAC12EN;
#else
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
#endif

    DAC->DHR12R1 = 0;
    DAC->CR     |= DAC_CR_EN1;

    spiBitbang_init(&det_spi);
    spiBitbang_init(&pll_spi);

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
     * Set VCTXO bias
     */
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

    SKY73210_terminate(&pll);
    AK2365A_terminate(&detector);

    DAC->DHR12R1 = 0;
#ifdef PLATFORM_CS7000P
    RCC->APB1LENR &= ~RCC_APB1LENR_DAC12EN;
#else
    RCC->APB1ENR &= ~RCC_APB1ENR_DACEN;
#endif
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
    int16_t *data;
    size_t len;

    // CTCSS sampling stream is stopped, cannot detect the tone
    if(ctcssCtx.running == 0)
        return false;

    // Update the CTCSS detector each time there is new data from the ADC
    len = stm32_adc_audio_driver.data(&ctcssCtx, &data);
    if(data != prevCtcssBuf)
    {
        prevCtcssBuf = data;
        ctcss.update(data, len);
    }

    return ctcss.toneDetected(ctcssFreqToIndex(config->rxTone));
}

void radio_enableAfOutput()
{
    // Undocumented register, bits [1:0] seem to enable/disable FM audio RX.
    // 0xFD enable FM receive.
    C6000.writeCfgRegister(0x26, 0xFD);
}

void radio_disableAfOutput()
{
    // Undocumented register, disable FM receive
    C6000.writeCfgRegister(0x26, 0xFE);
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

    // Enable RX LNA and first IF stage
    gpioDev_set(RX_PWR_EN);

    // Configure FM detector
    AK2365A_init(&detector);
    AK2365A_setFilterBandwidth(&detector, AK2365A_BPF_6);

    // Start sampling of CTCSS signal, if enabled
    if((config->opMode == OPMODE_FM) && (config->rxToneEn == true))
        stm32_adc_audio_driver.start(STM32_ADC_ADC3, (void *) ADC_CTCSS_CH, &ctcssCtx);

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
    DAC1->DHR8R1 = static_cast< uint8_t >(apc);

    switch(config->opMode)
    {
        case OPMODE_FM:
        {
            // WARNING: HR_C6000 quirk!
            // If the CTCSS tone is disabled immediately after TX stop, the IC
            // stops outputting demodulated audio until a reset. This may be
            // something related to the "tail tone elimination" function. To
            // overcome this, the CTCSS tone is enabled/disabled before starting
            // a new transmission.
            if(config->txToneEn)
                C6000.setTxCtcss(config->txTone, 0x20);
            else if(config->toneEn)
                C6000.sendTone(1750, 0x1E);
            else
                C6000.disableTones();

            FmConfig cfg = (config->bandwidth == BW_12_5) ? FmConfig::BW_12p5kHz
                                                          : FmConfig::BW_25kHz;
            C6000.startAnalogTx(TxAudioSource::MIC, cfg | FmConfig::PREEMPH_EN);
        }
            break;

        case OPMODE_M17:
            C6000.disableTones();
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

    // Shut down CTCSS ADC sampling and reset tone detector
    if(ctcssCtx.running)
    {
        stm32_adc_audio_driver.terminate(&ctcssCtx);
        ctcss.reset();
    }

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

    // RSSI interpolation curve
    rssi = interpRssi(config->rxFrequency, rssiCal);

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
     * RSSI value is get by reading the analog RSSI output from second IF stage
     * (AK2365 IC). The corresponding power value is obtained through the linear
     * correlation existing between measured voltage in mV and power in dBm.
     */
    float rssi_mv  = ((float) adc_getVoltage(&adc1, ADC_RSSI_CH)) / 1000.0f;
    float rssi_dbm = (rssi_mv * rssi.slope) + rssi.offset;
    return static_cast< rssi_t >(rssi_dbm);
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}
