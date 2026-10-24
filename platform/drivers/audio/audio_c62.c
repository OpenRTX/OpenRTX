/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <string.h>
#include <zephyr/kernel.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/pinctrl.h>
#include <zephyr/drivers/uart.h>
#include <hwconfig.h>
#include <interfaces/radio.h>
#include "csk6_codec.h"

/*
 * Board layer: radio AF output and speaker GPIOs, then the codec driver
 * connects the sources and sinks. This lock serializes initialization and
 * route changes; the codec worker never takes it, so route changes can wait
 * for the worker.
 */
static K_MUTEX_DEFINE(board_mutex);
static bool initialized;
static bool paths[3][3]; /* Active routes, by source and sink */
/* Both device tables use one driver; the config selects capture or playback */
static const bool input_config = true;
static const bool output_config = false;

const struct audioDevice outputDevices[] = {
    { NULL, NULL, 0, SINK_MCU },
    { &csk6_codec_audio_driver, &output_config, SINK_RTX, SINK_RTX },
    { &csk6_codec_audio_driver, &output_config, SINK_SPK, SINK_SPK },
};

const struct audioDevice inputDevices[] = {
    { NULL, NULL, 0, SOURCE_MCU },
    { &csk6_codec_audio_driver, &input_config, SOURCE_RTX, SOURCE_RTX },
    { &csk6_codec_audio_driver, &input_config, SOURCE_MIC, SOURCE_MIC },
};

PINCTRL_DT_DEFINE(DT_CHOSEN(zephyr_console));

/**
 * Re-apply the console pins and UART settings, which the DSP can change
 * while it sets up audio.
 */
static void restore_console(void)
{
    const struct device *console = DEVICE_DT_GET(DT_CHOSEN(zephyr_console));
    const struct uart_config config = {
        .baudrate = DT_PROP(DT_CHOSEN(zephyr_console), current_speed),
        .parity = UART_CFG_PARITY_NONE,
        .stop_bits = UART_CFG_STOP_BITS_1,
        .data_bits = UART_CFG_DATA_BITS_8,
        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE,
    };
    int pins = pinctrl_apply_state(
        PINCTRL_DT_DEV_CONFIG_GET(DT_CHOSEN(zephyr_console)),
        PINCTRL_STATE_DEFAULT);
    int uart = uart_configure(console, &config);
    if (pins != 0 || uart != 0)
        printk("Console configuration failed: pins=%d uart=%d\n", pins, uart);
}

/**
 * Configure the audio GPIOs and start the codec, once. Call with board_mutex
 * held.
 */
static void init_audio(void)
{
    if (initialized)
        return;
    gpio_pin_configure_dt(&speaker_enable, GPIO_OUTPUT_INACTIVE);
    gpio_pin_configure_dt(&dtmf_enable, GPIO_OUTPUT_INACTIVE);
    restore_console();
    initialized = csk6codec_init() == 0;
    restore_console();
}

/**
 * Keep each physical output enabled while any active route needs it.
 */
static void update_mux(void)
{
    bool radio = false;
    bool speaker = false;
    for (unsigned int i = 0; i < 3; i++) {
        radio |= paths[SOURCE_RTX][i];
        speaker |= paths[i][SINK_SPK];
    }
    if (radio)
        radio_enableAfOutput();
    else
        radio_disableAfOutput();
    gpio_pin_set_dt(&speaker_enable, speaker);
}

void audio_init(void)
{
    k_mutex_lock(&board_mutex, K_FOREVER);
    init_audio();
    k_mutex_unlock(&board_mutex);
}

void audio_terminate(void)
{
    k_mutex_lock(&board_mutex, K_FOREVER);
    if (initialized) {
        memset(paths, 0, sizeof(paths));
        update_mux();
        csk6codec_terminate();
        initialized = false;
    }
    k_mutex_unlock(&board_mutex);
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    if (source > SOURCE_MCU || sink > SINK_MCU)
        return;
    k_mutex_lock(&board_mutex, K_FOREVER);
    init_audio();
    if (initialized && !paths[source][sink]) {
        paths[source][sink] = true;
        csk6codec_setRoute(source, sink, true);
        update_mux();
    }
    k_mutex_unlock(&board_mutex);
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    if (source > SOURCE_MCU || sink > SINK_MCU)
        return;
    k_mutex_lock(&board_mutex, K_FOREVER);
    if (initialized && paths[source][sink]) {
        paths[source][sink] = false;
        csk6codec_setRoute(source, sink, false);
        update_mux();
    }
    k_mutex_unlock(&board_mutex);
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink p2Sink)
{
    /* Physical sources and sinks are exclusive. MCU is a memory endpoint,
     * so separate capture and playback streams can share it. */
    return (p1Source != p2Source || p1Source == SOURCE_MCU)
        && (p1Sink != p2Sink || p1Sink == SINK_MCU);
}
