/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 device-routing debug layer -- implementation.  See hd2_router.h.
 *
 * Drives the audio_HD2 path matrix (interfaces/audio.h) and pokes the named
 * HD2 routing/power/codec registers via the hd2_regs.h macros.  The AT1846S RF
 * path goes through the loader's proven C-linkage helpers (radio_HD2.cpp).
 * The UART output for the dump reuses main.c's puts_/put_hex* (de-static'd).
 */

#include "hd2_router.h"
#include "hd2_regs.h"
#include "interfaces/audio.h"
#include <stdio.h>

/* --- externs ----------------------------------------------------------- */

/* AT1846S register access (radio_HD2.cpp; same path as loader 'q'/'Q').
 * Present in both the bare-loader and threaded builds via ORTX_RADIO. */
extern uint16_t hd2_at1846s_read(uint8_t reg);
extern void     hd2_at1846s_write(uint8_t reg, uint16_t val);

/* --- AT1846S reg 0x30 bit roles (chip regs, not SoC MMIO) -------------- */
#define AT1846S_R30_TX_ON           0x0040u   /* bit6: enter TX (keys carrier) */

/* --- TX/PA arm guard --------------------------------------------------- */

static int g_tx_armed = 0;

void hd2_route_arm_tx(void)
{
    g_tx_armed = 1;
}

static int target_is_guarded(uint8_t t)
{
    return (t == RT_PA_ENABLE) || (t == RT_TX_MODE) || (t == RT_PWR_HOLD);
}

/* --- open-path tracking (for compatibility enforcement) ---------------- */

#define MAX_OPEN_PATHS 4
static struct { uint8_t src; uint8_t sink; uint8_t used; } g_open[MAX_OPEN_PATHS];

static int path_is_tx(uint8_t source, uint8_t sink)
{
    /* The only TX (carrier-keying) audio path today is mic -> transceiver. */
    return (source == SOURCE_MIC) && (sink == SINK_RTX);
}

int hd2_router_connect(uint8_t source, uint8_t sink)
{
    if(source > SOURCE_MCU || sink > SINK_MCU)
        return -3;

    /* Guard the TX path behind the one-shot arm.  PEEK here -- consume the arm
     * only once we actually commit to audio_connect below, so a connect that is
     * rejected (incompatible path) doesn't silently burn the arm. */
    if(path_is_tx(source, sink) && !g_tx_armed)
        return -2;

    /* Reject if incompatible with any already-open path. */
    for(int i = 0; i < MAX_OPEN_PATHS; ++i)
    {
        if(!g_open[i].used)
            continue;
        if(!audio_checkPathCompatibility((enum AudioSource)source,
                                         (enum AudioSink)sink,
                                         (enum AudioSource)g_open[i].src,
                                         (enum AudioSink)g_open[i].sink))
            return -1;
    }

    if(path_is_tx(source, sink))
        g_tx_armed = 0;                  /* commit: consume the one-shot arm */
    audio_connect((enum AudioSource)source, (enum AudioSink)sink);

    /* Record it (ignore if already tracked; drop silently if table full). */
    for(int i = 0; i < MAX_OPEN_PATHS; ++i)
    {
        if(g_open[i].used && g_open[i].src == source && g_open[i].sink == sink)
            return 0;
    }
    for(int i = 0; i < MAX_OPEN_PATHS; ++i)
    {
        if(!g_open[i].used)
        {
            g_open[i].src = source; g_open[i].sink = sink; g_open[i].used = 1;
            break;
        }
    }
    return 0;
}

int hd2_router_disconnect(uint8_t source, uint8_t sink)
{
    if(source > SOURCE_MCU || sink > SINK_MCU)
        return -3;

    audio_disconnect((enum AudioSource)source, (enum AudioSink)sink);

    for(int i = 0; i < MAX_OPEN_PATHS; ++i)
    {
        if(g_open[i].used && g_open[i].src == source && g_open[i].sink == sink)
            g_open[i].used = 0;
    }
    return 0;
}

/* --- named target set/get ---------------------------------------------- */

