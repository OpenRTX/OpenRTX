/*
 * SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * OpenRTX audio.h routing driver for the Ailunce HD2 (HR_C7000).
 *
 * Implements the standard OpenRTX audio path matrix (sources {MIC,RTX,MCU} ->
 * sinks {SPK,RTX,MCU}), modeled on platform/drivers/audio/audio_GDx.c.  It is
 * the single owner of the HD2's *board-level* audio routing: the GPIOB
 * speaker-amp (PTB4) + gain (PTB17) + RX-audio route (PTB10) lines that
 * radio_HD2.cpp deliberately left "to the audio agent", plus the codec /
 * socsys audio-gate bring-up.
 *
 * Division of labour (one source of truth for the HW-verified register
 * values):
 *   - This file OWNS the HR_C7000 codec bring-up (codec_bringup(), the
 *     byte-register CB() sequence captured LIVE from a vendor unit playing FM).
 *     It runs at platform_init time from audio_init(), like every other OpenRTX
 *     target, and is idempotent (a static latch makes repeat calls free).
 *   - hd2_audio_out_warm() sets the codec audio-OUTPUT stage (LINEOUT_CTRL/
 *     AUDIO_CONTROL/AUDIO_BUFFER_CLR) for MCU-PCM playback (beep path).
 *   - The AT1846S RX-audio chip-side mute (reg 0x30 bit7) is released via the
 *     radio.h hook radio_enableAfOutput()/disableAfOutput().
 *   - This file also owns the board-level GPIO twiddles.
 *
 * MCU->codec-DAC PCM playback (beep / voice prompt) is wired: the SINK_SPK
 * output device carries the outputStream_HD2.cpp driver, and the
 * audio_connect(SOURCE_MCU, SINK_SPK) case warms the codec + routes the amp
 * before the stream arms.  The analog FM-RX path (SOURCE_RTX -> SINK_SPK) is
 * pure GPIO and is fully preserved.
 *
 * TX (MIC->RTX) is intentionally not keyed here.
 */

#include "interfaces/audio.h"
#include "interfaces/radio.h"
#include "interfaces/delays.h"
#include "drivers/GPIO/gpio_hrc7000.h"
#include "pinmap.h" /* -> registers.h (SOCSYS, CODEC_BYTE) */

/*
 * HR_C7000 PCM/audio-codec block (byte-addressed MMIO @ 0x16000900, verified
 * alive + readable on hardware).  Port of boot_init_modem_audio_path
 * (0x0305e62c) + the analog-FM tail of boot_init_audio_route (0x0305e7d8).
 * MUST use byte writes -- the block ignores partial word writes.
 *
 * CODEC_BYTE(off) comes from registers.h (byte-addressed PCM block); keep the
 * short local CB() name this driver uses.
 */
#define CB(off) CODEC_BYTE(off)

/* Codec DAC output gain (GCR_DACL @ codec 0xdf): vendor sources the low 6 bits
 * (godl) from codeplug calibration; our FM build has no codeplug, so use the
 * vendor's known-good value (= g_tune_data 0x34 | 0x80, from a vendor unit
 * playing FM) so the codec DAC isn't mis-gained. */
#define CODEC_DACL_GAIN 0xb4u

static bool g_codec_inited = false;

/*
 * HR_C7000 codec bring-up (verbatim vendor sequence).  Pulses the PCM block
 * reset then clears the codec soft-reset (SOCSYS->SYS_SOFT_RSTN bit4 once; the
 * bit is auto-releasing, see radio_HD2.cpp's SYS_SOFT_RSTN note), waits on the
 * LINEOUT_CTRL[31] standby handshake, then sets the DAC gain + lineout.  Runs
 * once at platform_init time from audio_init(); idempotent.
 *
 * Ownership contract: audio_init() (this file) owns the codec/audio-if reset
 * bits (3,4); the modem FM boot (radio_HD2.cpp hd2_modem_fm_boot_init) PRESERVES
 * bits 3,4 via RMW so it never disturbs the codec state set up here.
 */
