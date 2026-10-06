/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef MELPE_HORSE_H
#define MELPE_HORSE_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MELPE_HORSE_SAMPLES_20MS 160
#define MELPE_HORSE_SAMPLES_40MS 320
#define MELPE_HORSE_BITS 96
#define MELPE_HORSE_BYTES 12

void melpe_horse_encoder_init(void);
void melpe_horse_encoder_terminate(void);
void melpe_horse_encode(const int16_t *pcm, size_t n_samples, uint8_t *bits_96);
int melpe_horse_encode_20ms(const int16_t *pcm160, uint8_t *bits_96);

void melpe_horse_decoder_init(void);
void melpe_horse_decoder_terminate(void);
void melpe_horse_decode(const uint8_t *bits_96, int16_t *pcm,
                        size_t *n_samples_out);
int melpe_horse_decode_20ms_start(const uint8_t *bits_96, int16_t *pcm160,
                                  size_t cap);
int melpe_horse_decode_20ms_next(int16_t *pcm160, size_t cap);

#ifdef __cplusplus
}
#endif

#endif
