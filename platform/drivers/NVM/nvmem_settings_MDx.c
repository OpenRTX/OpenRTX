/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
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

#include <string.h>
#include <interfaces/nvmem.h>
#include <cps.h>
#include "flash.h"

/*
 * Data structure defining the memory layout used for saving and restore
 * of user settings and VFO configuration.
 */
typedef struct
{
    uint32_t magic;
    uint32_t flags[64];
    struct dataBlock
    {
        settings_t settings;
        channel_t  vfoData;
    }
    data[2048];
}
__attribute__((packed)) memory_t;

static const uint32_t validMagic  = 0x584E504F;    // "OPNX"
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
int findActiveBlock()
{
    if(memory->magic != validMagic)
    {
        return -1;     // Invalid memory data
    }

    uint16_t block = 0;
    for(; block < 64; block++)
    {
        if(memory->flags[block] != 0x00000000)
        {
            break;
        }
    }

    uint16_t bit = 0;
    for(; bit < 32; bit++)
    {
        if((memory->flags[block] & (1 << bit)) != 0)
        {
            break;
        }
    }

    block = (block * 32) + bit;
    return block - 1;
}



int nvm_readVFOChannelData(channel_t *channel)
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
    uint32_t addr = 0;
    int block = findActiveBlock();

    /*
     * Memory never initialised or save space finished: erase all the sector.
     * On STM32F405 the settings are saved in sector 11, starting at address
     * 0x08060000.
     */
    if((block < 0) || (block >= 2047))
    {
        flash_eraseSector(11);
        addr = ((uint32_t) &(memory->magic));
        flash_write(addr, &validMagic, sizeof(validMagic));
        block = 0;
    }
    else
    {
        block += 1;
    }

    // Save settings
    addr = ((uint32_t) &(memory->data[block].settings));
    flash_write(addr, settings, sizeof(settings_t));

    // Save VFO configuration
    addr = ((uint32_t) &(memory->data[block].vfoData));
    flash_write(addr, vfo, sizeof(channel_t));

    // Update the flags marking used data blocks
    uint32_t flag = ~(1 << (block % 32));
    addr = ((uint32_t) &(memory->flags[block / 32]));
    flash_write(addr, &flag, sizeof(uint32_t));

    return 0;
}
