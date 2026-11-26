/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDIO_C62_H
#define AUDIO_C62_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Sound a beep on the speaker, mixed with whatever plays there, or stop it.
 *
 * @param freq: tone frequency in Hz, 0 to stop the beep.
 */
void audioC62_beep(uint16_t freq);

#ifdef __cplusplus
}
#endif

#endif // AUDIO_C62_H
