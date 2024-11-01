/***************************************************************************
 *   Copyright (C) 2024 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <spi_bitbang.h>
#include <spi_custom.h>
#include <hwconfig.h>
#include <pthread.h>
#include <pinmap.h>
#include <spi_stm32.h>
#include <adc_stm32.h>


static pthread_mutex_t c6000_mutex;
static pthread_mutex_t adcMutex;

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
        GPIOE->BSRR = (1 << 3);     // Set PE3 (CLK)

        if(value & (0x80 >> cnt))
            GPIOE->BSRR = 1 << 4;   // Set PE4 (MOSI)
        else
            GPIOE->BSRR = 1 << 20;  // Clear PE4 (MOSI)

        // ~70ns delay
        asm volatile("           mov   r1, #1     \n"
                     "___loop_1: cmp   r1, #0     \n"
                     "           itt   ne         \n"
                     "           subne r1, r1, #1 \n"
                     "           bne   ___loop_1  \n":::"r1");

        incoming <<= 1;
        GPIOE->BSRR = (1 << 19);                // Clear PE3 (CLK)
        incoming |= (GPIOE->IDR >> 5) & 0x01;   // Read PE5 (MISO)
    }

    __enable_irq();

    return incoming;
}

SPI_CUSTOM_DEVICE_DEFINE(c6000_spi, spiC6000_func, NULL, &c6000_mutex)
SPI_STM32_DEVICE_DEFINE(nvm_spi, SPI1, NULL)
ADC_STM32_DEVICE_DEFINE(adc1, ADC1, &adcMutex, ADC_COUNTS_TO_UV(3300000, 12))
