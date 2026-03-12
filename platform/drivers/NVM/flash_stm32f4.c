/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <interfaces/nvmem.h>
#include <stddef.h>
#include <string.h>
#include <errno.h>
#include "stm32f4xx.h"
#include "flash_stm32.h"

struct stm32FlashArea stm32Flash_priv[] = {
    {
        // SECTOR_16K (4 sectors)
        .address_low = 0x8000000,
        .address_high = 0x800FFFF,
        .first_sector = 0,
    },
    {
        // SECTOR_64K (1 sector)
        .address_low = 0x8010000,
        .address_high = 0x801FFFF,
        .first_sector = 4,
    },
    {
        // SECTOR_128K (3 or 5 sectors, variable size)
        .address_low = 0x8020000,
        .address_high = 0xFFFFFFFF,
        .first_sector = 5,
    },
};

/**
 * \internal
 * Utility function performing unlock of flash erase and write access.
 *
 * @return true on success, false on failure.
 */
static inline bool unlock()
{
    // Flash already unlocked
    if ((FLASH->CR & FLASH_CR_LOCK) == 0)
        return true;

    FLASH->KEYR = 0x45670123;
    FLASH->KEYR = 0xCDEF89AB;

    if ((FLASH->CR & FLASH_CR_LOCK) == 0)
        return true;

    return false;
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
    if (unlock() == false)
        return -EIO;

    if (secNum > 11)
        return -EINVAL;

    // Flash busy, wait until previous operation finishes
    while ((FLASH->SR & FLASH_SR_BSY) != 0)
        ;

    FLASH->CR |= FLASH_CR_SER;  // Sector erase
    FLASH->CR &= ~FLASH_CR_SNB;
    FLASH->CR |= (secNum << 3); // Sector number
    FLASH->CR |= FLASH_CR_STRT; // Start erase

    // Wait until erase ends
    while ((FLASH->SR & FLASH_SR_BSY) != 0)
        ;

    FLASH->CR &= ~FLASH_CR_SER;

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
static int checkAddress(const struct nvmDevice *dev, const uint32_t address,
                        const size_t len)
{
    const struct stm32FlashArea *config = (struct stm32FlashArea *)(dev->priv);

    if (address < config->address_low)
        return -EINVAL; // Below address pace

    if (address + len > config->address_high)
        return -EINVAL; // Above address space

    return 0;
}

int stm32Flash_init(struct nvmDevice *dev, size_t sector_size)
{
    switch (sector_size) {
        case 16384:
            dev->priv = &stm32Flash_priv[0];
            break;

        case 65536:
            dev->priv = &stm32Flash_priv[1];
            break;

        case 131072: {
            struct stm32FlashArea *tmp = &stm32Flash_priv[2];
            tmp->address_high = (FLASH_BASE + stm32Flash_size() * 1024) - 1;
            dev->priv = tmp;
        } break;

        default:
            return -EINVAL;
    }

    struct nvmInfo *infos = (struct nvmInfo *)dev->info;
    infos->write_size = 1;
    infos->erase_size = sector_size;
    infos->erase_cycles = 10000;
    infos->device_info = NVM_FLASH | NVM_WRITE | NVM_ERASE;

    return 0;
}

int stm32Flash_terminate(struct nvmDevice *dev)
{
    // Nothing to do here
    (void)dev;

    return 0;
}

uint16_t stm32Flash_size()
{
    return *(uint16_t *)(0x1FFF7A22);
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t address,
                        void *data, size_t len)
{
    int ret = checkAddress(dev, address, len);
    if (ret < 0)
        return ret;

    memcpy(data, (void *)address, len);

    return 0;
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t address,
                         const void *data, size_t len)
{
    int ret = checkAddress(dev, address, len);
    if (ret < 0)
        return ret;

    if (unlock() == false)
        return -EIO;

    if ((data == NULL) || (len == 0))
        return -EINVAL;

    // Write data to memory, 8 bit at a time
    const uint8_t *buf = ((uint8_t *)data);
    uint8_t *mem = ((uint8_t *)address);

    for (size_t i = 0; i < len; i++) {
        while ((FLASH->SR & FLASH_SR_BSY) != 0)
            ;

        FLASH->CR = FLASH_CR_PG;
        *mem = buf[i];
        mem++;

        while ((FLASH->SR & FLASH_SR_BSY) != 0)
            ;

        FLASH->CR &= ~FLASH_CR_PG;
    }

    return 0;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t address,
                         size_t len)
{
    struct stm32FlashArea *config = (struct stm32FlashArea *)(dev->priv);
    size_t first_sector = config->first_sector;
    size_t nb_sectors = len / dev->info->erase_size;

    int ret = checkAddress(dev, address, len);
    if (ret < 0)
        return ret;

    first_sector += (address - config->address_low) / dev->info->erase_size;
    for (size_t i = 0; i < nb_sectors; i++) {
        int ret = eraseSector(first_sector + i);
        if (ret < 0)
            return ret;
    }

    return 0;
}

struct nvmOps stm32Flash_ops = {
    .read = nvm_api_read,
    .write = nvm_api_write,
    .erase = nvm_api_erase,
    .sync = NULL,
};
