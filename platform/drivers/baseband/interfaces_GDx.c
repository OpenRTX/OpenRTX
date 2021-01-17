/***************************************************************************
 *   Copyright (C) 2020 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <hwconfig.h>
#include <interfaces/gpio.h>
#include <interfaces/delays.h>
#include <interfaces/platform.h>
#include <stdint.h>
#include <string.h>
#include <I2C0.h>
#include "interfaces.h"

/*
 * Implementation of AT1846S I2C interface.
 *
 * NOTE: on GDx devices the I2C bus is shared between the EEPROM and the AT1846S,
 * so we have to acquire exclusive ownership before exchanging data
 */

static const uint8_t devAddr = 0xE2;

void i2c_init()
{
    /* I2C already init'd by platform support package */
}

void i2c_writeReg16(uint8_t reg, uint16_t value)
{
    /*
     * Beware of endianness!
     * When writing an AT1846S register, bits 15:8 must be sent first, followed
     * by bits 7:0.
     */
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;

    i2c0_lockDeviceBlocking();
    i2c0_write(devAddr, buf, 3, true);
    i2c0_releaseDevice();
}

uint16_t i2c_readReg16(uint8_t reg)
{
    uint16_t value = 0;

    i2c0_lockDeviceBlocking();
    i2c0_write(devAddr, &reg, 1, false);
    i2c0_read(devAddr, &value, 2);
    i2c0_releaseDevice();

    /* Correction for AT1846S sending register data in big endian format */
    return __builtin_bswap16(value);
}

/*
 * Implementation of HR_C6000 "user" SPI interface.
 */

void uSpi_init()
{
    gpio_setMode(DMR_CLK,  OUTPUT);
    gpio_setMode(DMR_MOSI, OUTPUT);
    gpio_setMode(DMR_MISO, INPUT);

    gpio_setAlternateFunction(DMR_CLK,  0);
    gpio_setAlternateFunction(DMR_MOSI, 0);
    gpio_setAlternateFunction(DMR_MISO, 0);

    SIM->SCGC6 |= SIM_SCGC6_SPI0_MASK;

    SPI0->MCR &= ~SPI_MCR_MDIS_MASK;     /* Enable the SPI0 module    */
    SPI0->MCR |= SPI_MCR_MSTR_MASK       /* Master mode               */
              |  SPI_MCR_PCSIS_MASK      /* CS high when inactive     */
              |  SPI_MCR_DIS_RXF_MASK    /* Disable RX FIFO           */
              |  SPI_MCR_DIS_TXF_MASK    /* Disable TX FIFO           */
              |  SPI_MCR_HALT_MASK;      /* Stop transfers            */

    SPI0->CTAR[0] = SPI_CTAR_FMSZ(7)     /* 8bit frame size           */
                  |  SPI_CTAR_CPHA_MASK  /* CPHA = 1                  */
                  |  SPI_CTAR_PBR(2)     /* CLK prescaler divide by 5 */
                  |  SPI_CTAR_BR(3)      /* CLK scaler divide by 8    */
                  |  SPI_CTAR_PCSSCK(1)
                  |  SPI_CTAR_PASC(1)
                  |  SPI_CTAR_CSSCK(4)
                  |  SPI_CTAR_ASC(4);
}

uint8_t uSpi_sendRecv(uint8_t val)
{
    SPI0->MCR  &= ~SPI_MCR_HALT_MASK; /* Start transfer  */

    SPI0->MCR |= SPI_MCR_CLR_TXF_MASK | SPI_MCR_CLR_RXF_MASK;

    while((SPI0->SR & SPI_SR_TFFF_MASK) == 0) ;

    SPI0->PUSHR = SPI_PUSHR_EOQ_MASK | val;

    SPI0->SR |= SPI_SR_TFFF_MASK;

    while((SPI0->SR & SPI_SR_RFDF_MASK) == 0) ;
    SPI0->SR |= SPI_SR_RFDF_MASK;

    SPI0->MCR  |= SPI_MCR_HALT_MASK; /* Start transfer */

    return SPI0->POPR;
}
