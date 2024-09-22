/***************************************************************************
 *   Copyright (C) 2021 - 2024 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <rcc.h>
#include <errno.h>
#include <stm32f4xx.h>
#include "spi_stm32.h"


static inline uint8_t spi_sendRecv(SPI_TypeDef *spi, const uint8_t val)
{
    spi->DR = val;
    while((spi->SR & SPI_SR_RXNE) == 0) ;
    return spi->DR;
}

int spiStm32_init(const struct spiDevice *dev, const uint32_t speed, const uint8_t flags)
{
    SPI_TypeDef *spi = (SPI_TypeDef *) dev->priv;
    uint8_t busId;

    switch((uint32_t) spi)
    {
        case SPI1_BASE:
            busId = PERIPH_BUS_APB2;
            RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
            __DSB();
            break;

        case SPI2_BASE:
            busId = PERIPH_BUS_APB1;
            RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
            __DSB();
            break;

        case SPI3_BASE:
            busId = PERIPH_BUS_APB1;
            RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
            __DSB();
            break;

        default:
            return -ENODEV;
            break;
    }

    uint8_t spiDiv;
    uint32_t spiClk;
    uint32_t busClk = rcc_getBusClock(busId);

    // Find nearest clock frequency, round down
    for(spiDiv = 0; spiDiv < 7; spiDiv += 1)
    {
        spiClk = busClk / (1 << (spiDiv + 1));
        if(spiClk <= speed)
            break;
    }

    if(spiClk > speed)
        return -EINVAL;

    spi->CR1 = SPI_CR1_SSM     // Software managment of nCS
             | SPI_CR1_SSI     // Force internal nCS
             | (spiDiv << 3)   // Baud rate
             | SPI_CR1_MSTR;   // Master mode

    if((flags & SPI_FLAG_CPOL) != 0)
        spi->CR1 |= SPI_CR1_CPOL;

    if((flags & SPI_FLAG_CPHA) != 0)
        spi->CR1 |= SPI_CR1_CPHA;

    if((flags & SPI_LSB_FIRST) != 0)
        spi->CR1 |= SPI_CR1_LSBFIRST;

    spi->CR1 |= SPI_CR1_SPE;    // Enable peripheral

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
            RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
            __DSB();
            break;

        case SPI3_BASE:
            RCC->APB1ENR &= ~RCC_APB1ENR_SPI3EN;
            __DSB();
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

    return 0;
}

