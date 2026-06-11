/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_PROTOCOL_H
#define M17_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/**
 * Maximum payload data bytes for an M17 packet (33 frames x 25 bytes).
 */
#define M17_MAX_PACKET_DATA (33U * 25U)

/**
 * Size of the src/dst callsigns in front of an m17Packet payload.
 */
#define M17_PKT_HEADER offsetof(struct m17Packet, payload)

/**
 * M17 packet buffer with embedded src/dst callsigns.
 *
 * The caller populates src, dst, and payload, then passes a pointer
 * to this struct as pktDesc.buffer to rtx_addPacketTx().  The size
 * field of the descriptor must be M17_PKT_HEADER + payload_len (the
 * src/dst headers plus the number of payload bytes).
 *
 * On RX, the deframer writes directly into the payload region.  The
 * src/dst fields are populated from the M17 LSF by the OpMode layer.
 */
struct m17Packet {
    char src[10];
    char dst[10];
    uint8_t payload[M17_MAX_PACKET_DATA];
} __attribute__((packed));

#endif /* M17_PROTOCOL_H */
