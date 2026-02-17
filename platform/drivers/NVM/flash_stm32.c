/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "flash_stm32.h"
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include "interfaces/arch_registers.h"
#include "flash.h"
#include "interfaces/nvmem.h"


const struct nvmInfo stm32_flash_infos[] = {
#if defined(STM32F405xx) || defined(STM32F415xx) || \
    defined(STM32F407xx) || defined(STM32F417xx)    
    {
        .write_size = 1,
        .erase_size = 16384,
        .erase_cycles = 10000,
        .device_info = NVM_FLASH | NVM_WRITE | NVM_ERASE,
    },
    {
        .write_size = 1,
        .erase_size = 65536,
        .erase_cycles = 10000,
        .device_info = NVM_FLASH | NVM_WRITE | NVM_ERASE,
    },
    {
        .write_size = 1,
        .erase_size = 131072,
        .erase_cycles = 10000,
        .device_info = NVM_FLASH | NVM_WRITE | NVM_ERASE,
    },
#elif defined(STM32H743xx)
    { 
        .write_size = 32,
        .erase_size = 131072,
        .erase_cycles = 10000,
        .device_info = NVM_FLASH | NVM_WRITE | NVM_ERASE,
    },
#else
    #error "Missing flash memory infos for this device."
#endif      
};


const struct STM32FlashArea stm32_flash_priv[] = {
#if defined(STM32F405xx) || defined(STM32F415xx) || \
    defined(STM32F407xx) || defined(STM32F417xx)
    { // SECTOR_16K (4 sectors)
        .address_low = 0x8000000,
        .address_high = 0x800FFFF,
        .first_sector = 0,
    },
    { // SECTOR_64K (1 sector)
        .address_low = 0x8010000,
        .address_high = 0x801FFFF,
        .first_sector = 4,
    },
    { // SECTOR_128K (3 or 5 sectors, variable size)
        .address_low = 0x8020000,
        .address_high = 0xFFFFFFFF, 
        .first_sector = 5,
    },
#elif defined(STM32H743xx)
    { // SECTOR_128K (8 sectors)
        .address_low = 0x8000000,
        .address_high = 0x81FFFFF,
        .first_sector = 0,
    },
#else
    #error "Missing flash memory layout for this device."
#endif    
};




int STM32Flash_init(struct nvmDevice *dev)
{
    // Nothing to do here
    (void) dev;
    return 0;
}


int STM32Flash_terminate(struct nvmDevice *dev)
{
    // Nothing to do here
    (void) dev;
    return 0;
}

/**
 * Performs sanity checks before accessing the flash memory. 
 * Checks that access is within memory bounds (accounts for variable page size)
 * 
 * @param dev pointer to the device for which to perform the sanity checks
 * @param address address that will be used for later access
 * @param len length of the data to read/write/erase
 * @return 0 if the checks succeed, negative error code otherwise
 */
int STM32Flash_check(const struct nvmDevice *dev, const uint32_t address, const size_t len)
{
    const struct STM32FlashArea *config = (struct STM32FlashArea *)(dev->priv);
    
    if(address < config->address_low) return -EINVAL; // Below address pace

    if(config->address_high == 0xFFFFFFFF) // Check against device memory size
    {
        if( (address + len) > (FLASH_BASE + (flash_size()<<10)) ) return -EINVAL; // Above address space
    }
    else if(address + len > config->address_high) return -EINVAL; // Above address space

    return 0;
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t address,
                        void *data, size_t len)
{   
    int ret = STM32Flash_check(dev, address, len);
    if(ret < 0) return ret;
    
    uint32_t* flash_addr_32 = (uint32_t*)(address);

    size_t i = 0;
    for(; i < len/4; i++)
    {
        ((uint32_t *)data)[i] = flash_addr_32[i];
    }

    char* flash_addr_8 = (char*)(address);

    for(i *= 4; i < len; i++)
    {
        ((char *)data)[i] = flash_addr_8[i];
    }

    return 0;
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t address,
                         const void *data, size_t len)
{
    int ret = STM32Flash_check(dev, address, len);
    if(ret < 0) return ret;

    if(flash_write(address, data, len)) return 0;
    return -EIO;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t address, size_t len)
{
    const struct STM32FlashArea *config = (const struct STM32FlashArea*)(dev->priv);
    int ret = STM32Flash_check(dev, address, len);
    if(ret < 0) return ret;

    size_t first_sector = config->first_sector;
    first_sector += (address - config->address_low) / dev->info->erase_size;

    size_t nb_sectors = len / dev->info->erase_size;


    for(size_t i = 0; i < nb_sectors; i++)
    {
        if(!flash_eraseSector(first_sector + i))
            return -EIO; // Could not erase flash
    }

    return 0;
}

const struct nvmOps stm32_flash_ops =
{
    .read = nvm_api_read,
    .write = nvm_api_write,
    .erase = nvm_api_erase,
    .sync = NULL,
};

