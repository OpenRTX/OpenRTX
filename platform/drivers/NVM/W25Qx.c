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

#include "W25Qx.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <hwconfig.h>
#include <interfaces/gpio.h>

#define CMD_READ 0x03   /* Read data              */
#define CMD_RSEC 0x48   /* Read security register */
#define CMD_WKUP 0xAB   /* Release power down     */
#define CMD_PDWN 0xB9   /* Power down             */

/*
 * Target-specific SPI interface functions, their implementation can be found
 * in source files "spiFlash_xxx.c"
 */
extern uint8_t spiFlash_SendRecv(uint8_t val);
extern void spiFlash_init();
extern void spiFlash_terminate();

void W25Qx_init()
{
    gpio_setMode(FLASH_CS, OUTPUT);
    gpio_setPin(FLASH_CS);

    spiFlash_init();
}

void W25Qx_terminate()
{
    W25Qx_sleep();

    gpio_setMode(FLASH_CS,  INPUT);

    spiFlash_terminate();
}

void W25Qx_wakeup()
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_WKUP);
    gpio_setPin(FLASH_CS);
}

void W25Qx_sleep()
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_PDWN);
    gpio_setPin(FLASH_CS);
}

ssize_t W25Qx_readSecurityRegister(uint32_t addr, uint8_t* buf, size_t len)
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
    (void) spiFlash_SendRecv(CMD_RSEC);             /* Command        */
    (void) spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) spiFlash_SendRecv(addr & 0xFF);          /* Address low    */
    (void) spiFlash_SendRecv(0x00);                 /* Dummy byte     */

    for(size_t i = 0; i < readLen; i++)
    {
        buf[i] = spiFlash_SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);

    return ((ssize_t) readLen);
}

void W25Qx_readData(uint32_t addr, uint8_t* buf, size_t len)
{
    gpio_clearPin(FLASH_CS);
    (void) spiFlash_SendRecv(CMD_READ);             /* Command        */
    (void) spiFlash_SendRecv((addr >> 16) & 0xFF);  /* Address high   */
    (void) spiFlash_SendRecv((addr >> 8) & 0xFF);   /* Address middle */
    (void) spiFlash_SendRecv(addr & 0xFF);          /* Address low    */

    for(size_t i = 0; i < len; i++)
    {
        buf[i] = spiFlash_SendRecv(0x00);
    }

    gpio_setPin(FLASH_CS);
}
