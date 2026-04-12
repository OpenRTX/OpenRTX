/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "rtx/SMSQueue.hpp"
#include <cstring>

SMSQueue::SMSQueue() : msgCount(0)
{
}

void SMSQueue::clear()
{
    msgCount = 0;
}

void SMSQueue::push(const char *sender, const char *message)
{
    // Evict the oldest entry when at capacity.
    if (msgCount == MAX_MESSAGES) {
        for (uint8_t i = 1; i < msgCount; i++)
            entries[i - 1] = entries[i];
        msgCount--;
    }

    strncpy(entries[msgCount].sender, sender, CALLSIGN_LEN - 1);
    entries[msgCount].sender[CALLSIGN_LEN - 1] = '\0';

    strncpy(entries[msgCount].message, message, SMS_MAX_LEN - 1);
    entries[msgCount].message[SMS_MAX_LEN - 1] = '\0';

    msgCount++;
}

bool SMSQueue::get(uint8_t index, char *sender, size_t sender_len,
                   char *message, size_t message_len) const
{
    if (index >= msgCount || sender_len == 0 || message_len == 0)
        return false;

    strncpy(sender, entries[index].sender, sender_len - 1);
    sender[sender_len - 1] = '\0';

    strncpy(message, entries[index].message, message_len - 1);
    message[message_len - 1] = '\0';

    return true;
}

void SMSQueue::erase(uint8_t index)
{
    if (index >= msgCount)
        return;

    for (uint8_t i = index + 1; i < msgCount; i++)
        entries[i - 1] = entries[i];

    msgCount--;
}
