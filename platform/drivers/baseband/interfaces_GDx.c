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
