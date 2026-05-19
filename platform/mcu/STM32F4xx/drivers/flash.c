/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32f4xx.h"
#include <stddef.h>
#include "flash.h"

/**
 * \internal
 * Utility function performing unlock of flash erase and write access.
 *
 * @true on success, false on failure.
 */
static inline bool unlock()
{
    // Flash already unlocked
    if((FLASH->CR & FLASH_CR_LOCK) == 0)
    {
        return true;
    }

    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    // Succesful unlock
    if((FLASH->CR & FLASH_CR_LOCK) == 0)
    {
        return true;
    }

    return false;
}



bool flash_eraseSector(const uint8_t secNum)
{
    if(secNum > 11) return false;
    if(unlock() == false) return false;

    // Flash busy, wait until previous operation finishes
    while((FLASH->SR & FLASH_SR_BSY) != 0) ;

    FLASH->CR |= FLASH_CR_SER;      // Sector erase
    FLASH->CR &= ~FLASH_CR_SNB;
    FLASH->CR |= (secNum << 3);     // Sector number
    FLASH->CR |= FLASH_CR_STRT;     // Start erase

    // Wait until erase ends
    while((FLASH->SR & FLASH_SR_BSY) != 0) ;

    FLASH->CR &= ~FLASH_CR_SER;

    return true;
}

void flash_write(const uint32_t address, const void *data, const size_t len)
{
    if(unlock() == false) return;
    if((data == NULL) || (len == 0)) return;

    // Write data to memory, 8 bit at a time
    const uint8_t *buf = ((uint8_t *) data);
    uint8_t *mem       = ((uint8_t *) address);
    for(size_t i = 0; i < len; i++)
    {
        while((FLASH->SR & FLASH_SR_BSY) != 0) ;
        FLASH->CR = FLASH_CR_PG;

        *mem = buf[i];
        mem++;

        while((FLASH->SR & FLASH_SR_BSY) != 0) ;
        FLASH->CR &= ~FLASH_CR_PG;
    }
}
