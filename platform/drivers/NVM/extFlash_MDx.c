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

#include "extFlash_MDx.h"
#include <gpio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <hwconfig.h>

#define CMD_READ 0x03   /* Read data              */
#define CMD_RSEC 0x48   /* Read security register */
#define CMD_WKUP 0xAB   /* Release power down     */
#define CMD_PDWN 0xB9   /* Power down             */


uint8_t _spi1SendRecv(uint8_t val)
{
    SPI1->DR = val;
    while((SPI1->SR & SPI_SR_RXNE) == 0) ;
    return SPI1->DR;
}

void extFlash_init()
{
    gpio_setMode(FLASH_CLK, ALTERNATE);
    gpio_setMode(FLASH_SDO, ALTERNATE);
    gpio_setMode(FLASH_SDI, ALTERNATE);
    gpio_setAlternateFunction(FLASH_CLK, 5); /* SPI1 is on AF5 */
    gpio_setAlternateFunction(FLASH_SDO, 5);
    gpio_setAlternateFunction(FLASH_SDI, 5);
    gpio_setMode(FLASH_CS, OUTPUT);
    gpio_setPin(FLASH_CS);

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    __DSB();

    SPI1->CR1 = SPI_CR1_SSM     /* Software managment of nCS */
              | SPI_CR1_SSI     /* Force internal nCS        */
              | SPI_CR1_BR_2    /* Fclock: 84MHz/64 = 1.3MHz */
              | SPI_CR1_BR_0
              | SPI_CR1_MSTR    /* Master mode               */
              | SPI_CR1_SPE;    /* Enable peripheral         */
}

void extFlash_terminate()
{
    extFlash_sleep();

    RCC->APB2ENR &= ~RCC_APB2ENR_SPI1EN;
    __DSB();

    gpio_setMode(FLASH_CLK, INPUT);
    gpio_setMode(FLASH_SDO, INPUT);
    gpio_setMode(FLASH_SDI, INPUT);
    gpio_setMode(FLASH_CS,  INPUT);
}

void extFlash_wakeup()
{
    gpio_clearPin(FLASH_CS);
    (void) _spi1SendRecv(CMD_WKUP);
    gpio_setPin(FLASH_CS);
}

void extFlash_sleep()
{
    gpio_clearPin(FLASH_CS);
    (void) _spi1SendRecv(CMD_PDWN);
    gpio_setPin(FLASH_CS);
}

ssize_t extFlash_readSecurityRegister(uint32_t addr, uint8_t* buf, size_t len)
{
    uint32_t addrBase  = addr & 0x3000;
    uint32_t addrRange = addr & 0xCFFF;
    if((addrBase < 0x1000) || (addrBase > 0x3000)) return -1; /* Out of base  */
    if(addrRange > 0xFF) return -1;                           /* Out of range */

    /* Keep 256-byte boundary to avoid wrap-around when reading */
    size_t readLen = len;
    if((addrRange + len) > 0xFF)
    {
        readLen = 0xFF - (addrRange & 0xFF);
    }

    gpio_clearPin(FLASH_CS);
    (void) _spi1SendRecv(CMD_RSEC);             /* Command        */
    (void) _spi1SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) _spi1SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) _spi1SendRecv(addr & 0xFF);          /* Address low    */
    (void) _spi1SendRecv(0x00);                 /* Dummy byte     */

    for(size_t i = 0; i < readLen; i++)
    {
        buf[i] = _spi1SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);

    return ((ssize_t) readLen);
}

void extFlash_readData(uint32_t addr, uint8_t* buf, size_t len)
{
    gpio_clearPin(FLASH_CS);
    (void) _spi1SendRecv(CMD_READ);             /* Command        */
    (void) _spi1SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) _spi1SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) _spi1SendRecv(addr & 0xFF);          /* Address low    */
    (void) _spi1SendRecv(0x00);                 /* Dummy byte     */

    for(size_t i = 0; i < len; i++)
    {
        buf[i] = _spi1SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);
}
