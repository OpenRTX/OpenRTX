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
 * - EVENT_KBD is used to send a keypress
 * - EVENT_STATUS is used to send a status change notification
 */
typedef enum
{
    EVENT_NONE   = 0 ,
    EVENT_KBD    = 1 ,
    EVENT_STATUS = 2 ,
    EVENT_RTX    = 3
}EventType_en;

typedef enum
{
    EVENT_STATUS_TIME_TICK         = ( 1 << 0 ) ,
    EVENT_STATUS_DISPLAY_TIME_TICK = ( 1 << 1 ) ,
    EVENT_STATUS_BATTERY           = ( 1 << 2 ) ,
    EVENT_STATUS_RSSI              = ( 1 << 3 ) ,
    EVENT_STATUS_ALL               = 0x0F
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
