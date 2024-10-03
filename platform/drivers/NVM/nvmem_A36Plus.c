/***************************************************************************
 *   Copyright (C) 2020 - 2024 by Federico Amedeo Izzo IU2NUO,             *
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

#include <string.h>
#include <wchar.h>
#include <interfaces/delays.h>
#include <interfaces/nvmem.h>
#include <calibInfo_A36Plus.h>
#include <utils.h>
#include "AT24Cx.h"
#include "W25Qx.h"

W25Qx_DEVICE_DEFINE(eflash,  0x200000)  // 2 MB,  16 Mbit

static const struct nvmDescriptor nvmDevices[] =
{
    {
        .name       = "External flash",
        .dev        = &eflash,
        .partNum    = 0,
        .partitions = NULL
    }
};


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

static const uint32_t CAL_BASE = 0xF000;

static const uint32_t baseAddress = 0x000A1000;     // 0x000A1000;

// A function that dumps a portion of external flash to UART
#if 0
void nvm_dumpFlash()
{
    W25Qx_wakeup();
    delayUs(5);

    uint8_t buf[16];
    // there's no printf, so use sprintf to write to a buffer and then write the buffer to UART
    char str[64];
    for(uint32_t i = 0; i < 0x4000; i += 16)
    {
        W25Qx_readData(i, buf, 16);
        sprintf(str, "\r\nx%08X ", i);
        usart0_IRQwrite(str);
        for(uint8_t j = 0; j < 16; j++)
        {
            sprintf(str, "%02X", buf[j]);
            usart0_IRQwrite(str);
        }
    }

    //W25Qx_sleep();
}
#endif

void nvm_init()
{
    W25Qx_init();
}

void nvm_terminate()
{
    W25Qx_terminate();
}

const struct nvmDescriptor *nvm_getDesc(const size_t index)
{
    if(index > 1)
        return NULL;

    return &nvmDevices[index];
}

void nvm_readCalibData(void *buf)
{
    W25Qx_wakeup();
    delayUs(5);

    PowerCalibrationTables *calib = ((PowerCalibrationTables *) buf);

    // Load calibration data
    uint32_t addr = CAL_BASE;
    W25Qx_readData(addr, &calib->high, sizeof(PowerCalibration));
    addr += sizeof(PowerCalibration);
    W25Qx_readData(addr, &calib->med, sizeof(PowerCalibration));
    addr += sizeof(PowerCalibration);
    W25Qx_readData(addr, &calib->low, sizeof(PowerCalibration));

    // W25Qx_sleep();
}

void nvm_readHwInfo(hwInfo_t *info)
{
    /* Not sure yet. */
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    // First, read the first 2 bytes of the memory block. Are they FF?
    uint16_t crc;
    W25Qx_readData(baseAddress, &crc, sizeof(uint16_t));
    if(crc != 0x6969)
    {
        return -1;
    }
    // dataBlock is in the following order: crc, settings, vfoData
    // Read vfoData from the external flash: read sizeof(vfoData)
    // bytes starting from the address of the first byte of vfoData
    // (which is sizeof(uint16_t) + sizeof(settings_t) bytes from the start)
    W25Qx_readData(baseAddress + sizeof(uint16_t) + sizeof(settings_t),
                    channel, sizeof(channel_t));
    // Check if the channel is valid
    if(channel->rx_frequency == 0)
    {
        return -1;
    }
    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    // First, read the first 2 bytes of the memory block. Are they FF?
    uint16_t crc;
    W25Qx_readData(baseAddress, &crc, sizeof(uint16_t));
    if(crc != 0x6969)
    {
        return -1;
    }
    // dataBlock is in the following order: crc, settings, vfoData
    // Read settings from the external flash: read sizeof(settings)
    // bytes starting from the address of the first byte of settings
    // (which is sizeof(uint16_t) bytes from the start)
    W25Qx_readData(baseAddress + sizeof(uint16_t),
                    settings, sizeof(settings_t));
    if (settings->brightness == 0) {
        return -1;
    }
    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    W25Qx_wakeup();
    W25Qx_erase(baseAddress, 4096);
    // Create datablock of settings and vfoData
    dataBlock_t dataBlock;
    dataBlock.crc = 0x6969;
    memcpy(&dataBlock.settings, settings, sizeof(settings_t));
    memcpy(&dataBlock.vfoData, vfo, sizeof(channel_t));
    // Write dataBlock to the external flash: write sizeof(dataBlock)
    if(W25Qx_writeData(baseAddress, &dataBlock, sizeof(dataBlock_t)) < 0)
    {
        return -1;
    }
    return 0;
}