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

#include <stdbool.h>
#include <rtxlink.h>
#include <string.h>
#include <errno.h>
#include <slip.h>
#include <crc.h>

static protocolHandler handlers[RTXLINK_NUM_PROTOCOLS] = {NULL};
static uint8_t         rxBuf[1032];
static uint8_t         txBuf[1032];
static size_t          toSend = 0;

static const struct chardev *cDev = NULL;
static struct FrameCtx rxFrame;
static struct FrameCtx txFrame;


void rtxlink_init(const struct chardev *rtxlinkDev)
{
    if(rtxlinkDev == NULL)
        return;

    cDev = rtxlinkDev;
    chardev_init(cDev);
    slip_initFrame(&rxFrame, rxBuf, sizeof(rxBuf));
}

void rtxlink_task()
{
    uint8_t buf[512];
    ssize_t len     = 0;
    ssize_t decoded = 0;

    if(cDev == NULL)
        return;

    // If there is no more data to process, fetch new.
    if(len <= 0)
        len = chardev_read(cDev, buf, sizeof(buf));

    while(decoded < len)
    {
        bool newFrame = false;
        int ret = slip_decode(&rxFrame, buf, (size_t) len, &newFrame);
        if(ret > 0)
        {
            decoded += ret;
        }
        else if((ret == -ENOMEM) && (newFrame == false))
        {
            // No space left in the buffer and we're not in the corner case
            // of an rtxlink frame that "just fits" into the buffer space:
            // drop both decoded and remaining input data as otherwise we
            // may have an incomplete frame.
            len = 0;
            rxFrame.oPos = 0;
            rxFrame.iPos = 0;
        }

        if(newFrame)
        {
            uint8_t *frame;
            size_t  fLen = slip_popFrame(&rxFrame, (void **) &frame);
            uint16_t crc = (frame[fLen - 1] << 8) | (frame[fLen - 2]);

            if(crc == crc_ccitt(frame, fLen - 2))
            {
                uint8_t protocol = frame[0];
                frame += 1;
                fLen  -= 2;

                if(protocol < RTXLINK_NUM_PROTOCOLS)
                {
                    protocolHandler handler = handlers[protocol];
                    if(handler != NULL)
                    {
                        // Prepare response and call the handler function
                        uint8_t *txPtr = &txBuf[1];
                        size_t  txSize = sizeof(txBuf) - 3;
                        txBuf[0]       = protocol;

                        toSend = handler(frame, fLen, txPtr, txSize);

                        // If there is a reply, compute and append CRC
                        if(toSend > 0)
                        {
                            uint16_t crc = crc_ccitt(txBuf, toSend + 1);
                            txPtr[toSend]     = crc >> 8;
                            txPtr[toSend + 1] = crc & 0xff;
                            toSend           += 3;
                        }
                    }
                }
            }
        }
    }

    // If there is data to send, send it in chunks of 64 bytes each to avoid
    // stalling the caller thread for too much time. Sending 64 bytes at 115200
    // baud takes a bit less than 4.5ms.
    if(toSend > 0)
    {
        txFrame.data   = buf;
        txFrame.maxLen = sizeof(buf);
        int ret = slip_encode(&txFrame, txBuf, toSend, true);
        if(ret > 0)
            toSend = 0;

        uint8_t *frame;
        size_t  fLen = slip_popFrame(&txFrame, (void **) &frame);
        chardev_write(cDev, frame, fLen);
    }
}

void rtxlink_terminate()
{
    if(cDev == NULL)
        return;

    chardev_terminate(cDev);
}

int rtxlink_send(const enum ProtocolID proto, const void *data, const size_t len)
{
    if(toSend > 0)
        return -EAGAIN;

    if(data == NULL)
        return -EINVAL;

    if(len > (sizeof(txBuf) - 2))
        return -E2BIG;

    // Set protocol and fill data
    txBuf[0] = proto;
    memcpy(&txBuf[1], data, len);

    uint16_t crc   = crc_ccitt(txBuf, len + 1);
    txBuf[len + 1] = crc >> 8;
    txBuf[len + 2] = crc & 0xff;

    toSend = len + 3;

    return 0;
}

bool rtxlink_setProtocolHandler(const enum ProtocolID proto,
                                protocolHandler handler)
{
    if(proto >= RTXLINK_NUM_PROTOCOLS)
        return false;

    if(handlers[proto] != NULL)
        return false;

    handlers[proto] = handler;
    return true;
}

void rtxlink_removeProtocolHandler(const enum ProtocolID proto)
{
    if(proto >= RTXLINK_NUM_PROTOCOLS)
        return;

    handlers[proto] = NULL;
}
