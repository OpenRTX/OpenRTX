/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "stm32h743xx.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "drivers/NVM/internal_flash.h"
#include "interfaces/nvmem.h"
#include <errno.h>

/**
 * \internal
 * Utility function performing unlock of flash erase and write access in bank 1.
 *
 * @return true on success, false on failure.
 */
static inline bool unlock_bank1()
{
    // Flash already unlocked
    if ((FLASH->CR1 & FLASH_CR_LOCK) == 0) {
        return true;
    }

    FLASH->KEYR1 = 0x45670123;
    FLASH->KEYR1 = 0xCDEF89AB;
    __ISB();

    // Succesful unlock
    if ((FLASH->CR1 & FLASH_CR_LOCK) == 0) {
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
    if ((FLASH->CR2 & FLASH_CR_LOCK) == 0) {
        return true;
    }

    FLASH->KEYR2 = 0x45670123;
    FLASH->KEYR2 = 0xCDEF89AB;
    __ISB();

    // Succesful unlock
    if ((FLASH->CR2 & FLASH_CR_LOCK) == 0) {
        return true;
    }

    return false;
}

bool internalFlash_eraseSector_bank(const uint8_t secNum, const uint8_t bank)
{
    if (secNum > 7)
        return false;

    if (bank == 1) {
        if (unlock_bank1() == false)
            return false;

        // Flash busy, wait until previous operation finishes
        while ((FLASH->SR1 & FLASH_SR_BSY) != 0)
            ;

        FLASH->CR1 |= FLASH_CR_SER;                 // Sector erase
        FLASH->CR1 &= ~FLASH_CR_SNB;
        FLASH->CR1 |= (secNum << FLASH_CR_SNB_Pos); // Sector number
        __ISB();
        FLASH->CR1 |= FLASH_CR_START;               // Start erase

        // Wait until erase ends
        while ((FLASH->SR1 & FLASH_SR_QW) != 0)
            ;
        FLASH->CR1 &= ~FLASH_CR_SER;

        return true;
    } else if (bank == 2) {
        if (unlock_bank2() == false)
            return false;

        // Flash busy, wait until previous operation finishes
        while ((FLASH->SR2 & FLASH_SR_BSY) != 0)
            ;

        FLASH->CR2 |= FLASH_CR_SER;                 // Sector erase
        FLASH->CR2 &= ~FLASH_CR_SNB;
        FLASH->CR2 |= (secNum << FLASH_CR_SNB_Pos); // Sector number
        __ISB();
        FLASH->CR2 |= FLASH_CR_START;               // Start erase

        // Wait until erase ends
        while ((FLASH->SR2 & FLASH_SR_QW) != 0)
            ;
        FLASH->CR2 &= ~FLASH_CR_SER;

        return true;
    }

    return false;
}

/**
 * \internal
 * Erase one sector of the MCU flash memory.
 *
 * @param secNum: sector number.
 * @return true for successful erase, false otherwise.
 */
bool internalFlash_eraseSector(const uint8_t secNum)
{
    return internalFlash_eraseSector_bank(secNum % 8, 1 + (secNum / 8));
}

/**
 * \internal
 * Write data to the MCU flash memory.
 *
 * @param address: starting address for the write operation.
 * @param data: data to be written.
 * @param len: data length.
 * @return true for successful write, false otherwise
 */
bool internalFlash_write(const uint32_t address, const void *data,
                         const size_t len)
{
    if ((data == NULL) || (len == 0))
        return false;

    if ((address & 0x1F) || (len & 0x1F))
        return false; // area is not fully 32 bytes aligned

    if (address < FLASH_BANK2_BASE) {
        if (unlock_bank1() == false)
            return false;
    }

    if (address + len >= FLASH_BANK2_BASE) {
        if (unlock_bank2() == false)
            return false;
    }

    size_t total_len = len; // Number of 32 bits words
    size_t bank1_len = 0;
    if (address < FLASH_BANK2_BASE) {
        bank1_len = (FLASH_BANK2_BASE - address);
        if (bank1_len > total_len)
            bank1_len = total_len;
    }

    // Write data to memory, 32 bits at a time
    const uint8_t *buf = ((uint8_t *)data);
    uint8_t *mem = ((uint8_t *)address);

    if (bank1_len > 0) // Writes to bank 1
    {
        while ((FLASH->SR1 & FLASH_SR_QW) != 0)
            ;
        FLASH->CR1 = FLASH_CR_PG;

        memcpy((void *)mem, (void *)buf, bank1_len);
        __DSB();

        while ((FLASH->SR1 & FLASH_SR_QW) != 0)
            ;

        mem += bank1_len;
        buf += bank1_len;

        FLASH->CR1 &= ~FLASH_CR_PG;
    }
    if (bank1_len < total_len) // Writes to bank 2
    {
        while ((FLASH->SR2 & FLASH_SR_QW) != 0)
            ;
        FLASH->CR2 = FLASH_CR_PG;

        memcpy((void *)mem, (void *)buf, total_len - bank1_len);
        __DSB();

        while ((FLASH->SR2 & FLASH_SR_QW) != 0)
            ;
        FLASH->CR2 &= ~FLASH_CR_PG;
    }

    return true;
}

/**
 * \internal
 * Returns the flash size in kB as stored in the device itself.
 *
 * @return device flash size in kB
*/
uint16_t internalFlash_size()
{
    return *(uint16_t *)(FLASH_SIZE_DATA_REGISTER);
}

/**
 * \internal
 * Performs sanity checks before accessing the flash memory.
 * Checks that access is within memory bounds (accounts for variable page size)
 *
 * @param dev pointer to the device for which to perform the sanity checks
 * @param address address that will be used for later access
 * @param len length of the data to read/write/erase
 * @return 0 if the checks succeed, negative error code otherwise
 */
int internalFlash_check(const uint32_t address, const size_t len)
{
    if (address < FLASH_BASE)
        return -EINVAL; // Below address pace

    if ((address + len) > FLASH_END)
        return -EINVAL; // Above address space

    return 0;
}

int internalFlash_init(struct nvmDevice *dev, size_t sector_size)
{
    if (sector_size != FLASH_SECTOR_SIZE)
        return -EINVAL;
    struct nvmInfo *infos = (struct nvmInfo *)dev->info;
    infos->write_size = 32;
    infos->erase_size = FLASH_SECTOR_SIZE;
    infos->erase_cycles = 10000;
    infos->device_info = NVM_FLASH | NVM_WRITE | NVM_ERASE;

    return 0;
}

int internalFlash_terminate(struct nvmDevice *dev)
{
    (void)dev;
    return 0;
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t address,
                        void *data, size_t len)
{
    (void)(dev);
    int ret = internalFlash_check(address, len);
    if (ret < 0)
        return ret;

    memcpy(data, (void *)address, len);

    return 0;
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t address,
                         const void *data, size_t len)
{
    (void)(dev);
    int ret = internalFlash_check(address, len);
    if (ret < 0)
        return ret;

    if (internalFlash_write(address, data, len))
        return 0;
    return -EIO;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t address,
                         size_t len)
{
    (void)(dev);
    int ret = internalFlash_check(address, len);
    if (ret < 0)
        return ret;

    size_t first_sector = (address - FLASH_BASE) / FLASH_SECTOR_SIZE;

    size_t nb_sectors = len / FLASH_SECTOR_SIZE;

    for (size_t i = 0; i < nb_sectors; i++) {
        if (!internalFlash_eraseSector(first_sector + i))
            return -EIO; // Could not erase flash
    }

    return 0;
}

struct nvmOps internalFlash_ops = {
    .read = nvm_api_read,
    .write = nvm_api_write,
    .erase = nvm_api_erase,
    .sync = NULL,
};
