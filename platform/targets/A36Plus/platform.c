/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN,                            *
 *                         Frederik Saraci IU2NRO,                         *
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

#include <peripherals/gpio.h>
#include <interfaces/platform.h>
#include <platform/drivers/baseband/bk4819.h>
#include <hwconfig.h>
#include <platform/drivers/display/ST7735S_a36plus.h>
#include <platform/drivers/ADC/ADC0_A36plus.h>
#include "gd32f3x0_rtc.h"
#include "gd32f3x0_pmu.h"

static const hwInfo_t hwInfo =
{
    .vhf_maxFreq = 200,
    .vhf_minFreq = 136,
    .vhf_band    = 1,
    .uhf_maxFreq = 650,
    .uhf_minFreq = 200,
    .uhf_band    = 1,
    .hw_version  = 0,
    .name        = "A36Plus"
};

static void lcd_spi_config(void)
{
    rcu_periph_clock_enable(LCD_GPIO_RCU);
    gpio_af_set(LCD_GPIO_PORT, GPIO_AF_0, LCD_GPIO_SCK_PIN | LCD_GPIO_SDA_PIN);
    gpio_mode_set(LCD_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, LCD_GPIO_SCK_PIN | LCD_GPIO_SDA_PIN);
    gpio_output_options_set(LCD_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_GPIO_SCK_PIN | LCD_GPIO_SDA_PIN);

    gpio_mode_set(LCD_GPIO_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_PULLUP, LCD_GPIO_RST_PIN | LCD_GPIO_CS_PIN | LCD_GPIO_WR_PIN | LCD_GPIO_LIGHT_PIN);
    gpio_output_options_set(LCD_GPIO_PORT, GPIO_OTYPE_PP, GPIO_OSPEED_50MHZ, LCD_GPIO_RST_PIN | LCD_GPIO_CS_PIN | LCD_GPIO_WR_PIN | LCD_GPIO_LIGHT_PIN);
    spi_parameter_struct spi_init_struct;
    /* deinitialize SPI and the parameters */
    spi_i2s_deinit(SPI1);

    spi_struct_para_init(&spi_init_struct);

    /* configure SPI1 parameter */
    spi_init_struct.nss = SPI_NSS_SOFT;
    spi_init_struct.prescale = SPI_PSC_2;
    spi_init_struct.endian = SPI_ENDIAN_MSB;
    spi_init_struct.device_mode = SPI_MASTER;
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;
    spi_init_struct.trans_mode = SPI_TRANSMODE_BDTRANSMIT;
    spi_init_struct.clock_polarity_phase = SPI_CK_PL_HIGH_PH_2EDGE;
    spi_init(SPI1, &spi_init_struct);
    spi_enable(SPI1);
}

void spi_config(void)
{
    rcu_periph_clock_enable(RCU_SPI0);
    rcu_periph_clock_enable(RCU_SPI1);
    lcd_spi_config();
}

void platform_init()
{
    // Configure GPIOs
    // gpio_setMode(GREEN_LED, OUTPUT);
    // gpio_setMode(RED_LED,   OUTPUT);
    gpio_setMode(PTT_SW,    INPUT_PULL_UP);
    spi_config();
    backlight_init();
    nvm_init();         // Initialize nonvolatile memory
    //rtc_initialize();   // Initialize the RTC peripheral
    gpio_setMode(AIN_VBAT, ANALOG);
    //adc0_init();
}



void platform_terminate()
{
    // Shut down LED
    gpio_clearPin(GREEN_LED);
    gpio_clearPin(RED_LED);
    backlight_terminate();
    
}

uint16_t platform_getVbat()
{
    // Return the ADC reading from AIN_VBAT
    return adc0_getMeasurement(0);
}

uint8_t platform_getMicLevel()
{
    return ReadRegister(0x64) / 255;
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
    // PTT is active low
    return (gpio_readPin(PTT_SW) ? false : true);
}

bool platform_pwrButtonStatus()
{
    return !gpio_readPin(PWR_SW);
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
    BK4819_PlayTone(freq, true);
}

void platform_beepStop()
{
    return;
}

// Helper function to convert BCD to normal numbers
static uint8_t bcd2dec(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

datetime_t platform_getCurrentTime()
{
    // Initialize the RTC peripheral
    rtc_parameter_struct rtc_init_struct;
    rtc_current_time_get(&rtc_init_struct);
    datetime_t t;
    // rtc_parameter_struct stores stuff in BCD
    // so, convert
    t.year = bcd2dec(rtc_init_struct.rtc_year);
    t.month = bcd2dec(rtc_init_struct.rtc_month);
    t.date = bcd2dec(rtc_init_struct.rtc_date);
    t.day = bcd2dec(rtc_init_struct.rtc_day_of_week);
    t.hour = bcd2dec(rtc_init_struct.rtc_hour);
    t.minute = bcd2dec(rtc_init_struct.rtc_minute);
    t.second = bcd2dec(rtc_init_struct.rtc_second);
    return t;
}

void platform_setTime(datetime_t t)
{
    rtc_parameter_struct rtc_init_struct;
    rtc_current_time_get(&rtc_init_struct);
    rtc_init_struct.rtc_year = (t.year / 10) << 4 | (t.year % 10);
    rtc_init_struct.rtc_month = (t.month / 10) << 4 | (t.month % 10);
    rtc_init_struct.rtc_date = (t.day / 10) << 4 | (t.day % 10);
    rtc_init_struct.rtc_hour = (t.hour / 10) << 4 | (t.hour % 10);
    rtc_init_struct.rtc_minute = (t.minute / 10) << 4 | (t.minute % 10);
    rtc_init_struct.rtc_second = (t.second / 10) << 4 | (t.second % 10);
    rtc_init(&rtc_init_struct);
}

const hwInfo_t *platform_getHwInfo()
{
    return &hwInfo;
}
