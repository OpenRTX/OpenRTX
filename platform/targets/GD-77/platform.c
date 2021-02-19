/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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
#include <interfaces/nvmem.h>
#include <interfaces/gpio.h>
#include <calibInfo_GDx.h>
#include <ADC0_GDx.h>
#include <string.h>
#include <I2C0.h>
#include <os.h>
#include "hwconfig.h"

/* Mutex for concurrent access to ADC0 */
OS_MUTEX adc_mutex;
OS_ERR e;

gdxCalibration_t calibration;
hwInfo_t hwInfo;

void platform_init()
{
    /* Configure GPIOs */
    gpio_setMode(GREEN_LED, OUTPUT);
    gpio_setMode(RED_LED,   OUTPUT);

    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_clearPin(LCD_BKLIGHT);

    gpio_setMode(PTT_SW, INPUT);

    gpio_setMode(PWR_SW, OUTPUT);

    /*
     * Configure backlight PWM: 58.5kHz, 8 bit resolution
     */
    SIM->SCGC6 |= SIM_SCGC6_FTM0(1); /* Enable clock */

    FTM0->CONTROLS[3].CnSC = FTM_CnSC_MSB(1)
                           | FTM_CnSC_ELSB(1); /* Edge-aligned PWM, clear on match */
    FTM0->CONTROLS[3].CnV  = 0;

    FTM0->MOD  = 0xFF;                         /* Reload value          */
    FTM0->SC   = FTM_SC_PS(3)                  /* Prescaler divide by 8 */
               | FTM_SC_CLKS(1);               /* Enable timer          */

    gpio_setMode(LCD_BKLIGHT, OUTPUT);
    gpio_setAlternateFunction(LCD_BKLIGHT, 2);

    /*
     * Initialise ADC
     */
    adc0_init();
    OSMutexCreate(&adc_mutex, "", &e);

    /*
     * Initialise I2C driver, once for all the modules
     */
    gpio_setMode(I2C_SDA, OPEN_DRAIN);
    gpio_setMode(I2C_SCL, OPEN_DRAIN);
    gpio_setAlternateFunction(I2C_SDA, 3);
    gpio_setAlternateFunction(I2C_SCL, 3);
    i2c0_init();

    /*
     * Initialise non volatile memory manager and load calibration data.
     */
    nvm_init();
    nvm_readCalibData(&calibration);

    /* Initialise hardware information structure */
    hwInfo.uhf_maxFreq = FREQ_LIMIT_UHF_HI/1000000;
    hwInfo.uhf_minFreq = FREQ_LIMIT_UHF_LO/1000000;
    hwInfo.vhf_maxFreq = FREQ_LIMIT_VHF_HI/1000000;
    hwInfo.vhf_minFreq = FREQ_LIMIT_VHF_LO/1000000;
    hwInfo.uhf_band    = 1;
    hwInfo.vhf_band    = 1;
    hwInfo.lcd_type    = 0;
    memcpy(hwInfo.name, "GD-77", 5);
    hwInfo.name[5] = '\0';
}

void platform_terminate()
{
    gpio_clearPin(LCD_BKLIGHT);
    gpio_clearPin(RED_LED);
    gpio_clearPin(GREEN_LED);

    adc0_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);
}

float platform_getVbat()
{
    float value = 0.0f;
    OSMutexPend(&adc_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &e);
    value = adc0_getMeasurement(1);
    OSMutexPost(&adc_mutex, OS_OPT_POST_NONE, &e);

    return (value * 3.0f)/1000.0f;
}

float platform_getMicLevel()
{
    float value = 0.0f;
    OSMutexPend(&adc_mutex, 0u, OS_OPT_PEND_BLOCKING, 0u, &e);
    value = adc0_getMeasurement(3);
    OSMutexPost(&adc_mutex, OS_OPT_POST_NONE, &e);

    return value;
}

float platform_getVolumeLevel()
{
    /* TODO */
    return 0.0f;
}

uint8_t platform_getChSelector()
{
    /* GD77 does not have a channel selector */
    return 0;
}

bool platform_getPttStatus()
{
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
}

void platform_ledOn(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_setPin(GREEN_LED);
            break;

        case RED:
            gpio_setPin(RED_LED);
            break;

        default:
            break;
    }
}

void platform_ledOff(led_t led)
{
    switch(led)
    {
        case GREEN:
            gpio_clearPin(GREEN_LED);
            break;

        case RED:
            gpio_clearPin(RED_LED);
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

void platform_setBacklightLevel(uint8_t level)
{
    FTM0->CONTROLS[3].CnV = level;
}

const void *platform_getCalibrationData()
{
    return ((const void *) &calibration);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
