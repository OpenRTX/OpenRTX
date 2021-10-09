/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
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
 *   As a special exception, if other files instantiate templates or use   *
 *   macros or inline functions from this file, or you compile this file   *
 *   and link it with other works to produce a work based on this file,    *
 *   this file does not by itself cause the resulting work to be covered   *
 *   by the GNU General Public License. However the source code for this   *
 *   file must still be made available in accordance with the GNU General  *
 *   Public License. This exception does not invalidate any other reasons  *
 *   why a work based on this file might be covered by the GNU General     *
 *   Public License.                                                       *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <interfaces/gpio.h>
#include <interfaces/nvmem.h>
#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <hwconfig.h>
#include <string.h>
#include <backlight.h>
#include <ADC1_MDx.h>
#include <calibInfo_MDx.h>
#include <toneGenerator_MDx.h>
#include <interfaces/rtc.h>
#include <interfaces/audio.h>
#include <SPI2.h>
#include <chSelector.h>

hwInfo_t hwInfo;

void platform_init()
{
    /* Enable 8V power supply rail */
    gpio_setMode(PWR_SW, OUTPUT);
    gpio_setPin(PWR_SW);

    gpio_setMode(PTT_SW, INPUT);

    /*
     * Initialise ADC1, for vbat, RSSI, ...
     * Configuration of corresponding GPIOs in analog input mode is done inside
     * the driver.
     */
    adc1_init();

    /*
     * Initialise SPI2 for external flash and LCD
     */
    gpio_setMode(SPI2_CLK, ALTERNATE);
    gpio_setMode(SPI2_SDO, ALTERNATE);
    gpio_setMode(SPI2_SDI, ALTERNATE);
    gpio_setAlternateFunction(SPI2_CLK, 5); /* SPI2 is on AF5 */
    gpio_setAlternateFunction(SPI2_SDO, 5);
    gpio_setAlternateFunction(SPI2_SDI, 5);

    spi2_init();

    /* TODO: Hardcode hwInfo until we implement reading from flash */
    memset(&hwInfo, 0x00, sizeof(hwInfo));
    hwInfo.vhf_maxFreq = 174;
    hwInfo.vhf_minFreq = 136;
    hwInfo.vhf_band    = 1;
    hwInfo.uhf_maxFreq = 480;
    hwInfo.uhf_minFreq = 400;
    hwInfo.uhf_band    = 1;
    hwInfo.lcd_type    = 0;
    memcpy(hwInfo.name, "MD-9600", 7);
    hwInfo.name[8] = '\0';

    nvm_init();                      /* Initialise non volatile memory manager */
    toneGen_init();                  /* Initialise tone generator              */
    rtc_init();                      /* Initialise RTC                         */
    backlight_init();                /* Initialise backlight driver            */
    chSelector_init();               /* Initialise channel selector handler    */
    audio_init();                    /* Initialise audio management module     */
}

void platform_terminate()
{
    /* Shut down backlight */
    backlight_terminate();

    /* Shut down all the modules */
    adc1_terminate();
    toneGen_terminate();
    rtc_terminate();
    chSelector_terminate();
    audio_terminate();

    /* Finally, remove power supply */
    gpio_clearPin(PWR_SW);

    /*
     * MD-9600 does not have a proper power on/off mechanism and the MCU is
     * always powered. Thus, for turn off, perform a system reset.
     */
    NVIC_SystemReset();
    while(1) ;
}

uint16_t platform_getVbat()
{
    /*
     * Battery voltage is measured through an 1:5.7 voltage divider and
     * adc1_getMeasurement returns a value in mV. To have effective battery
     * voltage we have to multiply by the ratio: with a simple trick we can do
     * it also without using floats and with a maximum error of -1mV.
     */

    uint16_t vbat = adc1_getMeasurement(ADC_VBAT_CH);
    return (vbat * 6) - ((vbat * 3) / 10);
}

uint8_t platform_getMicLevel()
{
    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return (adc1_getRawSample(ADC_VOX_CH) >> 4);
}

uint8_t platform_getVolumeLevel()
{
    /*
     * Knob position corresponds to an analog signal in the range 0 - 1600mV,
     * converted to a value in range 0 - 255 using fixed point math: divide by
     * 1600 and then multiply by 256.
     */
    uint16_t value = adc1_getMeasurement(ADC_VOL_CH);
    if(value > 1599) value = 1599;
    uint32_t level = value << 16;
    level /= 1600;
    return ((uint8_t) (level >> 8));
}

bool platform_getPttStatus()
{
    /* PTT line has a pullup resistor with PTT switch closing to ground */
    return (gpio_readPin(PTT_SW) == 0) ? true : false;
}

bool platform_pwrButtonStatus()
{
    /*
     * The power on/off button, when pressed, connects keyboard coloumn 3 to
     * row 3. Here we set coloumn to input with pull up mode and row to output
     * mode, consistently with keyboard driver.
     */

    gpio_setMode(KB_COL3, INPUT_PULL_UP);
    gpio_setMode(KB_ROW3, OUTPUT);

    /*
     * Critical section to avoid stomping the keyboard driver.
     * Also, working at register level to keep it as short as possible
     */
    __disable_irq();
    uint32_t prevRowState = GPIOD->ODR & (1 << 4);  /* Row 3 is PD4 */
    GPIOD->BSRRH = 1 << 4;                          /* PD4 low      */
    delayUs(10);
    uint32_t btnStatus = GPIOE->IDR & 0x01;         /* Col 3 is PE0 */
    GPIOD->ODR |= prevRowState;                     /* Restore PD4  */
    __enable_irq();

    /*
     * Power button API requires this function to return true if power is
     * enabled. To comply to the requirement this function behaves in this way:
     * - if button is not pressed, return current status of PWR_SW.
     * - if button is pressed and PWR_SW is low, return true as user wants to
     *   turn the radio on.
     * - if button is pressed and PWR_SW is high, return false as user wants to
     *   turn the radio off.
     */

    /*
     * Power button follows an active-low logic: btnStatus is low when button
     * is pressed
     */
    if(btnStatus == 0)
    {
        if(gpio_readPin(PWR_SW))
        {
            /* Power switch high and button pressed: request to turn off */
            return false;
        }
        else
        {
            /* Power switch low and button pressed: request to turn on */
            return true;
        }
    }

    /* We get here if power button is not pressed, just return PWR_SW status */
    return gpio_readPin(PWR_SW) ? true : false;
}

void platform_ledOn(led_t led)
{
    /* No LEDs on this platform */
    (void) led;
}

void platform_ledOff(led_t led)
{
    /* No LEDs on this platform */
    (void) led;
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

const void *platform_getCalibrationData()
{
    return NULL;
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

/*
 * NOTE: implementation of this API function is provided in
 * platform/drivers/chSelector/chSelector_MD9600.c
 */
// int8_t platform_getChSelector()

/*
 * NOTE: implementation of this API function is provided in
 * platform/drivers/backlight/backlight_MDx.c
 */
// void platform_setBacklightLevel(uint8_t level)

