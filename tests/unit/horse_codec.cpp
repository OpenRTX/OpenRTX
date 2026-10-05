/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/melpe_horse.h"
#include <cstdio>
#include <cstring>
#include <cstdint>

int main()
{
    int16_t blk[MELPE_HORSE_SAMPLES_20MS];
    uint8_t frame[MELPE_HORSE_BYTES];
    uint8_t sent[MELPE_HORSE_BYTES];
    int16_t out[MELPE_HORSE_SAMPLES_20MS + 8];
    size_t i;
    int r;

    melpe_horse_encoder_init();
    melpe_horse_decoder_init();

    for (i = 0; i < MELPE_HORSE_SAMPLES_20MS; i++)
        blk[i] = (int16_t)((i * 37) & 0x7FFF);

    memset(frame, 0xA5, sizeof frame);
    r = melpe_horse_encode_20ms(blk, frame);
    if (r != 0)
    {
        std::printf("horse_codec_test: first 20 ms emitted a frame\n");
        return -1;
    }
    for (i = 0; i < sizeof frame; i++)
    {
        if (frame[i] != 0xA5)
        {
            std::printf("horse_codec_test: first 20 ms wrote the frame buffer\n");
            return -1;
        }
    }

    r = melpe_horse_encode_20ms(blk, frame);
    if (r != 1)
    {
        std::printf("horse_codec_test: second 20 ms did not emit a frame\n");
        return -1;
    }
    memcpy(sent, frame, sizeof frame);
    {
        bool all_zero = true;
        for (i = 0; i < sizeof frame; i++)
        {
            if (frame[i] != 0)
                all_zero = false;
        }
        if (all_zero)
        {
            std::printf("horse_codec_test: encode emitted a zero frame\n");
            return -1;
        }
    }

    memset(out, 0x5A, sizeof out);
    if (melpe_horse_decode_20ms_start(sent, out, MELPE_HORSE_SAMPLES_20MS) != 1)
        return -1;
    {
        uint8_t *p = (uint8_t *)(out + MELPE_HORSE_SAMPLES_20MS);
        for (i = 0; i < 8 * sizeof(int16_t); i++)
        {
            if (p[i] != 0x5A)
            {
                std::printf("horse_codec_test: decode overwrote past 20 ms\n");
                return -1;
            }
        }
    }

    memset(out, 0x5A, sizeof out);
    if (melpe_horse_decode_20ms_next(out, MELPE_HORSE_SAMPLES_20MS) != 1)
        return -1;
    {
        uint8_t *p = (uint8_t *)(out + MELPE_HORSE_SAMPLES_20MS);
        for (i = 0; i < 8 * sizeof(int16_t); i++)
        {
            if (p[i] != 0x5A)
            {
                std::printf("horse_codec_test: second decode overwrote past 20 ms\n");
                return -1;
            }
        }
    }

    if (melpe_horse_decode_20ms_start(sent, out, 159) != -1)
        return -1;

    melpe_horse_encoder_terminate();
    melpe_horse_decoder_terminate();
    std::printf("horse_codec_test: passed\n");
    return 0;
}
