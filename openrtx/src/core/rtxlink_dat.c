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

#include <interfaces/nvmem.h>
#include <nvmem_access.h>
#include <rtxlink_dat.h>
#include <rtxlink.h>
#include <errno.h>

#define DAT_PAYLOAD_SIZE 1024

#define ACK   (0x06)  // ACKnowledge, receive OK
#define NAK   (0x15)  // Negative ACKnowledge, receiver ERROR, retry

static enum DatStatus              status = RTXLINK_DAT_IDLE;
static const struct nvmDescriptor *memArea;
static uint8_t                     blockCnt;
static size_t                      curAddr;
static size_t                      readSize;

static size_t datReadHandler(const uint8_t *rxData, size_t rxLen, uint8_t *txData,
                             size_t txMaxLen)
{
    (void) rxLen;

    if(rxData[0] == ACK)
    {
        switch(status)
        {
            case RTXLINK_DAT_START_READ:
                status = RTXLINK_DAT_READ;
                break;

            case RTXLINK_DAT_READ:
                blockCnt += 1;
                curAddr  += readSize;
                break;

            default:
                return 0;
                break;
        }

        // Previous read ok, update the size for the next read
        readSize = memArea->dev->size - curAddr;
        if(readSize == 0)
        {
            status = RTXLINK_DAT_IDLE;
            return 0;
        }

        size_t maxSize = (txMaxLen > DAT_PAYLOAD_SIZE) ? DAT_PAYLOAD_SIZE : txMaxLen;
        if(readSize > maxSize)
            readSize = maxSize;
    }

    // Read from memory and send
    txData[0] = blockCnt;
    txData[1] = blockCnt ^ 0xFF;
    nvm_devRead(memArea->dev, curAddr, &txData[2], readSize);

    return readSize + 2;
}

static size_t datWriteHandler(const uint8_t *rxData, size_t rxLen, uint8_t *txData,
                              size_t txMaxLen)
{
    (void) txMaxLen;

    // Set up a NACK response, overridden to ACK if everything goes well
    txData[0] = NAK;

    // Check sequence numbers
    uint8_t bNum  = rxData[0];
    uint8_t ibNum = rxData[1];

    if(((bNum ^ ibNum) != 0xFF) || (bNum != blockCnt))
        return 1;

    int ret = nvm_devWrite(memArea->dev, curAddr, &rxData[2], rxLen - 2);
    if(ret < 0)
        return 1;

    // Success: prepare next block number, update address and send ACK
    blockCnt += 1;
    curAddr  += (rxLen - 2);
    txData[0] = ACK;

    return 1;
}


int dat_readNvmArea(const struct nvmDescriptor *area)
{
    if(status != RTXLINK_DAT_IDLE)
        return -EBUSY;

    status   = RTXLINK_DAT_START_READ;
    memArea  = area;
    curAddr  = 0;
    blockCnt = 0;

    rtxlink_setProtocolHandler(RTXLINK_FRAME_DAT, datReadHandler);
    return 0;
}

int dat_writeNvmArea(const struct nvmDescriptor *area)
{
    if(status != RTXLINK_DAT_IDLE)
        return -EBUSY;

    status = RTXLINK_DAT_WRITE;
    memArea  = area;
    curAddr  = 0;
    blockCnt = 0;

    rtxlink_setProtocolHandler(RTXLINK_FRAME_DAT, datWriteHandler);

    uint8_t ready = ACK;
    rtxlink_send(RTXLINK_FRAME_DAT, &ready, 1);

    return 0;
}

enum DatStatus dat_getStatus()
{
    return status;
}

void dat_reset()
{
    rtxlink_removeProtocolHandler(RTXLINK_FRAME_DAT);
    status = RTXLINK_DAT_IDLE;
}
