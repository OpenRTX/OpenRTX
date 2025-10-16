/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/nvmem.h"
#include <string.h>
#include "core/cps.h"
#include "core/crc.h"
#include "flash.h"

/*
 * Data structures defining the memory layout used for saving and restore
 * of user settings and VFO configuration.
 */
typedef struct
{
    uint16_t   crc;
    settings_t settings;
    channel_t  vfoData;
}
__attribute__((packed)) dataBlock_t;

typedef struct
{
    uint32_t    magic;
    uint32_t    flags[32];
    dataBlock_t data[1024];
}
__attribute__((packed)) memory_t;

static const uint32_t MEM_MAGIC   = 0x584E504F;    // "OPNX"
static const uint32_t baseAddress = 0x080E0000;
memory_t *memory = ((memory_t *) baseAddress);


/**
 * \internal
 * Utility function to find the currently active data block inside memory, that
 * is the one containing the last saved settings. Blocks containing legacy data
 * are marked with numbers starting from 4096.
 *
 * @return number currently active data block or -1 if memory data is invalid.
 */
static int findActiveBlock()
{
    // Check for invalid memory data
    if(memory->magic != MEM_MAGIC)
        return -1;

    uint16_t block = 0;
    uint16_t bit   = 0;

    // Find the first 32-bit block not full of zeroes
    for(; block < 32; block++)
    {
        if(memory->flags[block] != 0x00000000)
        {
            break;
        }
    }

    // Find the last zero within a block
    for(; bit < 32; bit++)
    {
        if((memory->flags[block] & (1 << bit)) != 0)
        {
            break;
        }
    }

    block = (block * 32) + bit;
    block -= 1;

    // Check data validity
    uint16_t crc = crc_ccitt(&(memory->data[block].settings),
                             sizeof(settings_t) + sizeof(channel_t));
    if(crc != memory->data[block].crc)
        return -2;

    return block;
}


int nvm_readVfoChannelData(channel_t *channel)
{
    int block = findActiveBlock();

    // Invalid data found
    if(block < 0) return -1;

    memcpy(channel, &(memory->data[block].vfoData), sizeof(channel_t));

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    int block = findActiveBlock();

    // Invalid data found
    if(block < 0) return -1;

    memcpy(settings, &(memory->data[block].settings), sizeof(settings_t));

    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    uint32_t addr    = 0;
    int      block   = findActiveBlock();
    uint16_t prevCrc = 0;

    /*
     * Memory never initialised or save space finished: erase all the sector.
     * On STM32F405 the settings are saved in sector 11, starting at address
     * 0x08060000.
     */
    if((block < 0) || (block >= 2047))
    {
        flash_eraseSector(11);
        addr = ((uint32_t) &(memory->magic));
        flash_write(addr, &MEM_MAGIC, sizeof(MEM_MAGIC));
        block = 0;
    }
    else
    {
        prevCrc = memory->data[block].crc;
        block += 1;
    }

    dataBlock_t tmpBlock;
    memcpy((&tmpBlock.settings), settings, sizeof(settings_t));
    memcpy((&tmpBlock.vfoData), vfo, sizeof(channel_t));
    tmpBlock.crc = crc_ccitt(&(tmpBlock.settings),
                             sizeof(settings_t) + sizeof(channel_t));

    // New data is equal to the old one, avoid saving
    if((block != 0) && (tmpBlock.crc == prevCrc))
        return 0;

    // Save data
    addr = ((uint32_t) &(memory->data[block]));
    flash_write(addr, &tmpBlock, sizeof(dataBlock_t));

    // Update the flags marking used data blocks
    uint32_t flag = ~(1 << (block % 32));
    addr = ((uint32_t) &(memory->flags[block / 32]));
    flash_write(addr, &flag, sizeof(uint32_t));

    return 0;
}
