/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse voice codec: MELPe 2400, 12 bytes (96 bits) per 40ms frame.
 */

#ifndef HORSE_CODEC_H
#define HORSE_CODEC_H

#include "core/audio_path.h"
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

#define HORSE_CODEC_FRAME_BYTES 12

void horse_codec_init(void);
void horse_codec_terminate(void);

bool horse_codec_startEncode(const pathId path);
bool horse_codec_startDecode(const pathId path);
void horse_codec_stop(const pathId path);
bool horse_codec_running(void);

int horse_codec_popFrame(uint8_t *frame, const bool blocking);
int horse_codec_pushFrame(const uint8_t *frame, const bool blocking);

#ifdef __cplusplus
}
#endif

#endif
