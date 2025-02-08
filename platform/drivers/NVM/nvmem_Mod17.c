/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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

#include <interfaces/nvmem.h>
#include <calibInfo_Mod17.h>
#include <string.h>
#include <crc.h>
#include "flash.h"

/*
 * Data structures defining the memory layout used for saving and restore
 * of user settings and VFO configuration.
 */
typedef struct
{
    uint16_t     crc;
    settings_t   settings;
    mod17Calib_t calibration;
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

mod17Calib_t mod17CalData;   // Calibration data, to be saved and loaded

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
    const size_t crcLen = sizeof(settings_t) + sizeof(mod17Calib_t);
    uint16_t crc = crc_ccitt(&(memory->data[block].settings), crcLen);
    if(crc != memory->data[block].crc)
        return -2;

    return block;
}

void nvm_init()
{

}

void nvm_terminate()
{

}

void nvm_readCalibData(void *buf)
{
    (void) buf;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    // Module 17 has no channels: just load default values for it
    channel->mode         = OPMODE_M17;
    channel->bandwidth    = BW_12_5;
    channel->power        = 1.0;
    channel->rx_frequency = 430000000;
    channel->tx_frequency = 430000000;
    channel->fm.rxToneEn  = 0; //disabled
    channel->fm.rxTone    = 0; //and no ctcss/dcs selected
    channel->fm.txToneEn  = 0;
    channel->fm.txTone    = 0;

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    int block = findActiveBlock();

    // Invalid data found
    if(block < 0) return -1;

    memcpy(settings,      &(memory->data[block].settings),    sizeof(settings_t));
    memcpy(&mod17CalData, &(memory->data[block].calibration), sizeof(mod17Calib_t));

    return 0;
}

int nvm_writeSettings(const settings_t *settings)
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
    memcpy((&tmpBlock.settings),    settings,      sizeof(settings_t));
    memcpy((&tmpBlock.calibration), &mod17CalData, sizeof(mod17Calib_t));

    const size_t crcLen = sizeof(settings_t) + sizeof(mod17Calib_t);
    tmpBlock.crc = crc_ccitt(&(tmpBlock.settings), crcLen);

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

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    (void) vfo;
    return nvm_writeSettings(settings);
}
