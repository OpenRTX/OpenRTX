/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * OpenRTX audio.h routing driver for the Ailunce HD2 (HR_C7000).
 *
 * Implements the standard OpenRTX audio path matrix (sources {MIC,RTX,MCU} ->
 * sinks {SPK,RTX,MCU}), modeled on platform/drivers/audio/audio_GDx.c.  This
 * replaces audio_stub.c and is the single owner of the HD2's *board-level*
 * audio routing: the GPIOB speaker-amp (PTB4) + RX-audio route (PTB10) lines
 * that radio_HD2.cpp deliberately left "to the audio agent", plus the codec /
 * socsys audio-gate bring-up.
 *
 * Division of labour (so we keep ONE source of truth for the HW-verified
 * register values and don't fight the in-flight FM-RX-audio task #C2):
 *   - The codec init + socsys audio-gate sequence (DAC/ADC/VOICE_PATH/LINEOUT/
 *     WORK_MODE/AF_GATE/MODEM_RXDP) was captured LIVE from a vendor unit playing
 *     FM and lives in radio_HD2.cpp::hd2_audio_out_warm().  We CALL that
 *     rather than re-hardcoding the magic constants here.
 *   - The AT1846S RX-audio chip-side mute (reg 0x30 bit7) is released via the
 *     radio.h hook radio_enableAfOutput()/disableAfOutput().
 *   - This file owns only the named-macro GPIO twiddles (hd2_regs.h).
 *
 * TX (MIC->RTX) is intentionally not keyed here: TX/PA bring-up is gated behind
 * the hd2_router "arm" guard and radio_HD2 still refuses to transmit.
 */

#include "interfaces/audio.h"
#include "interfaces/radio.h"
#include "hd2_regs.h"

/* Codec + socsys audio-gate warm-up (HW-verified constants live in
 * radio_HD2.cpp).  Idempotent: a static latch makes repeat calls free. */
extern void hd2_audio_out_warm(void);

/* RF-freeze flag (radio_HD2.cpp, loader op 'z').  While set, the
 * audio matrix must not rewrite the amp/route GPIOs (PTB4/PTB10) that a
 * host-side experiment may be holding -- audio_connect/disconnect become
 * no-ops.  (rtx_task's squelch gate is also frozen upstream in hd2_rtx.c;
 * this catches any other caller, e.g. UI beep paths.) */
extern volatile uint32_t g_rf_freeze;

#define PATH(x, y)  (((x) << 4) | (y))

/*
 * Path compatibility matrix -- can two paths be open simultaneously?
 * Indexed by (source * 3 + sink) for each of the two paths.  Started from the
 * GDx template; HD2 reality (single shared codec DAC -> speaker, single AT1846S
 * AF source) is stricter, so this needs empirical tightening as paths come up.
 * Conservative reading: paths that contend for the speaker/codec-DAC or the RTX
 * AF source must not co-exist.  (TODO: validate on HW per task acceptance.)
 *
 * Row/col order: MIC-SPK MIC-RTX MIC-MCU RTX-SPK RTX-RTX RTX-MCU MCU-SPK MCU-RTX MCU-MCU
 */
static const uint8_t pathCompatibilityMatrix[9][9] =
{
    //         M-S M-R M-M R-S R-R R-M C-S C-R C-M
    /* MIC-SPK */ { 0 , 0 , 0 , 0 , 1 , 1 , 0 , 1 , 1 },
    /* MIC-RTX */ { 0 , 0 , 0 , 1 , 0 , 1 , 1 , 0 , 1 },
    /* MIC-MCU */ { 0 , 0 , 0 , 1 , 1 , 0 , 1 , 1 , 0 },
    /* RTX-SPK */ { 0 , 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 },
    /* RTX-RTX */ { 1 , 0 , 1 , 0 , 0 , 0 , 1 , 0 , 1 },
    /* RTX-MCU */ { 1 , 1 , 0 , 0 , 0 , 0 , 1 , 1 , 0 },
    /* MCU-SPK */ { 0 , 1 , 1 , 0 , 1 , 1 , 0 , 0 , 0 },
    /* MCU-RTX */ { 1 , 0 , 1 , 1 , 0 , 1 , 0 , 0 , 0 },
    /* MCU-MCU */ { 1 , 1 , 0 , 1 , 1 , 0 , 0 , 0 , 0 }
};

/* CPU->codec-DAC PCM stream driver (outputStream_HD2.cpp): SAHB PCM mailbox
 * fed by the modem's 100 Hz frame IRQ.  8 kHz mono s16. */
extern const struct audioDriver hd2_pcm_audio_driver;

const struct audioDevice outputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {&hd2_pcm_audio_driver, NULL, 0, SINK_SPK},
};

