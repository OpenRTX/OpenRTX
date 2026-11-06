/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef AUDIO_C62_H
#define AUDIO_C62_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Keep the speaker amplifier on for the broadcast FM receiver, whose audio
 * reaches the amplifier directly rather than through the codec.
 *
 * @param enable: true while the FM receiver is playing.
 */
void audio_c62_setBroadcastFm(bool enable);

#ifdef __cplusplus
}
#endif

#endif /* AUDIO_C62_H */
