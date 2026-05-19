/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef XMODEM_H
#define XMODEM_H

#include <sys/types.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Send an XMODEM packet over the serial port.
 *
 * @param data: pointer to payload data.
 * @param size: data size, must be either 128 or 1024 byte.
 * @param blockNum: packet sequence number.
 */
void xmodem_sendPacket(const void *data, size_t size, uint8_t blockNum);

/**
 * Receive an XMODEM packet from the serial port.
 *
 * @param data: pointer to a buffer for payload data.
 * @param expectedBlockNum: expected block number, for sanity check.
 * @return number of bytes received or zero in case of errors.
 */
size_t xmodem_receivePacket(void *data, uint8_t expectedBlockNum);

/**
 * Send data using the XMODEM protocol, blocking function.
 * Data transfer begins when the start command from the receiving endpoint is
 * detected.
 *
 * @param size: data size.
 * @param callback: pointer to a callback function in charge of providing data
 * for the new packets being sent.
 * @return number of bytes sent.
 */
ssize_t xmodem_sendData(size_t size, int (*callback)(uint8_t *, size_t));

/**
 * Receive data using the XMODEM protocol, blocking function.
 * Transfer starts immediately when this function is called.
 *
 * @param size: expected data size, in bytes.
 * @param callback: callback function invoked when a new data block is recevied.
 * @return number of bytes received.
 */
ssize_t xmodem_receiveData(size_t size, void (*callback)(uint8_t *, size_t));

#ifdef __cplusplus
}
#endif

#endif /* XMODEM_H */