static void codec_bringup(void)
{
    if (g_codec_inited)
        return;

    /* PCM block reset pulse + codec soft-reset (matches vendor exactly). */
    CB(0xd2) |= 0x03;
    SOCSYS->SYS_SOFT_RSTN &= 0xffffffefu; /* clear bit4 = codec reset */
    delayMs(2);
    CB(0xd2) &= ~0x01;
    delayMs(100);
    CB(0xd2) &= ~0x02;
    CB(0xd3) = 0x40;
    CB(0xcb) = 0x00;
    CB(0xcc) |= 0x40;
    CB(0xc9) = 0xc0;
    CB(0xcf) &= ~0x10;
    delayMs(100);
    CB(0xcf) &= ~0x80;
    CB(0xc8) = 0xc0;
    CB(0xcd) &= ~0x10;

    /* Wait for the PCM handshake (socsys 0x88 bit31 clear), bounded. */
    for (uint32_t g = 0; g < 200u; ++g) {
        if ((SOCSYS->LINEOUT_CTRL & 0x80000000u) == 0u)
            break;
        delayMs(10);
    }

    CB(0xcd) &= ~0x80;
    delayMs(100);
    CB(0xdf) = CODEC_DACL_GAIN;
    CB(0xe5) = 0x8b;
    SOCSYS->LINEOUT_CTRL = 1;

    /* boot_init_audio_route analog tail (same block). */
    CB(0xc8) = 0xc0;
    CB(0xcd) = 0x20;
    CB(0xe5) = 0x8b;
    CB(0xdf) = CODEC_DACL_GAIN;
    SOCSYS->AUDIO_BUFFER_CLR |= 0x02; /* _hrc7000_pcm_mode |= 2 */

    g_codec_inited = true;
}

/*
 * Warm up the HR_C7000 codec audio-OUTPUT stage ONCE for MCU-PCM playback: make
 * sure the codec is up, then open the socsys audio gate (LINEOUT/AUDIO_CONTROL/
 * AUDIO_BUFFER_CLR).  The PWM-beep mixes THROUGH the codec (DAC -> lineout ->
 * speaker), so the codec must be initialised before a beep is audible.  Called
 * lazily from the beep path.  GPIOB.4/.10 (amp + route) are left to the caller
 * so they re-assert per beep.
 */
void hd2_audio_out_warm(void)
{
    static bool warmed = false;
    if (warmed)
        return;
    codec_bringup();
    SOCSYS->AUDIO_CONTROL = 0x00000002u;
    SOCSYS->AUDIO_BUFFER_CLR = 0x00000000u;
    SOCSYS->LINEOUT_CTRL =
        0x00000003u; /* both lineouts; speaker is on LINE2OUT */
    warmed = true;
}

#define PATH(x, y) (((x) << 4) | (y))

/*
 * Path compatibility matrix -- can two paths be open simultaneously?
 * Indexed by (source * 3 + sink) for each of the two paths.
 *
 * Row/col order: MIC-SPK MIC-RTX MIC-MCU RTX-SPK RTX-RTX RTX-MCU MCU-SPK MCU-RTX MCU-MCU
 */
static const uint8_t pathCompatibilityMatrix[9][9] = {
    //         M-S M-R M-M R-S R-R R-M C-S C-R C-M
    /* MIC-SPK */ { 0, 0, 0, 0, 1, 1, 0, 1, 1 },
    /* MIC-RTX */ { 0, 0, 0, 1, 0, 1, 1, 0, 1 },
    /* MIC-MCU */ { 0, 0, 0, 1, 1, 0, 1, 1, 0 },
    /* RTX-SPK */ { 0, 1, 1, 0, 0, 0, 0, 1, 1 },
    /* RTX-RTX */ { 1, 0, 1, 0, 0, 0, 1, 0, 1 },
    /* RTX-MCU */ { 1, 1, 0, 0, 0, 0, 1, 1, 0 },
    /* MCU-SPK */ { 0, 1, 1, 0, 1, 1, 0, 0, 0 },
    /* MCU-RTX */ { 1, 0, 1, 1, 0, 1, 0, 0, 0 },
    /* MCU-MCU */ { 1, 1, 0, 1, 1, 0, 0, 0, 0 }
};

/* CPU->codec-DAC PCM stream driver (outputStream_HD2.cpp), 8 kHz mono s16 --
 * the SINK_SPK output device.  SINK_MCU / SINK_RTX carry no driver here. */
extern const struct audioDriver hd2_pcm_audio_driver;

const struct audioDevice outputDevices[] = {
    { NULL, NULL, 0, SINK_MCU },
    { NULL, NULL, 0, SINK_RTX },
    { &hd2_pcm_audio_driver, NULL, 0, SINK_SPK },
};

