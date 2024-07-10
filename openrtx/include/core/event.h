/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#ifndef EVENT_H
#define EVENT_H

/**
 * This enum describes the event message type:
 * - EVENT_TYPE_KBD is used to send a keypress
 * - EVENT_TYPE_STATUS is used to send a status change notification
 */
typedef enum
{
    EVENT_TYPE_NONE    = 0x00 ,
    EVENT_TYPE_KBD     = 0x01 ,
    EVENT_TYPE_STATUS  = 0x02 ,
    EVENT_TYPE_RTX     = 0x03 ,
    EVENT_TYPE_SHIFT   =   30
}EventType_en;

#define EVENT_TYPE_MASK    0xC0000000
#define EVENT_PAYLOAD_MASK 0x3FFFFFFF

typedef enum
{
    EVENT_STATUS_ALL_KEYS          = 0x3FFFFFFF ,
    EVENT_STATUS_TIME_TICK         = ( 1 << 0 ) ,
    EVENT_STATUS_DISPLAY_TIME_TICK = ( 1 << 1 ) ,
    EVENT_STATUS_DEVICE_TIME_TICK  = ( 1 << 2 ) ,
    EVENT_STATUS_BATTERY           = ( 1 << 3 ) ,
    EVENT_STATUS_RSSI              = ( 1 << 4 ) ,
    EVENT_STATUS_ALL               = 0x1F       ,
    EVENT_STATUS_DEVICE            = 0x1E
}EventStatus_en;

/**
 * The event message is constrained to 32 bits size
 * This is necessary to send event messages in uC OS/III Queues
 * That accept a void * type message, which is 32-bits wide on
 * ARM cortex-M MCUs
 * uC OS/III Queues are meant to send pointer to allocated data
 * But if we keep the size under 32 bits, we can sent the
 * entire message, casting it to a void * pointer.
 */
typedef union
{
    struct
    {
        uint32_t type    : 2,
                 payload : 30;
    };

    uint32_t value;
}Event_st;

#endif /* EVENT_H */
