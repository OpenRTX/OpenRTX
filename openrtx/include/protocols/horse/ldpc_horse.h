/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * LDPC rate 1/2 for Horse. Voice: 184 bits -> 368 bits (46 bytes).
 * Placeholder: repeat-2 code (each bit sent twice). Full LDPC matrix can be added later.
 */

#ifndef LDPC_HORSE_H
#define LDPC_HORSE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LDPC_VOICE_PAYLOAD_BITS  184
#define LDPC_VOICE_ENCODED_BITS  368
#define LDPC_VOICE_PAYLOAD_BYTES 23
#define LDPC_VOICE_ENCODED_BYTES 46

void ldpc_horse_encode_voice(const uint8_t *payload, uint8_t *encoded);
void ldpc_horse_decode_voice(const uint8_t *encoded, uint8_t *payload);

#ifdef __cplusplus
}
#endif

#endif
