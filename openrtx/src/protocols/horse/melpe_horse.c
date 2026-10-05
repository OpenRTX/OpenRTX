/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse voice codec: CODEC2 2400 mode, 12 bytes (96 bits) per 40ms frame.
 */

#include "protocols/horse/melpe_horse.h"
#include <string.h>
#include <stdbool.h>

#if defined(PLATFORM_LINUX)
#include <codec2/codec2.h>
#else
#include "codec2.h"
#endif

#define CODEC2_2400_FRAME_SAMPLES 160
#define CODEC2_2400_FRAME_BYTES   6

static struct CODEC2 *c2_encoder;
static struct CODEC2 *c2_decoder;
static int16_t enc_pending[MELPE_HORSE_SAMPLES_20MS];
static bool enc_have_pending;
static int16_t dec_pending[MELPE_HORSE_SAMPLES_20MS];
static bool dec_have_pending;

void melpe_horse_encoder_init(void)
{
    if (c2_encoder == NULL)
        c2_encoder = codec2_create(CODEC2_MODE_2400);
    enc_have_pending = false;
}

void melpe_horse_encoder_terminate(void)
{
    if (c2_encoder != NULL)
    {
        codec2_destroy(c2_encoder);
        c2_encoder = NULL;
    }
}

void melpe_horse_encode(const int16_t *pcm, size_t n_samples, uint8_t *bits_96)
{
    if (pcm == NULL || bits_96 == NULL || c2_encoder == NULL)
        return;

    if (n_samples < MELPE_HORSE_SAMPLES_40MS)
        return;

    codec2_encode(c2_encoder, bits_96, (short *)(pcm));
    codec2_encode(c2_encoder, bits_96 + CODEC2_2400_FRAME_BYTES,
                  (short *)(pcm + CODEC2_2400_FRAME_SAMPLES));
}

int melpe_horse_encode_20ms(const int16_t *pcm160, uint8_t *bits_96)
{
    int16_t both[MELPE_HORSE_SAMPLES_40MS];

    if (pcm160 == NULL || bits_96 == NULL || c2_encoder == NULL)
        return -1;

    if (!enc_have_pending)
    {
        memcpy(enc_pending, pcm160,
               MELPE_HORSE_SAMPLES_20MS * sizeof(int16_t));
        enc_have_pending = true;
        return 0;
    }

    memcpy(both, enc_pending, MELPE_HORSE_SAMPLES_20MS * sizeof(int16_t));
    memcpy(both + MELPE_HORSE_SAMPLES_20MS, pcm160,
           MELPE_HORSE_SAMPLES_20MS * sizeof(int16_t));
    enc_have_pending = false;
    melpe_horse_encode(both, MELPE_HORSE_SAMPLES_40MS, bits_96);
    return 1;
}

void melpe_horse_decoder_init(void)
{
    if (c2_decoder == NULL)
        c2_decoder = codec2_create(CODEC2_MODE_2400);
    dec_have_pending = false;
}

void melpe_horse_decoder_terminate(void)
{
    if (c2_decoder != NULL)
    {
        codec2_destroy(c2_decoder);
        c2_decoder = NULL;
    }
}

void melpe_horse_decode(const uint8_t *bits_96, int16_t *pcm, size_t *n_samples_out)
{
    if (bits_96 == NULL || pcm == NULL || c2_decoder == NULL)
        return;

    codec2_decode(c2_decoder, pcm, (unsigned char *)bits_96);
    codec2_decode(c2_decoder, pcm + CODEC2_2400_FRAME_SAMPLES,
                  (unsigned char *)(bits_96 + CODEC2_2400_FRAME_BYTES));

    if (n_samples_out != NULL)
        *n_samples_out = MELPE_HORSE_SAMPLES_40MS;
}

int melpe_horse_decode_20ms_start(const uint8_t *bits_96, int16_t *pcm160,
                                  size_t cap)
{
    int16_t full[MELPE_HORSE_SAMPLES_40MS];
    size_t n = 0;

    if (pcm160 == NULL || cap < MELPE_HORSE_SAMPLES_20MS)
        return -1;
    melpe_horse_decode(bits_96, full, &n);
    memcpy(pcm160, full, MELPE_HORSE_SAMPLES_20MS * sizeof(int16_t));
    memcpy(dec_pending, full + MELPE_HORSE_SAMPLES_20MS,
           MELPE_HORSE_SAMPLES_20MS * sizeof(int16_t));
    dec_have_pending = true;
    return 1;
}

int melpe_horse_decode_20ms_next(int16_t *pcm160, size_t cap)
{
    if (pcm160 == NULL || cap < MELPE_HORSE_SAMPLES_20MS || !dec_have_pending)
        return -1;
    memcpy(pcm160, dec_pending, MELPE_HORSE_SAMPLES_20MS * sizeof(int16_t));
    dec_have_pending = false;
    return 1;
}
