/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/usb_vcom.h"
#include <stdbool.h>
#include <stdlib.h>
#include "core/xmodem.h"
#include <string.h>
#include "core/crc.h"

#define SOH     (0x01)  // start of 128-byte data packet
#define STX     (0x02)  // start of 1024-byte data packet
#define EOT     (0x04)  // End Of Transmission
#define ACK     (0x06)  // ACKnowledge, receive OK
#define NAK     (0x15)  // Negative ACKnowledge, receiver ERROR, retry
#define CAN     (0x18)  // two CAN in succession will abort transfer
#define CRC     (0x43)  // 'C' == 0x43, request 16-bit CRC, use in place of first NAK for CRC mode
#define ABT1    (0x41)  // 'A' == 0x41, assume try abort by user typing
#define ABT2    (0x61)  // 'a' == 0x61, assume try abort by user typing

/**
 * @internal
 * Collect a given amount of data from serial port.
 *
 * @param ptr: pointer to destination buffer.
 * @param size: number of bytes to be retrieved.
 */
static void waitForData(uint8_t *ptr, size_t size)
{
    size_t curSize = 0;

    while(curSize < size)
    {
        ssize_t recvd = vcom_readBlock(ptr + curSize, size - curSize);
        if(recvd >= 0) curSize += recvd;
    }
}


void xmodem_sendPacket(const void *data, size_t size, uint8_t blockNum)
{
    // Bad payload size, null block number or null data pointer: do not send
    if(((size != 128) && (size != 1024)) || (data == NULL))
    {
        return;
    }

    uint8_t buf[3] = {SOH, 0x00, 0x00};

    // Override header to STX for 1kB packets
    if(size > 128)
    {
        buf[0] = STX;
    }

    // Sequence number
    buf[1] = blockNum;
    buf[2] = blockNum ^ 0xFF;

    uint16_t crc = crc_ccitt(data, size);

    // Send header, then data and finally CRC
    vcom_writeBlock(buf, 3);
    vcom_writeBlock(data, size);

    buf[0] = crc >> 8;
    buf[1] = crc & 0xFF;
    vcom_writeBlock(buf, 2);
}

size_t xmodem_receivePacket(void* data, uint8_t expectedBlockNum)
{
    // Get first byte
    uint8_t status = 0;
    while((status != STX) && (status != SOH))
    {
        waitForData(&status, 1);
    }

    // Get sequence number
    uint8_t seq[2] = {0};
    waitForData(seq, 2);

    // Determine payload size and get data
    size_t blockSize = 128;
    if(status == STX) blockSize = 1024;
    waitForData(((uint8_t *) data), blockSize);

    // Get CRC
    uint8_t crc[2] = {0};
    waitForData(crc, 2);

    // First sanity check: sequence number
    if((seq[0] ^ seq[1]) != 0xFF)  return 0;
    if(expectedBlockNum != seq[0]) return 0;

    // Second sanity check: CRC
    uint16_t dataCrc = crc_ccitt(data, blockSize);
    if((crc[0] != (dataCrc >> 8)) || (crc[1] != (dataCrc & 0xFF))) return 0;

    return blockSize;
}

ssize_t xmodem_sendData(size_t size, int (*callback)(uint8_t *, size_t))
{
    // Wait for the start command from the receiver, only CRC mode is supported.
    uint8_t cmd = 0;
    while(cmd != CRC)
    {
        waitForData(&cmd, 1);
    }

    // Send data.
    uint8_t dataBuf[1024];
    uint8_t blockNum = 1;
    size_t  sentSize = 0;

    while(sentSize < size)
    {
        size_t remaining = size - sentSize;
        size_t blockSize = 1024;
        if(remaining < blockSize) blockSize = remaining;

        // Request data, stop transfer on failure
        if(callback(dataBuf, blockSize) < 0)
        {
            cmd = CAN;
            vcom_writeBlock(&cmd, 1);
            return -1;
        }

        // Pad data to 128 or 1024 bytes, if necessary
        size_t padSize = 0;
        if(blockSize < 128)
        {
            padSize = 128 - blockSize;
        }
        else if(blockSize < 1024)
        {
            padSize = 1024 - blockSize;
        }

        uint8_t *ptr = dataBuf + padSize + 1;
        memset(ptr, 0x1A, padSize);

        // Send packet and wait for ACK, resend on NACK.
        bool ok = false;
        do
        {
            blockSize += padSize;
            xmodem_sendPacket(dataBuf, blockSize, blockNum);

            cmd = 0;
            while((cmd != ACK) && (cmd != NAK))
            {
                waitForData(&cmd, 1);
                if(cmd == ACK) ok = true;
            }
        }
        while(ok == false);

        sentSize += blockSize - padSize;
        blockNum++;
    }

    // End of transfer
    cmd = EOT;
    vcom_writeBlock(&cmd, 1);
    while(cmd != ACK)
    {
        waitForData(&cmd, 1);
    }

    return sentSize;
}

ssize_t xmodem_receiveData(size_t size, void (*callback)(uint8_t *, size_t))
{
    uint8_t dataBuf[1024];
    uint8_t command  = 0;
    uint8_t blockNum = 1;
    size_t  rcvdSize = 0;

    // Request data transfer in CRC mode
    command = CRC;
    vcom_writeBlock(&command, 1);

    while(rcvdSize < size)
    {
        size_t blockSize = xmodem_receivePacket(dataBuf, blockNum);
        if(blockSize == 0)
        {
            // Bad packet, send NACK
            command = NAK;
        }
        else
        {
            // New data arrived
            size_t delta = size - rcvdSize;
            if(blockSize < delta) delta = blockSize;
            callback(dataBuf, delta);

            rcvdSize += delta;
            blockNum++;

            // ACK and go on
            command = ACK;
        }

        vcom_writeBlock(&command, 1);
    }

    // Wait for EOT from the sender, ACK and return
    uint8_t status = 0;
    while(status != EOT)
    {
        waitForData(&status, 1);
    }

    command = ACK;
    vcom_writeBlock(&command, 1);

    return rcvdSize;
}
