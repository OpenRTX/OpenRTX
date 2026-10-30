/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CSK6_CODEC_H
#define CSK6_CODEC_H

#include <interfaces/audio.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * C62 audio through the CSK6 codec and DSP: one 48 kHz stereo stream each way,
 * microphone/speaker on the left and the BK4819 on the right. The device
 * configuration is a bool, true for capture.
 */
extern const struct audioDriver csk6_codec_audio_driver;

/**
 * Configure the codec and start the DSP audio worker.
 * @return zero on success, a negative error code on failure.
 */
int csk6codec_init(void);

/** Cancel streams, join the worker and release DSP audio resources. */
void csk6codec_terminate(void);

/**
 * Enable or disable DSP audio for a board-selected route.
 * The caller handles GPIOs and radio AF muxing and ensures route compatibility.
 *
 * @param source: audio source.
 * @param sink: audio destination.
 * @param enabled: true to enable, false to disable and cancel its stream.
 */
void csk6codec_setRoute(enum AudioSource source, enum AudioSink sink,
                        bool enabled);

#ifdef __cplusplus
}
#endif

#endif /* CSK6_CODEC_H */
