/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef EVENT_H
#define EVENT_H

/**
 * This enum describes the event message type:
 * - EVENT_KBD is used to send a keypress
 * - EVENT_STATUS is used to send a status change notification
 */
enum eventType_t
{
    EVENT_KBD    = 0,
    EVENT_STATUS = 1
};

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
}event_t;

#endif /* EVENT_H */
