/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 device-routing debug layer.  Sits on top of the audio_HD2 routing matrix
 * (interfaces/audio.h) and exposes, over the loader UART protocol:
 *   - matrix passthrough: connect/disconnect a (source,sink) audio path, gated
 *     by the audio_HD2 compatibility matrix;
 *   - named lower-level "twiddlers" for the HD2's diplex pin-mux, audio gate,
 *     codec AFE, power rails, and AT1846S RF path;
 *   - a routing snapshot dump.
 *
 * RF/PA-keying and the power-hold latch are GUARDED: they refuse to act unless
 * hd2_route_arm_tx() was called immediately before (one-shot), so a stray byte
 * can't key the transmitter or cut the radio's own power.
 */
#ifndef HD2_ROUTER_H
#define HD2_ROUTER_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* hd2_route_set/get sentinel: guarded op refused (TX not armed). */
#define HD2_ROUTE_REFUSED   0xFFFFFFFFu

typedef enum {
    /* --- safe: audio routing bits / gates --- */
    RT_SPKR_AMP = 0,    /* GPIOB PTB4 speaker amp (1=unmute/on, 0=mute)        */
    RT_AUDIO_ROUTE,     /* GPIOB PTB10 RX-audio route (1=routed, 0=off)        */
    RT_PWM_AUDIO_MUTE,  /* SOCSYS IO_DIPLEX0 bit18 (1=mute PWM-audio path)     */
    RT_VOICE_PATH,      /* SOCSYS VOICE_PATH (full word)                       */
    RT_AF_GATE,         /* SOCSYS AF_GATE (full word)                          */
    RT_LINEOUT,         /* SOCSYS LINEOUT_CTRL (full word)                     */
    RT_DAC_POWER,       /* SOCSYS DAC_CONTROL (full word)                      */
    RT_ADC_POWER,       /* SOCSYS ADC_CONTROL (full word)                      */
    /* --- codec AFE (byte-addressed @ 0x16000900 block) + AT1846S gains --- */
    RT_CODEC_DAC_MUTE,  /* codec DAC_SOFT_MUTE bit7 (1=mute, 0=unmute)         */
    RT_CODEC_DAC_EN,    /* codec DAC enable (1=on -> clears SB bit4)           */
    RT_CODEC_ADC_EN,    /* codec ADC enable (1=on -> clears SB bit4)           */
    RT_DAC_GAIN,        /* RX volume: AT1846S reg 0x44 low byte                */
    RT_INPUT_GAIN,      /* mic gain/AGC: AT1846S reg 0x41 (full 16-bit)        */
    /* --- GUARDED (require prior hd2_route_arm_tx) --- */
    RT_PA_ENABLE,       /* AT1846S reg 0x30 TX-on bit6 (KEYS THE CARRIER)      */
    RT_TX_MODE,         /* AT1846S reg 0x30 full word (band/TX-RX encoding)    */
    RT_PWR_HOLD,        /* GPIOB PTB13 power self-latch (0=CUTS POWER)         */
    RT_APC_LEVEL,       /* APC TX power level (CPU DAC ch B, 12-bit; next TX)  */
    RT_TARGET_COUNT
} hd2_route_target;

/* Open/close an audio path via the audio_HD2 matrix.  Returns 0 on success,
 * <0 on failure: -1 incompatible with an already-open path, -2 guarded path
 * (TX) not armed, -3 bad source/sink id.
 *
 * NOTE: the -1 compatibility check only considers paths opened THROUGH this
 * router (tracked in a small internal table).  OpenRTX core opens paths by
 * calling audio_connect() directly (audio_path.c), bypassing the router, so
 * those are invisible here -- treat this enforcement as a debug aid, not a
 * hard interlock against the running app's own audio routing. */
int hd2_router_connect(uint8_t source, uint8_t sink);
int hd2_router_disconnect(uint8_t source, uint8_t sink);

/* Apply value to a routing target.  Returns the post-write readback (decoded
 * field / register), or HD2_ROUTE_REFUSED if the target is guarded and TX was
 * not armed.  Guarded targets consume the arm (one-shot). */
uint32_t hd2_route_set(uint8_t target, uint32_t value);

/* Current decoded value of a target (no write). */
uint32_t hd2_route_get(uint8_t target);

/* Arm the guarded targets for the NEXT guarded hd2_route_set / TX connect. */
void hd2_route_arm_tx(void);

/* Format a human-readable routing snapshot into buf (NUL-terminated).  Returns
 * the number of chars written (excluding the NUL).  Self-contained so it links
 * in both the bare-loader (main.c) and threaded (hd2_diag.cpp) command paths --
 * the caller transmits the buffer over whichever UART it owns. */
int hd2_route_dump(char *buf, unsigned buflen);

/* Full audio-path snapshot (loader op 'u', audio_snap): AT1846S regs
 * 0x30/0x3a/0x40/0x44/0x1b(RSSI)/0x0d(PLL lock), GPIOB EXT bits 4/10/15/17/19,
 * GPIOC EXT bit3, DIPLEX0/1/2, CLK_MGR 0x2c, codec CR_VIC/AICR_DAC bytes.
 * Same buffer/snprintf contract as hd2_route_dump. */
int hd2_audio_snap(char *buf, unsigned buflen);

/* Voice-prompt doorbell experiment: replicate the vendor beep-engine voice
 * trigger (g_beep_seq image @0x00043b38 + GPIOB.28/29 doorbell pulse).
 * mode 0 = struct+1 pulse, 1 = struct+(id+1) pulses, 2 = pulse only.
 * See docs/voice_prompt_map.md. */
uint32_t hd2_vp_fire(uint8_t msg_id, uint8_t mode);

#ifdef __cplusplus
}
#endif

#endif /* HD2_ROUTER_H */
