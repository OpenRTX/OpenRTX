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

#ifndef RTXLINK_H
#define RTXLINK_H

#include <interfaces/chardev.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Identifiers for each sub-protocol of rtxlink
 */
enum ProtocolID
{
    RTXLINK_FRAME_STDIO  = 0x00,
    RTXLINK_FRAME_CAT    = 0x01,
    RTXLINK_FRAME_FMP    = 0x02,
    RTXLINK_FRAME_XMODEM = 0x03,

    RTXLINK_NUM_PROTOCOLS
};

/**
 * Protocol handler function pointer.
 * A protocol handler function should have as input parameters:
 * - a const void * for the packet payload;
 * - a size_t for the payload size;
 * - a void * pointing to a buffer where to write an eventual reply;
 * - a size_t specifying the maximum size of the reply, in bytes.
 * And it should return a size_t indicating the number of bytes of the reply.
 */
typedef size_t (*protocolHandler)(const uint8_t *, size_t, uint8_t *, size_t);


/**
 * Initialize the rtxlink management module.
 *
 * @param rtxlinkDev: pointer to the character device used for rtxlink
 * communication.
 */
void rtxlink_init(const struct chardev *rtxlinkDev);

/**
 * Periodic rtxlink update task.
 */
void rtxlink_task();

/**
 * Shut down the rtxlink managment module.
 */
void rtxlink_terminate();

/**
 * Send a block of data over rtxlink.
 *
 * @param proto: protocol ID.
 * @param data: pointer to payload data;
 * @param len: payload length in bytes.
 * @return true on success, false if a transmission is alredy ongoing.
 */
int rtxlink_send(const enum ProtocolID proto, const void *data, const size_t len);

/**
 * Register a protocol handler callback.
 *
 * @param proto: protocol ID.
 * @param handler: callback function;
 * @return true on success, false if a callback is already registered.
 */
bool rtxlink_setProtocolHandler(const enum ProtocolID proto,
                                protocolHandler handler);

/**
 * Remove an registered protocol handler callback.
 *
 * @param proto: protocol ID.
 */
void rtxlink_removeProtocolHandler(const enum ProtocolID proto);

#ifdef __cplusplus
}
#endif

#endif