const struct audioDevice inputDevices[] = {
    { NULL, 0, 0, SINK_MCU },
    { NULL, 0, 0, SINK_RTX },
    { NULL, 0, 0, SINK_SPK },
};

/* --- board-level helpers ------------------------------------------------ */

static inline void spkr_amp_mute(void)
{
    gpio_setPin(GPIOB, SPKR_AMP_PIN);    /* PTB4  HIGH = muted    */
    gpio_clearPin(GPIOB, SPKR_GAIN_PIN); /* PTB17 LOW  = low gain */
}

static inline void spkr_amp_unmute(void)
{
    gpio_clearPin(GPIOB, SPKR_AMP_PIN); /* PTB4  LOW  = on       */
    gpio_setPin(GPIOB, SPKR_GAIN_PIN);  /* PTB17 HIGH = full gain
                                            * (without it ALL speaker
                                            * audio is barely audible
                                            * -- see pinmap.h) */
}

static inline void rx_route_on(void)
{
    gpio_clearPin(GPIOB, AUDIO_ROUTE_PIN);
} /* PTB10 LOW = routed */

static inline void rx_route_off(void)
{
    gpio_setPin(GPIOB, AUDIO_ROUTE_PIN);
} /* PTB10 HIGH = un-routed (close the route gate) */

void audio_init()
{
    /* Drive the amp + gain + route lines as outputs; start with the speaker
     * muted (PTB4 HIGH, PTB17 LOW), matching the vendor's tuning state. */
    gpio_setMode(GPIOB, SPKR_AMP_PIN, OUTPUT);
    gpio_setMode(GPIOB, SPKR_GAIN_PIN, OUTPUT);
    gpio_setMode(GPIOB, AUDIO_ROUTE_PIN, OUTPUT);
    spkr_amp_mute();
    rx_route_off(); /* start with the RX-audio route closed (PTB10 HIGH) */

    /* Bring the HR_C7000 codec up at platform_init, like every other target. */
    codec_bringup();
}

void audio_terminate()
{
    spkr_amp_mute();
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    switch (PATH(source, sink)) {
        case PATH(SOURCE_RTX, SINK_SPK):
            /* FM RX audio -> speaker.  PURE-GPIO gate (must be trivial: this
             * fires on every squelch crossing).  The heavy AT1846S AF-DSP
             * config + chip-side unmute already ran once in radio_enableRx;
             * here we only route the analog AF (PTB10) and unmute the speaker
             * amp (PTB4). */
            rx_route_on();
            spkr_amp_unmute();
            break;

        case PATH(SOURCE_MCU, SINK_SPK):
            /* Beep / voice-prompt playback from the MCU (mixes through the
             * codec DAC -> lineout -> speaker, so the codec must be warm). */
            hd2_audio_out_warm();
            radio_disableAfOutput(); /* mute the live analog FM-RX audio */
            rx_route_on();
            spkr_amp_unmute();
            break;

        case PATH(SOURCE_MIC, SINK_RTX):
            /* TX (mic -> transceiver).  Keying is owned by radio_enableTx;
             * left as an explicit no-op here. */
            break;

        default:
            break;
    }
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    switch (PATH(source, sink)) {
        case PATH(SOURCE_RTX, SINK_SPK):
            /* Squelch gate: mute the amp AND close the RX-audio route (PTB10).
             * The route is opened on connect but was never closed, so it latched
             * open on the first RX and left the AT1846S demod noise floor a
             * permanent path to the amp (a faint squeal that appeared on the
             * first audio event and persisted).  The AT1846S AF config +
             * chip-side unmute stay in place so re-open is still a GPIO toggle. */
            spkr_amp_mute();
            rx_route_off();
            break;

        case PATH(SOURCE_MCU, SINK_SPK):
            spkr_amp_mute();
            rx_route_off();         /* close the RX-audio route (PTB10) */
            radio_enableAfOutput(); /* restore the FM-RX AF output */
            break;

        default:
            break;
    }
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink p2Sink)
{
    uint8_t p1Index = (p1Source * 3) + p1Sink;
    uint8_t p2Index = (p2Source * 3) + p2Sink;

    return pathCompatibilityMatrix[p1Index][p2Index] == 1;
}
