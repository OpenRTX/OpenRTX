/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "peripherals/gpio.h"
#include "interfaces/nvmem.h"
#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "drivers/backlight/backlight.h"
#include "hwconfig.h"
#include <string.h>
#include "drivers/ADC/adc_stm32.h"
#include "calibration/calibInfo_MDx.h"
#include "drivers/tones/toneGenerator_MDx.h"
#include "peripherals/rtc.h"
#include "interfaces/audio.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/GPS/gps_stm32.h"
#include "drivers/chSelector/chSelector.h"
#include "core/gps.h"

/* TODO: Hardcoded hwInfo until we implement reading from flash */
static const hwInfo_t hwInfo =
{
    .vhf_maxFreq = 174,
    .vhf_minFreq = 136,
    .vhf_band    = 1,
    .uhf_maxFreq = 480,
    .uhf_minFreq = 400,
    .uhf_band    = 1,
    .hw_version  = 0,
    .name        = "MD-9600"
};

void platform_init()
{
    /* Enable 8V power supply rail */
    gpio_setMode(PWR_SW, OUTPUT);
    gpio_setPin(PWR_SW);

    gpio_setMode(PTT_SW, INPUT);

    gpio_setMode(AIN_VBAT,  ANALOG);
    gpio_setMode(AIN_MIC,   ANALOG);
    gpio_setMode(AIN_RSSI,  ANALOG);
    gpio_setMode(AIN_SW2,   ANALOG);
    gpio_setMode(AIN_SW1,   ANALOG);
    gpio_setMode(AIN_RSSI2, ANALOG);
    gpio_setMode(AIN_HTEMP, ANALOG);

    /* Initialise ADC1, for vbat, RSSI, ... */
    adcStm32_init(&adc1);

    /*
     * Initialise SPI2 for external flash and LCD
     */
    gpio_setMode(SPI2_CLK, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(SPI2_SDO, ALTERNATE | ALTERNATE_FUNC(5));
    gpio_setMode(SPI2_SDI, ALTERNATE | ALTERNATE_FUNC(5));
    spiStm32_init(&spi2, 1300000, 0);

    nvm_init();                      /* Initialise non volatile memory manager */
    toneGen_init();                  /* Initialise tone generator              */
    rtc_init();                      /* Initialise RTC                         */
    backlight_init();                /* Initialize Backlight                   */
    chSelector_init();               /* Initialise channel selector handler    */
    audio_init();                    /* Initialise audio management module     */
}

void platform_terminate()
{
    /* Shut down all the modules */
    adcStm32_terminate(&adc1);
    gpsStm32_terminate();
    toneGen_terminate();
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
     * adc1_getMeasurement returns a value in uV.
     */
    uint32_t vbat = adc_getVoltage(&adc1, ADC_VBAT_CH) * 57;
    return vbat / 10000;
}

uint8_t platform_getMicLevel()
{
    /* Value from ADC is 12 bit wide: shift right by four to get 0 - 255 */
    return adc_getRawSample(&adc1, ADC_VOX_CH) >> 4;
}

uint8_t platform_getVolumeLevel()
{
    /*
     * Knob position corresponds to an analog signal in the range 0 - 1600mV,
     * converted to a value in range 0 - 255 using fixed point math: divide by
     * 1600 and then multiply by 256.
     */
    uint16_t value = adc_getVoltage(&adc1, ADC_VOL_CH) / 1000;
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
    GPIOD->BSRR = 1 << (4 + 16);                    /* PD4 low      */
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

datetime_t platform_getCurrentTime()
{
    return rtc_getTime();
}

void platform_setTime(datetime_t t)
{
    rtc_setTime(t);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}

const struct gpsDevice *platform_initGps()
{
    const struct gpsDevice *dev = NULL;

    // Turn on the GPS and check if there is voltage on the RXD pin
    gpio_setMode(GPS_DATA, INPUT_PULL_DOWN);
    gpio_setMode(GPS_EN, OUTPUT);
    gpio_setPin(GPS_EN);

    for(size_t i = 0; i < 50; i++) {
        if(gpio_readPin(GPS_DATA) != 0) {
            dev = &gps;
            gpsStm32_init(9600);
            break;
        }

        sleepFor(0, 1);
    }

    gpio_clearPin(GPS_EN);
    gpio_setMode(GPS_DATA, ALTERNATE | ALTERNATE_FUNC(7));

    return dev;
}

/*
 * NOTE: implementation of this API function is provided in
 * platform/drivers/chSelector/chSelector_MD9600.c
 */
// int8_t platform_getChSelector()
