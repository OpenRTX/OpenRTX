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
 * The event type field is 1 bit wide, see event_t.
 */
enum eventType_t
{
    EVENT_KBD    = 0,
    EVENT_STATUS = 1
};

/**
 * The event message is constrained to 32 bits size, so that it can be
 * read and written in a single access on 32-bit MCUs.
 * One bit is enough to encode the event type, leaving 31 bits of payload
 * for a whole keyboard event (see kbd_msg_t).
 */
typedef union
{
    struct
    {
        uint32_t type    : 1,
                 payload : 31;
    };

    uint32_t value;
}event_t;

#endif /* EVENT_H */