uint32_t hd2_route_set(uint8_t target, uint32_t value)
{
    const int on = (value != 0u);

    /* Guarded targets need a prior one-shot arm.  PEEK then consume only on
     * commit, so a refused op doesn't burn the arm.  (Guarded targets all map
     * to real cases below, so consuming here == consuming on commit.) */
    if(target_is_guarded(target))
    {
        if(!g_tx_armed) return HD2_ROUTE_REFUSED;
        g_tx_armed = 0;
    }

    switch(target)
    {
        case RT_SPKR_AMP:                       /* PTB4 active-low: LOW = on;
                                                 * PTB17 active-HIGH gain stage
                                                 * rides along (see hd2_regs.h) */
            if(on)
            {
                gpio_atomic_clear(&GPIOB_DR, SPKR_AMP_BIT);
                gpio_atomic_set(&GPIOB_DR,   SPKR_GAIN_BIT);
            }
            else
            {
                gpio_atomic_set(&GPIOB_DR,   SPKR_AMP_BIT);
                gpio_atomic_clear(&GPIOB_DR, SPKR_GAIN_BIT);
            }
            return (GPIOB_DR & SPKR_AMP_BIT) ? 0u : 1u;

        case RT_AUDIO_ROUTE:                    /* PTB10 active-low: LOW = routed */
            if(on) gpio_atomic_clear(&GPIOB_DR, AUDIO_ROUTE_BIT);
            else   gpio_atomic_set(&GPIOB_DR,   AUDIO_ROUTE_BIT);
            return (GPIOB_DR & AUDIO_ROUTE_BIT) ? 0u : 1u;

        case RT_PWM_AUDIO_MUTE:                 /* DIPLEX0 bit18: 1 = mute */
            if(on) SOCSYS_IO_DIPLEX0 |= DIPLEX0_AUDIO_MUTE;
            else   SOCSYS_IO_DIPLEX0 &= ~DIPLEX0_AUDIO_MUTE;
            return (SOCSYS_IO_DIPLEX0 & DIPLEX0_AUDIO_MUTE) ? 1u : 0u;

        case RT_VOICE_PATH:  SOCSYS_VOICE_PATH   = value; return SOCSYS_VOICE_PATH;
        case RT_AF_GATE:     SOCSYS_AF_GATE      = value; return SOCSYS_AF_GATE;
        case RT_LINEOUT:     SOCSYS_LINEOUT_CTRL = value; return SOCSYS_LINEOUT_CTRL;
        case RT_DAC_POWER:   SOCSYS_DAC_CONTROL  = value; return SOCSYS_DAC_CONTROL;
        case RT_ADC_POWER:   SOCSYS_ADC_CONTROL  = value; return SOCSYS_ADC_CONTROL;

        case RT_CODEC_DAC_MUTE:                 /* soft-mute bit7: 1 = muted */
            if(on) CODEC_DAC_MUTE |= CODEC_DAC_SOFT_MUTE;
            else   CODEC_DAC_MUTE &= (uint8_t)~CODEC_DAC_SOFT_MUTE;
            return (CODEC_DAC_MUTE & CODEC_DAC_SOFT_MUTE) ? 1u : 0u;

        case RT_CODEC_DAC_EN:                   /* enable -> CLEAR the en bit (0=on) */
            if(on) CODEC_DAC_CFG &= (uint8_t)~CODEC_DAC_CFG_EN_BIT;
            else   CODEC_DAC_CFG |= CODEC_DAC_CFG_EN_BIT;
            return (CODEC_DAC_CFG & CODEC_DAC_CFG_EN_BIT) ? 0u : 1u;

        case RT_CODEC_ADC_EN:
            if(on) CODEC_ADC_CFG &= (uint8_t)~CODEC_ADC_CFG_EN_BIT;
            else   CODEC_ADC_CFG |= CODEC_ADC_CFG_EN_BIT;
            return (CODEC_ADC_CFG & CODEC_ADC_CFG_EN_BIT) ? 0u : 1u;

        case RT_DAC_GAIN:                       /* AT1846S reg 0x44 RX volume */
            hd2_at1846s_write(0x44, (uint16_t)(0x0900u | (value & 0xffu)));
            return hd2_at1846s_read(0x44);

        case RT_INPUT_GAIN:                     /* AT1846S reg 0x41 mic gain/AGC */
            hd2_at1846s_write(0x41, (uint16_t)(value & 0xffffu));
            return hd2_at1846s_read(0x41);

        case RT_PA_ENABLE: {                    /* GUARDED: AT1846S TX-on bit6 */
            uint16_t r = hd2_at1846s_read(0x30);
            r = on ? (uint16_t)(r | AT1846S_R30_TX_ON)
                   : (uint16_t)(r & ~AT1846S_R30_TX_ON);
            hd2_at1846s_write(0x30, r);
            return hd2_at1846s_read(0x30);
        }

        case RT_TX_MODE:                        /* GUARDED: AT1846S reg 0x30 full word */
            hd2_at1846s_write(0x30, (uint16_t)(value & 0xffffu));
            return hd2_at1846s_read(0x30);

        case RT_PWR_HOLD:                       /* GUARDED: PTB13 (0 = power off!) */
            if(on) gpio_atomic_set(&GPIOB_DR,   PWR_HOLD_BIT);
            else   gpio_atomic_clear(&GPIOB_DR, PWR_HOLD_BIT);
            return (GPIOB_DR & PWR_HOLD_BIT) ? 1u : 0u;

        case RT_APC_LEVEL: {                    /* APC TX power (DAC ch B, 12-bit);
                                                 * stored level, applied at next TX
                                                 * key-up by radio_enableTx. */
            extern void     hd2_apc_set(uint16_t level);
            extern uint16_t hd2_apc_get(void);
            hd2_apc_set((uint16_t)value);
            return hd2_apc_get();
        }

        default:
            return HD2_ROUTE_REFUSED;
    }
}

