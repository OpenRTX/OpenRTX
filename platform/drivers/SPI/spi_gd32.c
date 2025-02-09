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

//#include <rcc.h>
#include <errno.h>
//#include <stm32f4xx.h>
#include "gd32f3x0_rcu.h"
#include "gd32f3x0_spi.h"
#include "spi_gd32.h"

//static inline uint8_t spi_sendRecv(SPI_TypeDef *spi, const uint8_t val)
/*!
    \brief      SPI receive/transmit data
    \param[in]  spi_periph: SPIx(x=0,1)
    \param[in]  val: 8-bit data
    \param[out] none
    \retval     8-bit data
*/
static inline uint8_t spi_sendRecv(uint32_t spi_periph, const uint8_t val)
{
    SPI_DATA(spi_periph) = (uint32_t)val;   // Write to SPI
    while((SPI_STAT(spi_periph) & SPI_STAT_RBNE) == 0);  // Wait for receive buffer not empty
    return ((uint8_t)SPI_DATA(spi_periph));
}

int spiGd32_init(const struct spiDevice *dev, const uint32_t speed, const uint8_t flags)
{
    spi_parameter_struct *spi = (spi_parameter_struct *) dev->priv;
    rcu_clock_freq_enum busId;

    switch((uint32_t) spi)
    {
        case SPI0:
            busId = CK_APB2;
            rcu_periph_clock_enable(RCU_SPI0);
            __DSB();
            break;

        case SPI1:
            busId = CK_APB1;
            rcu_periph_clock_enable(RCU_SPI1);
            __DSB();
            break;

        default:
            return -ENODEV;
            break;
    }

    uint8_t spiDiv;
    uint32_t spiClk;
    uint32_t busClk = rcu_clock_freq_get(busId);

    // Find nearest clock frequency, round down
    for(spiDiv = 0; spiDiv < 7; spiDiv += 1)
    {
        spiClk = busClk / (1 << (spiDiv + 1));
        if(spiClk <= speed)
            break;
    }

    if(spiClk > speed)
        return -EINVAL;

    spi->prescale = spiDiv;             // Prescale Divider
    spi->device_mode = SPI_MASTER;      // Master mode and Software NSS (SPI_master is defined as SPI_CTL0_MSTMOD | SPI_CTL0_SWNSS)

    if((flags & SPI_FLAG_CPOL) != 0)
        spi->clock_polarity_phase |= SPI_CTL0_CKPL;

    if((flags & SPI_FLAG_CPHA) != 0)
        spi->clock_polarity_phase |= SPI_CTL0_CKPH;

    if((flags & SPI_LSB_FIRST) != 0)
        spi->endian |= SPI_ENDIAN_LSB;

    spi_enable((uint32_t) spi);     // Enable peripheral

    if(dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) dev->mutex, NULL);

    return 0;
}

void spiGd32_terminate(const struct spiDevice *dev)
{
    spi_parameter_struct *spi = (spi_parameter_struct *) dev->priv;

    switch((uint32_t) spi)
    {
        case SPI0:
            rcu_periph_clock_disable(RCU_SPI0);
            __DSB();
            break;

        case SPI1:
            rcu_periph_clock_disable(RCU_SPI1);
            __DSB();
            break;
    }

    if(dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) dev->mutex);
}

int spiGd32_transfer(const struct spiDevice *dev, const void *txBuf,
                      void *rxBuf, const size_t size)
{
    spi_parameter_struct *spi = (spi_parameter_struct *) dev->priv;
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

