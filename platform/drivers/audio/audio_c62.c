/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <hwconfig.h>
#include <interfaces/radio.h>
#include "csk6_codec.h"

/*
 * Board layer: radio AF output and speaker GPIO, then the codec driver
 * connects the sources and sinks. This lock serializes route changes; the
 * codec worker never takes it, so route changes can wait for the worker.
 * audio_init() and audio_terminate() are called once each, so they don't
 * take it.
 */
static K_MUTEX_DEFINE(boardMutex);
static bool initialized;
/* Active routes, by source and sink */
static bool paths[SOURCE_MCU + 1][SINK_MCU + 1];

/*
 * Both device tables use one driver: the config selects capture or playback,
 * and the instance is the codec channel, the same number as the endpoint.
 */
const struct audioDevice outputDevices[] = {
    { .driver = NULL, .config = NULL, .instance = 0, .endpoint = SINK_MCU },
    {
        .driver = &csk6_codec_audio_driver,
        .config = (const void *)CSK6_CODEC_MODE_OUTPUT,
        .instance = SINK_RTX,
        .endpoint = SINK_RTX,
    },
    {
        .driver = &csk6_codec_audio_driver,
        .config = (const void *)CSK6_CODEC_MODE_OUTPUT,
        .instance = SINK_SPK,
        .endpoint = SINK_SPK,
    },
};

const struct audioDevice inputDevices[] = {
    { .driver = NULL, .config = NULL, .instance = 0, .endpoint = SOURCE_MCU },
    {
        .driver = &csk6_codec_audio_driver,
        .config = (const void *)CSK6_CODEC_MODE_INPUT,
        .instance = SOURCE_RTX,
        .endpoint = SOURCE_RTX,
    },
    {
        .driver = &csk6_codec_audio_driver,
        .config = (const void *)CSK6_CODEC_MODE_INPUT,
        .instance = SOURCE_MIC,
        .endpoint = SOURCE_MIC,
    },
};

/**
 * Keep the speaker enabled while any active route plays to it.
 */
static void updateSpeaker(void)
{
    bool speaker = false;
    for (size_t i = 0; i < ARRAY_SIZE(paths); i++)
        speaker |= paths[i][SINK_SPK];
#ifndef C62_ENABLE_SWD
    gpio_pin_set_dt(&speaker_enable, speaker);
#endif
}

void audio_init(void)
{
#ifndef C62_ENABLE_SWD
    gpio_pin_configure_dt(&speaker_enable, GPIO_OUTPUT_INACTIVE);
#endif
    gpio_pin_configure_dt(&dtmf_enable, GPIO_OUTPUT_INACTIVE);
    initialized = csk6codec_init() == 0;
}

void audio_terminate(void)
{
    if (!initialized)
        return;

#ifndef C62_ENABLE_SWD
    gpio_pin_set_dt(&speaker_enable, 0);
#endif
    radio_disableAfOutput();
    csk6codec_terminate();
    initialized = false;
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    if (!initialized)
        return;

    k_mutex_lock(&boardMutex, K_FOREVER);
    if (!paths[source][sink]) {
        paths[source][sink] = true;
        csk6codec_setRoute(source, sink, true);
        if ((source == SOURCE_RTX) && (sink == SINK_SPK))
            radio_enableAfOutput();
        updateSpeaker();
    }
    k_mutex_unlock(&boardMutex);
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    if (!initialized)
        return;

    k_mutex_lock(&boardMutex, K_FOREVER);
    if (paths[source][sink]) {
        paths[source][sink] = false;
        csk6codec_setRoute(source, sink, false);
        updateSpeaker();
        if ((source == SOURCE_RTX) && (sink == SINK_SPK))
            radio_disableAfOutput();
    }
    k_mutex_unlock(&boardMutex);
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink p2Sink)
{
    /* Physical sources and sinks are exclusive. MCU is a memory endpoint,
     * so separate capture and playback streams can share it. */
    return ((p1Source != p2Source) || (p1Source == SOURCE_MCU))
        && ((p1Sink != p2Sink) || (p1Sink == SINK_MCU));
}
