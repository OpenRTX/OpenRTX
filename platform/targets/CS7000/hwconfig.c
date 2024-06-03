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

#include <gpio_shiftReg.h>
#include <spi_bitbang.h>
#include <adc_stm32.h>
#include <hwconfig.h>
#include <pthread.h>

static const struct spiConfig spiFlashCfg =
{
    .clk       = { FLASH_CLK },
    .mosi      = { FLASH_SDI },
    .miso      = { FLASH_SDO },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = 0
};

/**
 * SPI bitbang function for SN74HC595 gpio extender.
 *
 * Hand-tuned to be as fast as possible, gives the following clock performance
 * when compiled with -Os and run on STM32F405 at 168MHz:
 *
 * - Freq 8.46MHz
 * - Pos. width 36ns
 * - Neg. with 82ns
 */
static uint8_t spiSr_func(const void *priv, uint8_t value)
{
    (void) priv;

    for(uint8_t cnt = 0; cnt < 8; cnt++)
    {
        GPIOE->BSRR = (1 << 23);    // Clear PE7 (CLK)

        if(value & (0x80 >> cnt))
            GPIOE->BSRR = 1 << 9;   // Set PE9 (MOSI)
        else
            GPIOE->BSRR = 1 << 25;  // Clear PE9 (MOSI)

        // ~70ns delay
        asm volatile("           mov   r1, #1     \n"
                     "___loop_2: cmp   r1, #0     \n"
                     "           itt   ne         \n"
                     "           subne r1, r1, #1 \n"
                     "           bne   ___loop_2  \n":::"r1");

        GPIOE->BSRR = (1 << 7);                 // Set PE7 (CLK)
    }

    return 0;
}

static const struct gpioPin shiftRegStrobe = { GPIOEXT_STR };
static pthread_mutex_t adc1Mutex;

SPI_CUSTOM_DEVICE_DEFINE(spiSr, spiSr_func, NULL, NULL)
SPI_BITBANG_DEVICE_DEFINE(flash_spi, spiFlashCfg, NULL)
GPIO_SHIFTREG_DEVICE_DEFINE(extGpio, (const struct spiDevice *) &spiSr, shiftRegStrobe, 24)
ADC_STM32_DEVICE_DEFINE(adc1, ADC1, &adc1Mutex, 3300000)
