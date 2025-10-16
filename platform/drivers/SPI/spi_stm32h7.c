/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rcc.h"
#include <errno.h>
#include "stm32h7xx.h"
#include "spi_stm32.h"


static inline uint8_t spi_sendRecv(SPI_TypeDef *spi, const uint8_t val)
{
    // Of course, setting the SPI frame size is not enough: to actually send
    // 8 bits instead of 32, we have to access the TXDR register with an 8 bit
    // write operation. Damn ST...
    *((volatile uint8_t *) &spi->TXDR) = val;
    while((spi->SR & SPI_SR_TXC) == 0) ;
    return spi->RXDR;
}

int spiStm32_init(const struct spiDevice *dev, const uint32_t speed, const uint8_t flags)
{
    SPI_TypeDef *spi = (SPI_TypeDef *) dev->priv;
    uint32_t busClk;

    // On STM32H7 the clock tree is a bit more complicated than on STM32F4:
    // - SPI1/2/3 use PLL1_Q clock for SCK generation and APB clock for reg access
    // - SPI4/5/6 use APB clock both for SCK generation and reg access
    // From PLL config PLL1_Q output is set at 100MHz
    switch((uint32_t) spi)
    {
        case SPI1_BASE:
            busClk = 100000000;
            RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
            __DSB();
            break;

        case SPI2_BASE:
            busClk = 100000000;
            RCC->APB1LENR |= RCC_APB1LENR_SPI2EN;
            __DSB();
            break;

        case SPI3_BASE:
            busClk = 100000000;
            RCC->APB1LENR |= RCC_APB1LENR_SPI3EN;
            __DSB();
            break;

        case SPI4_BASE:
            busClk = getBusClock(PERIPH_BUS_APB2);
            RCC->APB2ENR |= RCC_APB2ENR_SPI4EN;
            __DSB();
            break;

        case SPI5_BASE:
            busClk = getBusClock(PERIPH_BUS_APB2);
            RCC->APB2ENR |= RCC_APB2ENR_SPI5EN;
            __DSB();
            break;

         case SPI6_BASE:
            busClk = getBusClock(PERIPH_BUS_APB4);
            RCC->APB4ENR |= RCC_APB4ENR_SPI6EN;
            __DSB();
            break;

        default:
            return -ENODEV;
            break;
    }

    uint8_t spiDiv;
    uint32_t spiClk;

    // Find nearest clock frequency, round down
    for(spiDiv = 0; spiDiv < 7; spiDiv += 1)
    {
        spiClk = busClk / (1 << (spiDiv + 1));
        if(spiClk <= speed)
            break;
    }

    if(spiClk > speed)
        return -EINVAL;

    spi->I2SCFGR = 0;
    spi->CR2  = 0;
    spi->CFG1 = (spiDiv << SPI_CFG1_MBR_Pos)    // Baud rate
              | SPI_CFG1_DSIZE_2                // SPI frame size 8-bit
              | SPI_CFG1_DSIZE_1
              | SPI_CFG1_DSIZE_0;

    spi->CR1  = SPI_CR1_SSI;                    // Force nCS state to active
    spi->CFG2 = SPI_CFG2_AFCNTR                 // Peripheral keeps control of gpio AF mode
              | SPI_CFG2_SSM                    // Software management of nCS
              | SPI_CFG2_MASTER;

    if((flags & SPI_FLAG_CPOL) != 0)
        spi->CFG2 |= SPI_CFG2_CPOL;

    if((flags & SPI_FLAG_CPHA) != 0)
        spi->CFG2 |= SPI_CFG2_CPHA;

    if((flags & SPI_LSB_FIRST) != 0)
        spi->CFG2 |= SPI_CFG2_LSBFRST;

    if(dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) dev->mutex, NULL);

    return 0;
}

void spiStm32_terminate(const struct spiDevice *dev)
{
    SPI_TypeDef *spi = (SPI_TypeDef *) dev->priv;

    switch((uint32_t) spi)
    {
        case SPI1_BASE:
            RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
            __DSB();
            break;

        case SPI2_BASE:
            RCC->APB1LENR &= ~RCC_APB1LENR_SPI2EN;
            __DSB();
            break;

        case SPI3_BASE:
            RCC->APB1LENR &= ~RCC_APB1LENR_SPI3EN;
            __DSB();
            break;

        case SPI4_BASE:
            RCC->APB2ENR &= ~RCC_APB2ENR_SPI4EN;
            __DSB();
            break;

        case SPI5_BASE:
            RCC->APB2ENR &= ~RCC_APB2ENR_SPI5EN;
            __DSB();
            break;

         case SPI6_BASE:
            RCC->APB4ENR &= ~RCC_APB4ENR_SPI6EN;
            __DSB();
            break;

        default:
            break;
    }

    if(dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) dev->mutex);
}

int spiStm32_transfer(const struct spiDevice *dev, const void *txBuf,
                      void *rxBuf, const size_t size)
{
    SPI_TypeDef *spi = (SPI_TypeDef *) dev->priv;
    uint8_t *rxData = (uint8_t *) rxBuf;
    const uint8_t *txData = (const uint8_t *) txBuf;

    spi->CR1 |= SPI_CR1_SPE;
    spi->CR1 |= SPI_CR1_CSTART;

    // Send only
    if(rxBuf == NULL)
    {
        for(size_t i = 0; i < size; i++)
            spi_sendRecv(spi, txData[i]);

        return 0;
    }

    // Receive only
    if(txBuf == NULL)
    {
        for(size_t i = 0; i < size; i++)
            rxData[i] = spi_sendRecv(spi, 0x00);

        return 0;
    }

    // Transmit and receive
    for(size_t i = 0; i < size; i++)
        rxData[i] = spi_sendRecv(spi, txData[i]);

    spi->CR1 &= ~SPI_CR1_SPE;

    return 0;
}