uint32_t hd2_route_get(uint8_t target)
{
    switch(target)
    {
        case RT_SPKR_AMP:        return (GPIOB_DR & SPKR_AMP_BIT)      ? 0u : 1u;
        case RT_AUDIO_ROUTE:     return (GPIOB_DR & AUDIO_ROUTE_BIT)   ? 0u : 1u;
        case RT_PWM_AUDIO_MUTE:  return (SOCSYS_IO_DIPLEX0 & DIPLEX0_AUDIO_MUTE) ? 1u : 0u;
        case RT_VOICE_PATH:      return SOCSYS_VOICE_PATH;
        case RT_AF_GATE:         return SOCSYS_AF_GATE;
        case RT_LINEOUT:         return SOCSYS_LINEOUT_CTRL;
        case RT_DAC_POWER:       return SOCSYS_DAC_CONTROL;
        case RT_ADC_POWER:       return SOCSYS_ADC_CONTROL;
        case RT_CODEC_DAC_MUTE:  return (CODEC_DAC_MUTE & CODEC_DAC_SOFT_MUTE) ? 1u : 0u;
        case RT_CODEC_DAC_EN:    return (CODEC_DAC_CFG  & CODEC_DAC_CFG_EN_BIT)? 0u : 1u;
        case RT_CODEC_ADC_EN:    return (CODEC_ADC_CFG  & CODEC_ADC_CFG_EN_BIT)? 0u : 1u;
        case RT_DAC_GAIN:        return hd2_at1846s_read(0x44);
        case RT_INPUT_GAIN:      return hd2_at1846s_read(0x41);
        case RT_PA_ENABLE:       return (hd2_at1846s_read(0x30) & AT1846S_R30_TX_ON) ? 1u : 0u;
        case RT_TX_MODE:         return hd2_at1846s_read(0x30);
        case RT_PWR_HOLD:        return (GPIOB_DR & PWR_HOLD_BIT) ? 1u : 0u;
        default:                 return HD2_ROUTE_REFUSED;
    }
}

/* --- routing snapshot -------------------------------------------------- */

