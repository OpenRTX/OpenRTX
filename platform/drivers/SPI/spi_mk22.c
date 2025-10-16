/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "MK22F51212.h"
#include "spi_mk22.h"

static inline uint8_t spi_sendRecv(SPI_Type *spi, const uint8_t val)
{
    spi->MCR  &= ~SPI_MCR_HALT_MASK; // Start transfer

    spi->MCR |= SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK;

    while((spi->SR & SPI_SR_TFFF_MASK) == 0) ;

    spi->PUSHR = SPI_PUSHR_EOQ_MASK | val;

    spi->SR |= SPI_SR_TFFF_MASK;

    while((spi->SR & SPI_SR_RFDF_MASK) == 0) ;
    spi->SR |= SPI_SR_RFDF_MASK;

    spi->MCR  |= SPI_MCR_HALT_MASK; // Start transfer

    return spi->POPR;
}

int spiMk22_init(const struct spiDevice *dev, const uint8_t pbr, const uint8_t br,
                 const uint8_t flags)
{
    SPI_Type *spi = (SPI_Type *) dev->priv;
    switch((uint32_t) spi)
    {
        case SPI0_BASE:
            SIM->SCGC6 |= SIM_SCGC6_SPI0_MASK;
            __DSB();
            break;

        case SPI1_BASE:
            SIM->SCGC6 |= SIM_SCGC6_SPI1_MASK;
            __DSB();
            break;

        default:
            return -ENODEV;
            break;
    }

    spi->MCR &= ~SPI_MCR_MDIS_MASK;     // Enable the SPI module
    spi->MCR |= SPI_MCR_MSTR_MASK       // Master mode
             |  SPI_MCR_PCSIS_MASK      // CS high when inactive
             |  SPI_MCR_DIS_RXF_MASK    // Disable RX FIFO
             |  SPI_MCR_DIS_TXF_MASK    // Disable TX FIFO
             |  SPI_MCR_HALT_MASK;      // Stop transfers

    spi->CTAR[0] = SPI_CTAR_FMSZ(7)     // 8bit frame size
                 |  SPI_CTAR_PBR(pbr)   // CLK prescaler divide by 5
                 |  SPI_CTAR_BR(br)     // CLK scaler divide by 8
                 |  SPI_CTAR_PCSSCK(1)
                 |  SPI_CTAR_PASC(1)
                 |  SPI_CTAR_CSSCK(4)
                 |  SPI_CTAR_ASC(4);

    if((flags & SPI_FLAG_CPOL) != 0)
        spi->CTAR[0] |= SPI_CTAR_CPOL_MASK;

    if((flags & SPI_FLAG_CPHA) != 0)
        spi->CTAR[0] |= SPI_CTAR_CPHA_MASK;

    if((flags & SPI_LSB_FIRST) != 0)
        spi->CTAR[0] |= SPI_CTAR_LSBFE_MASK;

    if(dev->mutex != NULL)
        pthread_mutex_init((pthread_mutex_t *) dev->mutex, NULL);

    return 0;
}

void spiMk22_terminate(const struct spiDevice *dev)
{
    SPI_Type *spi = (SPI_Type *) dev->priv;
    switch((uint32_t) spi)
    {
        case SPI0_BASE:
            SIM->SCGC6 &= ~SIM_SCGC6_SPI0_MASK;
            __DSB();
            break;

        case SPI1_BASE:
            SIM->SCGC6 &= ~SIM_SCGC6_SPI1_MASK;
            __DSB();
            break;

        default:
            break;
    }

    if(dev->mutex != NULL)
        pthread_mutex_destroy((pthread_mutex_t *) dev->mutex);
}

int spiMk22_transfer(const struct spiDevice *dev, const void *txBuf,
                      void *rxBuf, const size_t size)
{
    SPI_Type *spi = (SPI_Type *) dev->priv;
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

