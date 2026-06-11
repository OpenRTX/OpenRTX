/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_PROTOCOL_H
#define M17_PROTOCOL_H

#include <stdint.h>
#include <stddef.h>

/** Maximum payload data bytes for an M17 packet (33 frames x 25 bytes). */
#define M17_MAX_PACKET_DATA (33U * 25U)

/**
 * M17 packet buffer with embedded src/dst callsigns.
 *
 * The caller populates src, dst, and payload, then passes a pointer
 * to this struct as pktDesc.buffer to rtx_addPacketTx().  The size
 * field of the descriptor must be 20 + payload_len (the 20 bytes for
 * the src/dst headers plus the number of payload bytes).
 *
 * On RX, the deframer writes directly into the payload region.  The
 * src/dst fields are populated from the M17 LSF by the OpMode layer.
 */
struct m17Packet {
    char src[10]; /**< Source callsign (10 bytes, null-padded) */
    char dst[10]; /**< Destination callsign (10 bytes, null-padded) */
    uint8_t payload[M17_MAX_PACKET_DATA]; /**< Packet payload data */
} __attribute__((packed));

#endif /* M17_PROTOCOL_H */