int hd2_route_dump(char *buf, unsigned buflen)
{
    /* AT1846S reg 0x30 -- NOTE: bit-bang I2C read may contend with a running
     * FM RSSI poll (see task C2); value may be stale if the radio thread is up. */
    unsigned r30 = (unsigned)hd2_at1846s_read(0x30);

    int n = snprintf(buf, buflen,
        "ROUTE:\r\n"
        "  DIPLEX0=%08x DIPLEX2=%08x\r\n"
        "  VOICE_PATH=%08x AF_GATE=%08x\r\n"
        "  DAC_CTRL=%08x ADC_CTRL=%08x\r\n"
        "  LINEOUT=%08x WORK_MODE=%08x\r\n"
        "  codec DAC_CFG=%02x ADC_CFG=%02x DAC_MUTE=%02x\r\n"
        "  GPIOB amp(on)=%u route(on)=%u pwr_hold=%u\r\n"
        "  AT1846S reg30=%04x\r\n",
        (unsigned)SOCSYS_IO_DIPLEX0,  (unsigned)SOCSYS_IO_DIPLEX2,
        (unsigned)SOCSYS_VOICE_PATH,  (unsigned)SOCSYS_AF_GATE,
        (unsigned)SOCSYS_DAC_CONTROL, (unsigned)SOCSYS_ADC_CONTROL,
        (unsigned)SOCSYS_LINEOUT_CTRL,(unsigned)SOCSYS_WORK_MODE,
        (unsigned)CODEC_DAC_CFG, (unsigned)CODEC_ADC_CFG, (unsigned)CODEC_DAC_MUTE,
        (GPIOB_DR & SPKR_AMP_BIT)    ? 0u : 1u,
        (GPIOB_DR & AUDIO_ROUTE_BIT) ? 0u : 1u,
        (GPIOB_DR & PWR_HOLD_BIT)    ? 1u : 0u,
        r30);

    if(n < 0) { if(buflen) buf[0] = '\0'; return 0; }
    if((unsigned)n >= buflen) n = (int)buflen - 1;   /* snprintf truncates + NUL-terminates */
    return n;
}

/* --- audio-path snapshot (loader op 'u') -------------------------------- */

int hd2_audio_snap(char *buf, unsigned buflen)
{
    /* AT1846S regs over the bit-bang bus -- run with rf_freeze=1 if the rtx
     * thread is polling RSSI, or values may interleave/corrupt (no bus lock). */
    unsigned r30 = (unsigned)hd2_at1846s_read(0x30);   /* mode/RX-TX enable   */
    unsigned r3a = (unsigned)hd2_at1846s_read(0x3a);   /* AF-source/voice mux */
    unsigned r40 = (unsigned)hd2_at1846s_read(0x40);   /* AF-DSP ctrl         */
    unsigned r44 = (unsigned)hd2_at1846s_read(0x44);   /* gain / RX volume    */
    unsigned r1b = (unsigned)hd2_at1846s_read(0x1b);   /* RSSI (upper byte)   */
    unsigned r0d = (unsigned)hd2_at1846s_read(0x0d);   /* PLL lock status     */

    uint32_t gb = GPIOB_EXT_PORT;                      /* live pin states     */
    uint32_t gc = GPIOC_EXT_PORT;

    int n = snprintf(buf, buflen,
        "AUDIO:\r\n"
        "  AT1846S 30=%04x 3a=%04x 40=%04x 44=%04x 1b=%04x 0d=%04x\r\n"
        "  GPIOB.ext b4=%u b10=%u b15=%u b17=%u b19=%u GPIOC.ext b3=%u\r\n"
        "  DIPLEX0=%08x DIPLEX1=%08x DIPLEX2=%08x\r\n"
        "  CLK_MGR2C=%08x codec CR_VIC(d2)=%02x AICR_DAC(c8)=%02x\r\n",
        r30, r3a, r40, r44, r1b, r0d,
        (unsigned)((gb >>  4) & 1u),  /* PTB4  speaker-amp mute (LOW=on)   */
        (unsigned)((gb >> 10) & 1u),  /* PTB10 RX-audio route (LOW=routed) */
        (unsigned)((gb >> 15) & 1u),
        (unsigned)((gb >> 17) & 1u),
        (unsigned)((gb >> 19) & 1u),
        (unsigned)((gc >>  3) & 1u),
        (unsigned)SOCSYS_IO_DIPLEX0,
        (unsigned)SOCSYS_IO_DIPLEX1,
        (unsigned)SOCSYS_IO_DIPLEX2,
        (unsigned)SOCSYS_REG2C,       /* gated-clock enables incl. [7] fmrx */
        (unsigned)CODEC_SLEEP,        /* CR_VIC: [1]sb_sleep [0]sb          */
        (unsigned)CODEC_DAC_CFG);     /* AICR_DAC: [4]IF-en(0=on) [5]master */

    if(n < 0) { if(buflen) buf[0] = '\0'; return 0; }
    if((unsigned)n >= buflen) n = (int)buflen - 1;
    return n;
}

