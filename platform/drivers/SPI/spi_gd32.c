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

static inline uint8_t spi_sendRecv(uint32_t spi_periph, const uint8_t val)
{

    //Reusing code from spiFlash_A36plus, which should be working, later try if it's possible with less branching
    while (spi_i2s_flag_get(spi_periph, SPI_FLAG_TBE) == RESET)
        ;
    spi_i2s_data_transmit(spi_periph, val);
    while (spi_i2s_flag_get(spi_periph, SPI_FLAG_TRANS) != RESET)
        ;
    while (spi_i2s_flag_get(spi_periph, SPI_FLAG_RBNE) == RESET)
        ;
    return spi_i2s_data_receive(spi_periph);

}

int spiGd32_init(const struct spiDevice *dev, const uint32_t speed, const uint8_t flags)
{
    uint32_t spi_periph = (uint32_t)dev->priv;
    spi_parameter_struct spi_init_struct;
    uint8_t busId;

    switch(spi_periph)
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

        // F303 seems to have SPI2, F330 does not
        // case SPI2:
        //     busId = CK_APB1; //check for f303!!
        //     rcu_periph_clock_enable(RCU_SPI2);
        //     __DSB();
        //     break;

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

    // Initialize SPI parameter structure with default values
    spi_struct_para_init(&spi_init_struct);

    /* configure SPI1 parameters based on the flags */
    spi_init_struct.device_mode = SPI_MASTER;                                                  /*!< SPI master or slave */
    spi_init_struct.trans_mode = SPI_TRANSMODE_FULLDUPLEX;                                     /*!< SPI transfer type */
    spi_init_struct.frame_size = SPI_FRAMESIZE_8BIT;                                           /*!< SPI frame size */
    spi_init_struct.nss = SPI_NSS_SOFT;                                                        /*!< SPI NSS control by handware or software */
    spi_init_struct.endian = (flags & SPI_LSB_FIRST) ? SPI_ENDIAN_LSB : SPI_ENDIAN_MSB;        /*!< SPI big endian or little endian */
    spi_init_struct.clock_polarity_phase =                                                     /*!< SPI clock phase and polarity */
        ((flags & SPI_FLAG_CPOL) ? SPI_CTL0_CKPL : 0) |
        ((flags & SPI_FLAG_CPHA) ? SPI_CTL0_CKPH : 0);
    spi_init_struct.prescale = CTL0_PSC(spiDiv);                                               /*!< Prescale Divider */

    GD32_spi_init(spi_periph, &spi_init_struct);
    spi_enable(spi_periph);

    if(dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) dev->mutex, NULL);

    return 0;
}

void spiStm32_terminate(const struct spiDevice *dev)
{
    uint32_t spi_periph = (uint32_t)dev->priv;
    spi_i2s_deinit(spi_periph);

    if(dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) dev->mutex);
}

int spiGd32_transfer(const struct spiDevice *dev, const void *txBuf,
                      void *rxBuf, const size_t size)
{
    //SPI_TypeDef *spi = (SPI_TypeDef *) dev->priv;
    uint32_t spi_periph = (uint32_t)dev->priv;

    uint8_t *rxData = (uint8_t *) rxBuf;
    const uint8_t *txData = (const uint8_t *) txBuf;

    // Send only
    if(rxBuf == NULL)
    {
        for(size_t i = 0; i < size; i++)
            spi_sendRecv(spi_periph, txData[i]);

        return 0;
    }

    // Receive only
    if(txBuf == NULL)
    {
        for(size_t i = 0; i < size; i++)
            rxData[i] = spi_sendRecv(spi_periph, 0x00);

        return 0;
    }

    // Transmit and receive
    for(size_t i = 0; i < size; i++)
        rxData[i] = spi_sendRecv(spi_periph, txData[i]);

    return 0;
}

