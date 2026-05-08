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
 * Locate the message text inside an SMS packet payload, without copying it.
 *
 * Checks that the first byte is the M17 SMS protocol ID (0x05) and returns a
 * pointer to the text that follows, stripping the trailing NUL if present.
 * The text is not NUL-terminated in place: use textLen.
 *
 * @param data:    Pointer to the reassembled M17 packet data (from
 *                 PacketDeframer::data()).
 * @param len:     Length of the data in bytes (from PacketDeframer::length()).
 * @param textLen: Receives the length of the message text in bytes.
 * @return         Pointer to the message text inside data, or nullptr if data
 *                 is not a valid SMS packet.
 */
const char *sms_packet_text(const uint8_t *data, size_t len, size_t *textLen);

} // namespace M17

#endif /* M17_SMSPACKET_H */
