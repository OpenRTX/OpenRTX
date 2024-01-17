/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
#include <calibInfo_Mod17.h>
#include <ADC1_Mod17.h>
#include <backlight.h>
#include <hwconfig.h>
#include <MCP4551.h>

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
    .name        = "Module17",
    .other       = NULL,
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

    /* Set analog output for baseband signal to an idle level of 1.1V */
    gpio_setMode(BASEBAND_TX, INPUT_ANALOG);
    RCC->APB1ENR |= RCC_APB1ENR_DACEN;
    DAC->CR      |= DAC_CR_EN1;
    DAC->DHR12R1  = 1365;

    nvm_init();
    adc1_init();
    i2c_init();
    mcp4551_init(SOFTPOT_RX);
    mcp4551_init(SOFTPOT_TX);
    audio_init();

    /* Set defaults for calibration */
    mod17CalData.tx_wiper  = 0x080;
    mod17CalData.rx_wiper  = 0x080;
    mod17CalData.tx_invert = 0;
    mod17CalData.rx_invert = 0;
    mod17CalData.mic_gain  = 0;

    /*
     * Hardware version is set using a voltage divider on PA3.
     * - 0V:   rev. 0.1d or lower
     * - 3.3V: rev 0.1e
     */
    uint16_t ver = adc1_getMeasurement(ADC_HWVER_CH);
    if(ver >= 3000)
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

    adc1_terminate();
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
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
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
