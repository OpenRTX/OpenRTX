/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
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

#define __LINUX_ERRNO_EXTENSIONS__

#include <interfaces/nvmem.h>
#include <nvmem_access.h>
#include <rtxlink_dat.h>
#include <rtxlink_fmp.h>
#include <rtxlink.h>
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>
#include <state.h>

static const size_t MAX_REPLY_SIZE = 512;

enum fmpFrameType
{
    FMP_FRAME_MEMINFO = 0x01,
    FMP_FRAME_DUMP    = 0x02,
    FMP_FRAME_FLASH   = 0x03,
    FMP_FRAME_READ    = 0x04,
    FMP_FRAME_WRITE   = 0x05,
    FMP_FRAME_LIST    = 0x06,
    FMP_FRAME_MOVE    = 0x07,
    FMP_FRAME_COPY    = 0x08,
    FMP_FRAME_MKDIR   = 0x09,
    FMP_FRAME_RM      = 0x0a,
    FMP_FRAME_RESET   = 0xff,
};

struct memInfo
{
    uint32_t size;      // Size of the memory in Bytes
    uint8_t  flags;     // Memory type and access flags
    char     name[27];  // Name of the memory
};


static size_t cmd_meminfo(const uint8_t *args, const uint8_t nArg, uint8_t *reply)
{
    (void) args;
    (void) nArg;

    // Get the list of the NVM areas on the device
    const struct nvmDescriptor *area;
    size_t numAreas = 0;

    // Prepare the response frame
    size_t replySize = 3;
    reply[0]         = FMP_FRAME_MEMINFO;   // Command ID
    reply[1]         = 0x00;                // Status = OK

    // Append the memory information blocks in the reply
    while (NULL != nvm_getDesc(numAreas))
    {
        numAreas++;
        // Set length of the parameter and parameter data
        reply[replySize] = sizeof(struct memInfo);
        replySize       += 1;
    }

    reply[2]         = numAreas;            // Num of elements

    for(size_t i = 0; i < numAreas; i++)
    {
        if(replySize == (MAX_REPLY_SIZE - 1))
            break;

        area = nvm_getDesc(i);
        struct memInfo infoBlock;
        infoBlock.size  = area->dev->size;
        infoBlock.flags = 0;
        strncpy(infoBlock.name, area->name, sizeof(infoBlock.name));

        memcpy(&reply[replySize], &infoBlock, sizeof(infoBlock));

        // Increment the total length of the reply
        replySize += sizeof(struct memInfo);
    }

    return replySize;
}

static size_t cmd_dumpRestore(const uint8_t cmd, const uint8_t *args,
                              const uint8_t nArg, uint8_t *reply)
{
    (void) nArg;

    // Verify memory index
    uint8_t area_index = args[1];
    const struct nvmDescriptor *area = nvm_getDesc(area_index);


    // Prepare the response frame
    size_t replySize = 3;
    reply[1]         = 0x00;    // Status = OK
    reply[2]         = 0x00;    // Empty response, no parameters

    if(area == NULL)
    {
        reply[1] = EINVAL;
        return replySize;
    }

    int ret;
    if(cmd == FMP_FRAME_DUMP)
        ret = dat_readNvmArea(area);
    else
        ret = dat_writeNvmArea(area);

    if(ret < 0)
        reply[1] = -ret;

    return replySize;
}

static size_t fmpProtocolHandler(const uint8_t *rxData, size_t rxLen,
                                 uint8_t *txData, size_t txMaxLen)
{
    (void) txMaxLen;
    size_t rLen = 0;

    /*
     * FMP request structure:
     *
     * - byte 0: command
     * - byte 1: number of arguments
     * - byte 2..N: command arguments
     */
    const uint8_t cmd   = rxData[0];
    const uint8_t nArgs = rxData[1];
    const uint8_t *args = rxData + 2;

    // A command frame must contain at least two bytes
    if(rxLen < 2)
    {
        txData[0] = cmd;
        txData[1] = EPROTO;

        return 2;
    }

    // Setup standard reply, content will be overridden by command handlers
    txData[0] = cmd;
    txData[1] = EPERM;
    rLen      = 2;

    // Handle the incoming command
    switch(cmd)
    {
        case FMP_FRAME_MEMINFO:
            rLen = cmd_meminfo(args, nArgs, txData);
            break;

        case FMP_FRAME_DUMP:
        case FMP_FRAME_FLASH:
            // Raw memory dump/flash is available only in data transfer mode
            if(state.devStatus != DATATRANSFER)
                break;

            rLen = cmd_dumpRestore(cmd, args, nArgs, txData);
            break;

        default:
            break;
    }

    return rLen;
}


void fmp_init()
{
    rtxlink_setProtocolHandler(RTXLINK_FRAME_FMP, fmpProtocolHandler);
}

void fmp_terminate()
{
    rtxlink_removeProtocolHandler(RTXLINK_FRAME_FMP);
}
