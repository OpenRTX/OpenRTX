/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32h743xx.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "flash.h"

/**
 * \internal
 * Utility function performing unlock of flash erase and write access in bank 1.
 *
 * @return true on success, false on failure.
 */
static inline bool unlock_bank1()
{
    // Flash already unlocked
    if((FLASH->CR1 & FLASH_CR_LOCK) == 0)
    {
        return true;
    }

    FLASH->KEYR1 = 0x45670123;
    FLASH->KEYR1 = 0xCDEF89AB;

    // Succesful unlock
    if((FLASH->CR1 & FLASH_CR_LOCK) == 0)
    {
        return true;
    }

    return false;
}

/**
 * \internal
 * Utility function performing unlock of flash erase and write access in bank 2.
 *
 * @return true on success, false on failure.
 */
static inline bool unlock_bank2()
{
    // Flash already unlocked
    if((FLASH->CR2 & FLASH_CR_LOCK) == 0)
    {
        return true;
    }

    FLASH->KEYR2 = 0x45670123;
    FLASH->KEYR2 = 0xCDEF89AB;

    // Succesful unlock
    if((FLASH->CR2 & FLASH_CR_LOCK) == 0)
    {
        return true;
    }

    return false;
}

bool flash_eraseSector_bank(const uint8_t secNum, const uint8_t bank)
{
    if(secNum > 7) return false;

    if(bank == 1)
    {
        if(unlock_bank1() == false) return false;

        // Flash busy, wait until previous operation finishes
        while((FLASH->SR1 & FLASH_SR_BSY) != 0) ;

        FLASH->CR1 |= FLASH_CR_SER;                 // Sector erase
        FLASH->CR1 &= ~FLASH_CR_SNB;
        FLASH->CR1 |= (secNum << FLASH_CR_SNB_Pos); // Sector number
        FLASH->CR1 |= FLASH_CR_START;                // Start erase

        // Wait until erase ends
        while((FLASH->SR1 & FLASH_SR_QW) != 0) ;

        FLASH->CR1 &= ~FLASH_CR_SER;

        return true;
    }
    else if(bank == 2)
    {
        if(unlock_bank2() == false) return false;

        // Flash busy, wait until previous operation finishes
        while((FLASH->SR2 & FLASH_SR_BSY) != 0) ;

        FLASH->CR2 |= FLASH_CR_SER;                 // Sector erase
        FLASH->CR2 &= ~FLASH_CR_SNB;
        FLASH->CR2 |= (secNum << FLASH_CR_SNB_Pos); // Sector number
        FLASH->CR2 |= FLASH_CR_START;                // Start erase

        // Wait until erase ends
        while((FLASH->SR2 & FLASH_SR_QW) != 0) ;

        FLASH->CR2 &= ~FLASH_CR_SER;

        return true;
    }

    return false;
}

bool flash_eraseSector(const uint8_t secNum)
{
    return flash_eraseSector_bank(secNum%8, 1 + (secNum/8) );
}

bool flash_write(const uint32_t address, const void *data, const size_t len)
{
    if((data == NULL) || (len == 0)) return false;

    if((address & 0x1F) || (len & 0x1F)) return false; // area is not fully 32 bytes aligned

    if(address < 0x8100000)
    {
        if(unlock_bank1() == false) return false;
    }

    if(address + len >= 0x8100000)
    {
        if(unlock_bank2() == false) return false;
    }

    size_t total_words = len/4; // Number of 32 bits words
    size_t bank1_words = 0;
    if(address < 0x8100000)
    {
        bank1_words = (0x8100000-address)/4;
        if(bank1_words > total_words)
            bank1_words = total_words;
    }

    // Write data to memory, 32 bits at a time
    const uint32_t *buf = ((uint32_t *) data);
    uint32_t *mem       = ((uint32_t *) address);

    size_t i = 0; // Written words
    if(bank1_words > 0) // Writes to bank 1
    {
        while((FLASH->SR1 & FLASH_SR_BSY) != 0) ;
        FLASH->CR1 = FLASH_CR_PG;

        while(i < bank1_words)
        {
            memcpy(mem, buf+i, 8);
            mem += 8;
            i += 8;

            while((FLASH->SR1 & FLASH_SR_QW) != 0) ;
        }

        FLASH->CR1 &= ~FLASH_CR_PG;
    }
    if(i < total_words) // Writes to bank 2
    {
        while((FLASH->SR2 & FLASH_SR_BSY) != 0) ;
        FLASH->CR2 = FLASH_CR_PG;

        while(i < total_words)
        {
            memcpy(mem, buf+i, 8);
            mem += 8;
            i += 8;
            
            while((FLASH->SR2 & FLASH_SR_QW) != 0) ;
        }

        FLASH->CR2 &= ~FLASH_CR_PG;
    }

    return true;
}

uint16_t flash_size()
{
    return *(uint16_t*)(FLASH_SIZE_DATA_REGISTER);
}