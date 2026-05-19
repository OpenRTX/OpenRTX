/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/SPI/spi_bitbang.h"
#include "drivers/SPI/spi_custom.h"
#include "hwconfig.h"
#include <pthread.h>
#include "pinmap.h"
#include "drivers/SPI/spi_stm32.h"
#include "drivers/ADC/adc_stm32.h"

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
