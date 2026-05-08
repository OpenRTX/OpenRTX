/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_SMSPACKET_H
#define M17_SMSPACKET_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>

namespace M17
{

/**
 * Format an SMS message into an M17 application-layer packet buffer suitable
 * for passing to PacketFramer::init().
 *
 * Writes [0x05][UTF-8 text][0x00] into the buffer.  The buffer must be at
 * least msgLen + 2 bytes (protocol ID + text + NUL).
 *
 * @param message:    Pointer to the NUL-terminated SMS text.
 * @param msgLen:     Length of the message NOT counting the NUL terminator.
 * @param buffer:     Output buffer to receive the application-layer data.
 * @param bufferSize: Size of the output buffer in bytes.
 * @return            Number of application-data bytes written (0 on error).
 *                    Pass this value as len to PacketFramer::init().
 */
size_t sms_format_packet(const char *message, size_t msgLen, uint8_t *buffer,
                         size_t bufferSize);

/**
 * Parse an SMS packet payload extracted by PacketDeframer.
 *
 * Checks that the first byte is the M17 SMS protocol ID (0x05) and copies
 * the NUL-terminated message text that follows into the caller-supplied
 * buffer.
 *
 * @param data:    Pointer to the reassembled M17 packet data (from
 *                 PacketDeframer::data()).
 * @param len:     Length of the data in bytes (from PacketDeframer::length()).
 * @param message: Buffer to receive the NUL-terminated message text.
 * @param msgSize: Size of the message buffer in bytes.
 * @return         true if data is a valid SMS packet and extraction succeeded.
 */
bool sms_parse_packet(const uint8_t *data, size_t len, char *message,
                      size_t msgSize);

} // namespace M17

#endif /* M17_SMSPACKET_H */
