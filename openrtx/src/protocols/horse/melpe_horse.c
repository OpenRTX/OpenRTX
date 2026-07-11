/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse voice codec: CODEC2 2400 mode, 12 bytes (96 bits) per 40ms frame.
 */

#include "protocols/horse/melpe_horse.h"
#include <string.h>

#if defined(PLATFORM_LINUX)
#include <codec2/codec2.h>
#else
#include "codec2.h"
#endif

#define CODEC2_2400_FRAME_SAMPLES 160
#define CODEC2_2400_FRAME_BYTES   6

static struct CODEC2 *c2_encoder;
static struct CODEC2 *c2_decoder;

void melpe_horse_encoder_init(void)
{
    if (c2_encoder == NULL)
        c2_encoder = codec2_create(CODEC2_MODE_2400);
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
    {
        memset(bits_96, 0, MELPE_HORSE_BYTES);
        return;
    }

    codec2_encode(c2_encoder, bits_96, (short *)(pcm));
    codec2_encode(c2_encoder, bits_96 + CODEC2_2400_FRAME_BYTES,
                  (short *)(pcm + CODEC2_2400_FRAME_SAMPLES));
}

void melpe_horse_decoder_init(void)
{
    if (c2_decoder == NULL)
        c2_decoder = codec2_create(CODEC2_MODE_2400);
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
