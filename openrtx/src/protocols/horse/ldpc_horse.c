/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * LDPC rate 1/2 placeholder: repeat-2 (each bit sent twice).
 * Replace with full LDPC matrix when available.
 */

#include "protocols/horse/ldpc_horse.h"
#include <string.h>

static void bits_to_bytes(const uint8_t *bits, size_t n_bits, uint8_t *bytes)
{
    size_t n_bytes = (n_bits + 7) / 8;
    memset(bytes, 0, n_bytes);
    for (size_t i = 0; i < n_bits; i++)
        if (bits[i])
            bytes[i / 8] |= 0x80 >> (i % 8);
}

static void bytes_to_bits(const uint8_t *bytes, size_t n_bits, uint8_t *bits)
{
    for (size_t i = 0; i < n_bits; i++)
        bits[i] = (bytes[i / 8] & (0x80 >> (i % 8))) ? 1 : 0;
}

void ldpc_horse_encode_voice(const uint8_t *payload, uint8_t *encoded)
{
    uint8_t in_bits[LDPC_VOICE_PAYLOAD_BITS];
    uint8_t out_bits[LDPC_VOICE_ENCODED_BITS];
    bytes_to_bits(payload, LDPC_VOICE_PAYLOAD_BITS, in_bits);
    for (size_t i = 0; i < LDPC_VOICE_PAYLOAD_BITS; i++)
    {
        out_bits[2 * i]     = in_bits[i];
        out_bits[2 * i + 1] = in_bits[i];
    }
    bits_to_bytes(out_bits, LDPC_VOICE_ENCODED_BITS, encoded);
}

void ldpc_horse_decode_voice(const uint8_t *encoded, uint8_t *payload)
{
    uint8_t in_bits[LDPC_VOICE_ENCODED_BITS];
    uint8_t out_bits[LDPC_VOICE_PAYLOAD_BITS];
    bytes_to_bits(encoded, LDPC_VOICE_ENCODED_BITS, in_bits);
    for (size_t i = 0; i < LDPC_VOICE_PAYLOAD_BITS; i++)
        out_bits[i] = (in_bits[2 * i] & in_bits[2 * i + 1]) ? 1 : 0;
    bits_to_bytes(out_bits, LDPC_VOICE_PAYLOAD_BITS, payload);
}
