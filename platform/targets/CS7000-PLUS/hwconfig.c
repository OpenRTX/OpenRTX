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
#include <spi_custom.h>
#include <spi_stm32.h>
#include <adc_stm32.h>
#include <SKY72310.h>
#include <hwconfig.h>
#include <pthread.h>
#include <AK2365A.h>

static const struct spiConfig spiSrConfig =
{
    .clk  = {GPIOEXT_CLK},
    .miso = {GPIOEXT_DAT},
    .mosi = {GPIOEXT_DAT},
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags = SPI_HALF_DUPLEX
};

static const struct spiConfig spiFlashCfg =
{
    .clk       = { FLASH_CLK },
    .mosi      = { FLASH_SDI },
    .miso      = { FLASH_SDO },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = 0
};

static const struct spiConfig spiDetCfg =
{
    .clk       = { DET_CLK },
    .mosi      = { DET_DAT },
    .miso      = { DET_DAT },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_HALF_DUPLEX
};

static const struct spiConfig spiPllCfg =
{
    .clk       = { PLL_CLK },
    .mosi      = { PLL_DAT },
    .miso      = { PLL_DAT },
    .clkPeriod = SCK_PERIOD_FROM_FREQ(1000000),
    .flags     = SPI_HALF_DUPLEX
};

/**
 * SPI bitbang function for HR_C6000 command interface (U_SPI).
 *
 * Hand-tuned to be as fast as possible, gives the following clock performance
 * when compiled with -Os and run on STM32F405 at 168MHz:
 *
 * - Freq 6.46MHz
 * - Pos. width 71ns
 * - Neg. with 83ns
 */
static uint8_t spiC6000_func(const void *priv, uint8_t value)
{
    (void) priv;
    uint8_t incoming = 0;

    __disable_irq();

    for(uint8_t cnt = 0; cnt < 8; cnt++)
    {
        GPIOB->BSRR = (1 << 13);    // Set PB13 (CLK)

        if(value & (0x80 >> cnt))
            GPIOB->BSRR = 1 << 15;  // Set PB15 (MOSI)
        else
            GPIOB->BSRR = 1 << 31;  // Clear PB15 (MOSI)

        // ~70ns delay
        asm volatile("           mov   r1, #25    \n"
                     "___loop_1: cmp   r1, #0     \n"
                     "           itt   ne         \n"
                     "           subne r1, r1, #1 \n"
                     "           bne   ___loop_1  \n":::"r1");

        incoming <<= 1;
        GPIOB->BSRR = (1 << 29);                // Clear PB13 (CLK)
        incoming |= (GPIOB->IDR >> 14) & 0x01;  // Read PB14 (MISO)

        asm volatile("           mov   r1, #25    \n"
                     "___loop_2: cmp   r1, #0     \n"
                     "           itt   ne         \n"
                     "           subne r1, r1, #1 \n"
                     "           bne   ___loop_2  \n":::"r1");
    }

    __enable_irq();

    return incoming;
}


static const struct gpioPin shiftRegStrobe = { GPIOEXT_STR };
static pthread_mutex_t adc1Mutex;
static pthread_mutex_t c6000_mutex;

SPI_BITBANG_DEVICE_DEFINE(spiSr,     spiSrConfig,   NULL)
SPI_BITBANG_DEVICE_DEFINE(det_spi,   spiDetCfg,     NULL)
SPI_BITBANG_DEVICE_DEFINE(pll_spi,   spiPllCfg,     NULL)
SPI_STM32_DEVICE_DEFINE(c6000_spi,   SPI2,          &c6000_mutex)
SPI_STM32_DEVICE_DEFINE(flash_spi,   SPI4,          NULL)
GPIO_SHIFTREG_DEVICE_DEFINE(extGpio, (const struct spiDevice *) &spiSr, shiftRegStrobe, 24)
ADC_STM32_DEVICE_DEFINE(adc1, ADC1, &adc1Mutex, ADC_COUNTS_TO_UV(3300000, 16))

const struct ak2365a detector =
{
    .spi = (const struct spiDevice *) &det_spi,
    .cs  = { DET_CS  },
    .res = { DET_RST }
};

const struct sky73210 pll =
{
    .spi    = (const struct spiDevice *) &pll_spi,
    .cs     = { PLL_CS },
    .refClk = 16800000
};
