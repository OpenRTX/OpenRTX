/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/nvmem.h"
#include "interfaces/audio.h"
#include "peripherals/gpio.h"
#include "drivers/i2c_stm32.h"
#include "calibration/calibInfo_Mod17.h"
#include "drivers/ADC/adc_stm32.h"
#include "drivers/backlight/backlight.h"
#include "hwconfig.h"
#include "drivers/baseband/MCP4551.h"
#include <errno.h>


ADC_STM32_DEVICE_DEFINE(adc1, ADC1, NULL, ADC_COUNTS_TO_UV(3300000, 12))
I2C_STM32_DEVICE_DEFINE(i2c1, I2C1, NULL)
I2C_STM32_DEVICE_DEFINE(i2c2, I2C2, NULL)

extern mod17Calib_t mod17CalData;

static hwInfo_t hwInfo =
{
    .vhf_maxFreq = 0,
    .vhf_minFreq = 0,
    .vhf_band    = 0,
    .uhf_maxFreq = 0,
    .uhf_minFreq = 0,
    .uhf_band    = 0,
    .hw_version  = 0,
    .flags       = 0,
    .name        = "Module17"
};

void platform_init()
{
    gpio_setMode(POWER_SW, OUTPUT);
    gpio_setPin(POWER_SW);

    /* Configure GPIOs */
    gpio_setMode(PTT_LED,  OUTPUT);
    gpio_setMode(SYNC_LED, OUTPUT);
    gpio_setMode(ERR_LED,  OUTPUT);

    gpio_setMode(PTT_SW,  INPUT);
    gpio_setMode(PTT_OUT, OUTPUT);
    gpio_clearPin(PTT_OUT);

    gpio_setMode(AIN_HWVER, ANALOG);

    /*
     * Check if external I2C1 pull-ups are present. If they are not,
     * enable internal pull-ups and slow-down I2C1.
     * The sequence of operation have to be respected otherwise the
     * I2C peripheral might report as continuously busy.
     */
    gpio_setMode(I2C1_SCL, INPUT_PULL_DOWN);
    gpio_setMode(I2C1_SDA, INPUT_PULL_DOWN);
    delayUs(100);

    uint8_t i2cSpeed   = I2C_SPEED_100kHz;
    bool    i2cPullups = gpio_readPin(I2C1_SCL)
                       & gpio_readPin(I2C1_SDA);

    /* Set gpios to alternate function, connected to I2C peripheral  */
    if(i2cPullups == false)
    {
        gpio_setMode(I2C1_SCL, ALTERNATE_OD_PU | ALTERNATE_FUNC(4));
        gpio_setMode(I2C1_SDA, ALTERNATE_OD_PU | ALTERNATE_FUNC(4));
        i2cSpeed = I2C_SPEED_LOW;
    }
    else
    {
        gpio_setMode(I2C1_SCL, ALTERNATE_OD | ALTERNATE_FUNC(4));
        gpio_setMode(I2C1_SDA, ALTERNATE_OD | ALTERNATE_FUNC(4));
    }

    i2c_init(&i2c1, i2cSpeed);

    nvm_init();
    audio_init();
    adcStm32_init(&adc1);

    /* Baseband tuning soft potentiometers */
    int ret = mcp4551_init(&i2c1, SOFTPOT_RX);
    if(ret == 0)
    {
        hwInfo.flags |= MOD17_FLAGS_SOFTPOT;
        mcp4551_init(&i2c1, SOFTPOT_TX);
    }

    /* Set defaults for calibration */
    mod17CalData.tx_wiper     = 0x080;
    mod17CalData.rx_wiper     = 0x080;
    mod17CalData.bb_tx_invert = 0;
    mod17CalData.bb_rx_invert = 0;
    mod17CalData.mic_gain     = 0;

    /*
     * Hardware version is set using a voltage divider on PA3.
     * - 0V:   rev. 0.1d or lower
     * - 3.3V: rev 0.1e
     * - 1.65V: rev 1.0
     */
    uint32_t ver = adc_getVoltage(&adc1, ADC_HWVER_CH);
    if(ver <= (MOD17_HW01D_VOLTAGE + MOD17_HWDET_THRESH))
    {
        hwInfo.hw_version = MOD17_HW_V01_D;
    }
    else if(ver >= (MOD17_HW01E_VOLTAGE - MOD17_HWDET_THRESH))
    {
        hwInfo.hw_version = MOD17_HW_V01_E;
    }
    else if((ver >= (MOD17_HW10_VOLTAGE - MOD17_HWDET_THRESH)) &&
            (ver <= (MOD17_HW10_VOLTAGE + MOD17_HWDET_THRESH)))
    {
        hwInfo.hw_version = MOD17_HW_V10;

        /*
         * Determine if HMI is connected by checking if the I2C pull-up
         * resistors are present
         */
        i2cPullups = gpio_readPin(HMI_SMCLK)
                   & gpio_readPin(HMI_SMDATA);

        if(i2cPullups)
        {
            hwInfo.flags |= MOD17_FLAGS_HMI_PRESENT;

            /* Determine HMI hardware version */
            gpio_setMode(HMI_AIN_HWVER, ANALOG);
            ver = adc_getVoltage(&adc1, ADC_HMI_HWVER_CH);

            if((ver >= (MOD17_HMI10_VOLTAGE - MOD17_HWDET_THRESH)) &&
               (ver <= (MOD17_HMI10_VOLTAGE + MOD17_HWDET_THRESH)))
            {
                hwInfo.hw_version |= (MOD17_HMI_V10 << 8);
            }
        }
    }

    /* 100ms blink of sync led to signal device startup */
    gpio_setPin(SYNC_LED);
    sleepFor(0, 100);
    gpio_clearPin(SYNC_LED);
}

void platform_terminate()
{
    /* Shut down LEDs */
    gpio_clearPin(PTT_LED);
    gpio_clearPin(SYNC_LED);
    gpio_clearPin(ERR_LED);

    adcStm32_terminate(&adc1);
    nvm_terminate();
    audio_terminate();

    /*
     * Cut off the power switch then wait 100ms to allow the 3.3V rail to
     * effectively go down to 0V. Without this delay, the board fails to power
     * off because the main() function returns, triggering an OS reboot.
     */
    gpio_clearPin(POWER_SW);
    sleepFor(0, 100);
}

uint16_t platform_getVbat()
{
   return 0;
}

uint8_t platform_getMicLevel()
{
    return 0;
}

uint8_t platform_getVolumeLevel()
{
    return 0;
}

int8_t platform_getChSelector()
{
    return 0;
}

bool platform_getPttStatus()
{
    // Return true if gpio status matches the PTT in active level
    uint8_t ptt_status = gpio_readPin(PTT_SW);
    if(ptt_status == mod17CalData.ptt_in_level)
        return true;

    return false;
}

bool platform_pwrButtonStatus()
{
    return true;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case RED:
            gpio_setPin(PTT_LED);
            break;

        case GREEN:
            gpio_setPin(SYNC_LED);
            break;

        case YELLOW:
            gpio_setPin(ERR_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch(led)
    {
        case RED:
            gpio_clearPin(PTT_LED);
            break;

        case GREEN:
            gpio_clearPin(SYNC_LED);
            break;

        case YELLOW:
            gpio_clearPin(ERR_LED);
            break;

        default:
            break;
    }
}

void platform_beepStart(uint16_t freq)
{
    /* TODO */
    (void) freq;
}

void platform_beepStop()
{
    /* TODO */
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
