/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SMSQUEUE_H
#define SMSQUEUE_H

#include <cstdint>
#include <cstddef>

/**
 * Fixed-capacity queue for received SMS messages.
 *
 * Each entry stores a sender callsign and message text in fixed-size
 * buffers — no heap allocation is performed.  When the queue reaches
 * MAX_MESSAGES the oldest entry is automatically evicted on the next push.
 */
class SMSQueue
{
public:
    static constexpr uint8_t MAX_MESSAGES = 10;
    static constexpr uint8_t CALLSIGN_LEN = 10;
    static constexpr uint16_t SMS_MAX_LEN = 822;

    SMSQueue();

    /**
     * Remove all messages and reset count.
     */
    void clear();

    /**
     * Push a new message.  If the queue is already at capacity the oldest
     * entry is evicted first.  Sender and message strings are truncated to
     * fit the fixed internal buffers.
     *
     * @param sender:  null-terminated sender callsign.
     * @param message: null-terminated message text.
     */
    void push(const char *sender, const char *message);

    /**
     * Retrieve a message by index.
     *
     * @param index:       zero-based position in the queue.
     * @param sender:      buffer to receive the sender callsign.
     * @param sender_len:  size of the sender buffer in bytes.
     * @param message:     buffer to receive the message text.
     * @param message_len: size of the message buffer in bytes.
     * @return true if the index was valid and buffers were filled.
     */
    bool get(uint8_t index, char *sender, size_t sender_len, char *message,
             size_t message_len) const;

    /**
     * Delete a message by index.  Entries above it shift down.
     *
     * @param index: zero-based position to remove.
     */
    void erase(uint8_t index);

    /**
     * @return current number of messages in the queue.
     */
    uint8_t count() const
    {
        return msgCount;
    }

private:
    struct Entry {
        char sender[CALLSIGN_LEN];
        char message[SMS_MAX_LEN];
    };

    uint8_t msgCount;
    Entry entries[MAX_MESSAGES];
};

#endif /* SMSQUEUE_H */