const struct audioDevice inputDevices[] =
{
    {NULL, 0, 0, SINK_MCU},
    {NULL, 0, 0, SINK_RTX},
    {NULL, 0, 0, SINK_SPK},
};

/* --- board-level helpers (named macros, no magic addresses) ------------- */

/* GPIOB_DR is shared with the router + FM worker; RMW under gpio_atomic_* so a
 * preemption can't drop an update (worst case: PTB13 power-hold). */
static inline void spkr_amp_mute(void)
{
    gpio_atomic_set(&GPIOB_DR,   SPKR_AMP_BIT);    /* PTB4  HIGH = muted    */
    gpio_atomic_clear(&GPIOB_DR, SPKR_GAIN_BIT);   /* PTB17 LOW  = low gain */
}

static inline void spkr_amp_unmute(void)
{
    gpio_atomic_clear(&GPIOB_DR, SPKR_AMP_BIT);    /* PTB4  LOW  = on       */
    gpio_atomic_set(&GPIOB_DR,   SPKR_GAIN_BIT);   /* PTB17 HIGH = full gain
                                                    * (without it ALL speaker
                                                    * audio is barely audible
                                                    * -- see hd2_regs.h) */
}

static inline void rx_route_on(void)     { gpio_atomic_clear(&GPIOB_DR, AUDIO_ROUTE_BIT); }/* PTB10 LOW = routed */

void audio_init()
{
    /* Drive the amp + gain + route lines as outputs; start with the speaker
     * muted (PTB4 HIGH, PTB17 LOW), matching the vendor's tuning state. */
    gpio_atomic_set(&GPIOB_DDR, SPKR_AMP_BIT | SPKR_GAIN_BIT | AUDIO_ROUTE_BIT);
    spkr_amp_mute();
}

void audio_terminate()
{
    spkr_amp_mute();
}

void audio_connect(const enum AudioSource source, const enum AudioSink sink)
{
    if(g_rf_freeze != 0u)                /* rf_freeze: no audio-GPIO rewrites */
        return;

    switch(PATH(source, sink))
    {
        case PATH(SOURCE_RTX, SINK_SPK):
            /* FM RX audio -> speaker.  PURE-GPIO gate (must be trivial: this
             * fires on every squelch crossing).  The heavy AT1846S AF-DSP
             * config + chip-side unmute already ran once in radio_enableRx;
             * here we only route the analog AF (PTB10) and unmute the speaker
             * amp (PTB4) -- same nodes the broadcast tuner drives. */
            rx_route_on();
            spkr_amp_unmute();
            break;

        case PATH(SOURCE_MCU, SINK_SPK):
            /* Beep / voice-prompt playback from the MCU (PWM ch1 mixes through
             * the codec DAC -> lineout -> speaker, so the codec must be warm). */
            hd2_audio_out_warm();
            rx_route_on();
            spkr_amp_unmute();
            break;

        case PATH(SOURCE_MIC, SINK_RTX):
            /* TX (mic -> transceiver).  NOT keyed here -- TX/PA bring-up is
             * gated behind the hd2_router arm guard and radio_HD2 refuses TX.
             * Left as an explicit no-op until the TX audio path is verified. */
            break;

        default:
            /* Unimplemented paths (MCU/RTX capture, etc.) are no-ops for now. */
            break;
    }
}

void audio_disconnect(const enum AudioSource source, const enum AudioSink sink)
{
    if(g_rf_freeze != 0u)                /* rf_freeze: no audio-GPIO rewrites */
        return;

    switch(PATH(source, sink))
    {
        case PATH(SOURCE_RTX, SINK_SPK):
            /* Squelch gate: re-mute the amp (pure GPIO).  Leave the AT1846S AF
             * config + chip-side unmute in place so the next open is just a
             * GPIO toggle (matches the vendor's per-squelch amp flutter). */
            spkr_amp_mute();
            break;

        case PATH(SOURCE_MCU, SINK_SPK):
            spkr_amp_mute();
            break;

        default:
            break;
    }
}

bool audio_checkPathCompatibility(const enum AudioSource p1Source,
                                  const enum AudioSink   p1Sink,
                                  const enum AudioSource p2Source,
                                  const enum AudioSink   p2Sink)
{
    uint8_t p1Index = (p1Source * 3) + p1Sink;
    uint8_t p2Index = (p2Source * 3) + p2Sink;

    return pathCompatibilityMatrix[p1Index][p2Index] == 1;
}
