/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
 *                                Frederik Saraci IU2NRO,                  *
 *                                Silvano Seva IU2KWO                      *
 *                                Mathis Schmieder DB9MAT                  *
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
#include <interfaces/delays.h>
#include <interfaces/nvmem.h>
#include <interfaces/audio.h>
#include <peripherals/gpio.h>
#include <drivers/adc_stm32.h>
#include <drivers/i2c_stm32.h>
#include <calibInfo_Mod17.h>
#include <backlight.h>
#include <hwconfig.h>
#include <MCP4551.h>


ADC_STM32_DEVICE_DEFINE(adc1, ADC1, NULL, 3300000)
I2C_STM32_DEVICE_DEFINE(i2c1, I2C1, NULL)

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
     */
    gpio_setMode(I2C1_SCL, INPUT_PULL_DOWN);
    gpio_setMode(I2C1_SDA, INPUT_PULL_DOWN);

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

    /* Set analog output for baseband signal to an idle level of 1.1V */
    gpio_setMode(BASEBAND_TX, ANALOG);
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR      |= DAC_CR_EN1;
    DAC->DHR12R1  = 1365;

    nvm_init();
    audio_init();
    adcStm32_init(&adc1);
    mcp4551_init(&i2c1, SOFTPOT_RX);
    mcp4551_init(&i2c1, SOFTPOT_TX);

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
     */
    uint32_t ver = adc_getVoltage(&adc1, ADC_HWVER_CH);
    if(ver >= 3000000)
        hwInfo.hw_version = 1;

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
