/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include "SPI2.h"
#include <pthread.h>
#include <stm32f4xx.h>

pthread_mutex_t mutex;

void spi2_init()
{
    RCC->APB1ENR |= RCC_APB1ENR_SPI2EN;
    __DSB();

    SPI2->CR1 = SPI_CR1_SSM     /* Software managment of nCS */
              | SPI_CR1_SSI     /* Force internal nCS        */
              | SPI_CR1_BR_2    /* Fclock: 42MHz/32 = 1.3MHz */
              | SPI_CR1_MSTR    /* Master mode               */
              | SPI_CR1_SPE;    /* Enable peripheral         */

    pthread_mutex_init(&mutex, NULL);
}

void spi2_terminate()
{
    RCC->APB1ENR &= ~RCC_APB1ENR_SPI2EN;
    __DSB();

    pthread_mutex_destroy(&mutex);
}

uint8_t spi2_sendRecv(const uint8_t val)
{
    SPI2->DR = val;
    while((SPI2->SR & SPI_SR_RXNE) == 0) ;
    return SPI2->DR;
}

bool spi2_lockDevice()
{
    if(pthread_mutex_trylock(&mutex) == 0)
    {
        return true;
    }

    return false;
}

void spi2_lockDeviceBlocking()
{
    pthread_mutex_lock(&mutex);
}

void spi2_releaseDevice()
{
    pthread_mutex_unlock(&mutex);
}
