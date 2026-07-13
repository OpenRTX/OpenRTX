/*
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Thin wrapper around the device-resident software AMBE+2 voice codec (carved
 * from the user's firmware, copied to its native VMA at runtime). For the DMR
 * voice path; entry-point ABI documented in scripts/labels/ambe_codec.py.
 *
 * Built only when HD2_DMR_VOICE is defined (needs build-ambe/, gitignored).
 */
#ifndef HD2_AMBE_H
#define HD2_AMBE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Copy the carved codec blob from flash .rodata to its native SRAM VMAs
 * (0x1bfe0, 0x3227c). Idempotent. Returns bytes copied, 0 if the blob is
 * absent/empty. Must run once before any other call here. */
int hd2_ambe_load(void);

/* Allocate + initialise one encoder state (ambe_codec_init). Caller owns the
 * returned pointer (free with hd2_ambe_free). Returns NULL on OOM. */
void *hd2_ambe_enc_alloc(void);

void hd2_ambe_free(void *state);

/* Encode one 20 ms frame: 160 s16 PCM @ 8 kHz. PCM is staged into the state
 * struct (encode reads it there). The 49-bit param vector lands in the struct;
 * `out` receives the first 9 raw param bytes. Returns 9, or <0 on error. */
int hd2_ambe_encode_frame(void *enc, const int16_t *pcm160, uint8_t out[9]);

/* Encode one 80-sample half-block (half = 0 or 1): stages pcm80 into the state
 * struct at +0x1898 and invokes the encoder. Output stays in the struct. */
int hd2_ambe_encode_half(void *enc, const int16_t *pcm80, int half);

/* Fill pcm160 with a 250 Hz test tone (a known, non-trivial input for the
 * bring-up self-test). */
void hd2_ambe_test_tone(int16_t *pcm160);

#ifdef __cplusplus
}
#endif

#endif /* HD2_AMBE_H */