/* --- voice-prompt doorbell experiment (docs/voice_prompt_map.md) -------- */

/*
 * Replicate the vendor beep-engine VOICE trigger from our firmware.  On the
 * vendor (v2.1.3), prompts play when:
 *   (a) the request struct g_beep_seq @ RAM 0x00043b38 holds
 *       [5]=msg_id, [0]|=0x02 (live-proven via dbgshell pokes), and
 *   (b) the step handler pulses the modem doorbell GPIOs
 *       (FUN_03057d60: clear GPIOB.28, clear GPIOB.29, set GPIOB.28).
 * The modem's internal CPU then plays clip <msg_id> from its mask ROM out the
 * codec DAC -> lineout -> amp (no app-side audio data at all).
 *
 * UNKNOWN: where the modem reads msg_id from.  Best guess = it snoops the
 * vendor's g_beep_seq RAM address over AHB, so we write the request image at
 * that PHYSICAL address (0x00043b38) regardless of our own RAM layout.  In
 * OUR build those 16 bytes may belong to heap/.bss -- they are saved and
 * restored after the pulse (~60 ms exposure; an experiment-grade risk).
 *
 * mode 0: struct write + single doorbell pulse (vendor-faithful)
 * mode 1: struct write + (id+1) pulses, 1 ms apart (tests pulse-count coding)
 * mode 2: doorbell pulse only, no struct write (control)
 */
#define VP_BEEP_SEQ_ADDR   0x00043b38u
#define VP_DOORBELL_A      (1u << 28)   /* GPIOB.28 = vendor gpio event 0x3c */
#define VP_DOORBELL_B      (1u << 29)   /* GPIOB.29 = vendor gpio event 0x3d */

static void vp_delay_us(unsigned us)
{
    for (volatile unsigned i = 0; i < us * 8u; ++i) { }
}

static void vp_doorbell_pulse(void)
{
    /* Exact vendor handler order: clear A, clear B, set A. */
    gpio_atomic_clear(&GPIOB_DR, VP_DOORBELL_A);
    gpio_atomic_clear(&GPIOB_DR, VP_DOORBELL_B);
    gpio_atomic_set(&GPIOB_DR, VP_DOORBELL_A);
}

uint32_t hd2_vp_fire(uint8_t msg_id, uint8_t mode)
{
    volatile uint8_t *bs = (volatile uint8_t *)VP_BEEP_SEQ_ADDR;
    uint8_t saved[16];
    unsigned i;

    gpio_atomic_set(&GPIOB_DDR, VP_DOORBELL_A | VP_DOORBELL_B);

    if (mode != 2u)
    {
        for (i = 0; i < 16u; ++i) saved[i] = bs[i];
        for (i = 0; i < 16u; ++i) bs[i] = 0u;
        bs[5] = msg_id;          /* active msg_id              */
        bs[0] = 0x22u;           /* idle 0x20 | request 0x02   */
    }

    if (mode == 1u)
    {
        for (i = 0; i <= msg_id; ++i) { vp_doorbell_pulse(); vp_delay_us(1000); }
    }
    else
    {
        vp_doorbell_pulse();
    }

    if (mode != 2u)
    {
        vp_delay_us(60000);      /* give the modem time to latch the request */
        for (i = 0; i < 16u; ++i) bs[i] = saved[i];
    }
    return 0u;
}
