/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef SMSTXPACKET_H
#define SMSTXPACKET_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstddef>
#include <cstdint>

/**
 * Format an SMS message into an application-layer packet buffer suitable for
 * passing to PacketAssembler::init().
 *
 * Writes [0x05][UTF-8 text][0x00] into the buffer.  The buffer must be at
 * least msgLen + 3 bytes (protocol ID + text + NUL + 2 bytes room for CRC
 * that PacketAssembler will append).
 *
 * @param message:    Pointer to the NUL-terminated SMS text.
 * @param msgLen:     Length of the message NOT counting the NUL terminator.
 * @param buffer:     Output buffer to receive the application-layer data.
 * @param bufferSize: Size of the output buffer in bytes.
 * @return            Number of application-data bytes written (0 on error).
 *                    Pass this value as dataLen to PacketAssembler::init().
 */
size_t prepareSmsPacketData(const char *message, size_t msgLen, uint8_t *buffer,
                            size_t bufferSize);

#endif /* SMSTXPACKET_H */
