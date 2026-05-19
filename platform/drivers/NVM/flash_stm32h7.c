/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/nvmem.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "stm32h743xx.h"
#include "flash_stm32.h"

/**
 * \internal
 * Utility function performing unlock of flash erase and write access in bank 1.
 *
 * @return true on success, false on failure.
 */
static inline bool unlock_bank1()
{
    // Flash already unlocked
    if ((FLASH->CR1 & FLASH_CR_LOCK) == 0)
        return true;

    FLASH->KEYR1 = 0x45670123;
    FLASH->KEYR1 = 0xCDEF89AB;
    __ISB();

    // Succesful unlock
    if ((FLASH->CR1 & FLASH_CR_LOCK) == 0)
        return true;

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
    if ((FLASH->CR2 & FLASH_CR_LOCK) == 0)
        return true;

    FLASH->KEYR2 = 0x45670123;
    FLASH->KEYR2 = 0xCDEF89AB;
    __ISB();

    // Succesful unlock
    if ((FLASH->CR2 & FLASH_CR_LOCK) == 0)
        return true;

    return false;
}

static bool eraseSector_bank1(const uint8_t secNum)
{
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
}

static bool eraseSector_bank2(const uint8_t secNum)
{
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

/**
 * \internal
 * Erase one sector of the MCU flash memory.
 *
 * @param secNum: sector number.
 * @return true for successful erase, false otherwise.
 */
static bool eraseSector(const uint8_t secNum)
{
    uint8_t bank = secNum / 8;
    uint8_t sector = secNum % 8;

    if (sector > 7)
        return false;

    switch (bank) {
        case 0:
            return eraseSector_bank1(sector);
            break;

        case 1:
            return eraseSector_bank2(sector);
            break;
    }

    return false;
}

static void write_bank1(void *mem, const void *buf, const size_t len)
{
    while ((FLASH->SR1 & FLASH_SR_QW) != 0)
        ;

    FLASH->CR1 = FLASH_CR_PG;

    memcpy(mem, buf, len);
    __DSB();

    while ((FLASH->SR1 & FLASH_SR_QW) != 0)
        ;

    FLASH->CR1 &= ~FLASH_CR_PG;
}

static void write_bank2(void *mem, const void *buf, const size_t len)
{
    while ((FLASH->SR2 & FLASH_SR_QW) != 0)
        ;

    FLASH->CR2 = FLASH_CR_PG;

    memcpy(mem, buf, len);
    __DSB();

    while ((FLASH->SR2 & FLASH_SR_QW) != 0)
        ;

    FLASH->CR2 &= ~FLASH_CR_PG;
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
static bool writeData(const uint32_t address, const void *data,
                      const size_t len)
{
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

    if (bank1_len > 0) {
        write_bank1(mem, buf, bank1_len);
        mem += bank1_len;
        buf += bank1_len;
    }

    if (bank1_len < total_len)
        write_bank2(mem, buf, total_len - bank1_len);

    return true;
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
static int checkAddress(const uint32_t address, const size_t len)
{
    if (address < FLASH_BASE)
        return -EINVAL; // Below address pace

    if ((address + len) > FLASH_END)
        return -EINVAL; // Above address space

    return 0;
}

int stm32Flash_init(struct nvmDevice *dev, size_t sector_size)
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

int stm32Flash_terminate(struct nvmDevice *dev)
{
    (void)dev;

    return 0;
}

uint16_t stm32Flash_size()
{
    return *(uint16_t *)(FLASH_SIZE_DATA_REGISTER);
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t address,
                        void *data, size_t len)
{
    (void)(dev);
    int ret = checkAddress(address, len);
    if (ret < 0)
        return ret;

    memcpy(data, (void *)address, len);

    return 0;
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t address,
                         const void *data, size_t len)
{
    (void)(dev);

    int ret = checkAddress(address, len);
    if (ret < 0)
        return ret;

    if (writeData(address, data, len))
        return 0;

    return -EIO;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t address,
                         size_t len)
{
    (void)(dev);
    int ret = checkAddress(address, len);
    if (ret < 0)
        return ret;

    size_t first_sector = (address - FLASH_BASE) / FLASH_SECTOR_SIZE;
    size_t nb_sectors = len / FLASH_SECTOR_SIZE;

    for (size_t i = 0; i < nb_sectors; i++) {
        if (!eraseSector(first_sector + i))
            return -EIO; // Could not erase flash
    }

    return 0;
}

struct nvmOps stm32Flash_ops = {
    .read = nvm_api_read,
    .write = nvm_api_write,
    .erase = nvm_api_erase,
    .sync = NULL,
};
