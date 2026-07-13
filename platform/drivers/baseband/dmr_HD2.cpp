/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 DMR TX burst scheduler -- see dmr_HD2.h for the architecture and the
 * canonical clean-room reference (DMR_PATH_PROPOSAL.md §3.4/§4.3/§5.2/§11.2,
 * _findings/C-burst-sched.md).
 *
 * MMIO-only (modem RAM + HR_C7000 Layer-2 registers); safe in IRQ context.
 */

#include "dmr_HD2.h"
#include "hd2_regs.h"
#include <interfaces/interrupts.h>
#include <miosix.h>

using namespace miosix;

/* AT1846S register access (defined in radio_HD2.cpp; the proven exact-value
 * path the FM-TX bring-up uses). */
extern "C" void     hd2_at1846s_write(uint8_t reg, uint16_t val);
extern "C" uint16_t hd2_at1846s_read(uint8_t reg);

/* Vendor DMR-TX sync-word block: 14 words at modem 0x12c..0x160 (matches the
 * live vendor capture exactly).  The old leading 0x098006aau word was an
 * off-by-one that landed on 0x128 = PHASE_OUT (RO) -- dropped. */
static const uint32_t kDmrSync[14] = {
    0x00d5d7f7u, 0x007fd757u, 0x0077d55fu, 0x007dfd77u,
    0x80d5d7f7u, 0x007fd757u, 0x80dff57du, 0x0075df5du, 0x00f7fdd5u,
    0x00ddfd55u, 0x00d7557fu, 0x005ff7f5u, 0x0077d55fu, 0x007dfd77u,
};

/* Full Link Control Opcode (FLCO) for the LC word (ETSI TS 102 361-2). */
#define DMR_FLCO_GROUP     0x00u   /* group voice channel user        */
#define DMR_FLCO_PRIVATE   0x03u   /* unit-to-unit voice channel user */
#define DMR_FID_STANDARD   0x00u   /* feature set ID: ETSI standard   */

/* LAYER2_SEND_TYPE (0x418) field values. */
#define SEND_TYPE_VOICE    0x08u   /* bit3 localvod = 1 (voice); [7:4]=sub-frame */
#define SEND_TYPE_TERM     0x68u   /* voice terminator-with-LC frame type        */

enum dmr_tx_phase
{
    DMR_IDLE = 0,   /* no call active                        */
    DMR_VOICE,      /* streaming voice super-frames (A..F)   */
    DMR_TERMINATING /* next slot emits the terminator burst  */
};

static struct
{
    enum dmr_tx_phase phase;
    uint8_t  frameSeq;    /* 0..5 -> voice burst A..F              */
    uint8_t  colorCode;   /* 0..15                                 */
    uint8_t  stopReq;     /* dekey requested; terminate at A again */
    uint8_t  lc[9];       /* FLCO, FID, SVCOPT, DST[3], SRC[3]     */
    uint16_t bursts;      /* TX bursts committed since tx_start    */
}
g = { DMR_IDLE, 0, 0, 0, { 0 }, 0 };

/* Stage n bytes into modem TX RAM (byte-addressed; proposal §3.3). */
static void write_modem_ram(uint16_t off, const uint8_t *src, uint16_t n)
{
    for(uint16_t i = 0; i < n; i++)
        MODEM_RAM_TX(off + i) = src[i];
}

/* Commit the staged burst on the upcoming slot (txnextsloten = bit7, clear
 * rxnextsloten = bit6).  This is the per-slot "transmit" write (proposal §3.2). */
static void layer2_tx_next_slot(void)
{
    SOCSYS_LAYER2_TXRX_CTRL = (SOCSYS_LAYER2_TXRX_CTRL & ~0x40u) | 0x80u;
}

/* Release the upcoming slot to RX (rxnextsloten = bit6). */
static void layer2_rx_next_slot(void)
{
    SOCSYS_LAYER2_TXRX_CTRL = (SOCSYS_LAYER2_TXRX_CTRL & ~0x40u) | 0x40u;
}

/* Build the 9-byte (72-bit) LC word (proposal §5.2): FLCO/FID/SVCOPT then the
 * 3-byte big-endian destination and source DMR IDs. */
static void build_lc(const hd2_dmr_call_t *c, uint8_t out[9])
{
    out[0] = c->isPrivate ? DMR_FLCO_PRIVATE : DMR_FLCO_GROUP;
    out[1] = DMR_FID_STANDARD;
    out[2] = 0x00u;                          /* service options */
    out[3] = (uint8_t)(c->dstId >> 16);
    out[4] = (uint8_t)(c->dstId >> 8);
    out[5] = (uint8_t)(c->dstId);
    out[6] = (uint8_t)(c->srcId >> 16);
    out[7] = (uint8_t)(c->srcId >> 8);
    out[8] = (uint8_t)(c->srcId);
}

void hd2_dmr_tx_call_start(const hd2_dmr_call_t *call)
{
    build_lc(call, g.lc);
    g.colorCode = call->colorCode & 0x0fu;
    g.frameSeq  = 0;
    g.stopReq   = 0;
    g.phase     = DMR_VOICE;

    /* Stage the LC word now so the bring-up prime + voice-A burst have it. */
    write_modem_ram(MODEM_RAM_LC_HDR, g.lc, 9);
    SOCSYS_LOCAL_CC = (SOCSYS_LOCAL_CC & ~0x0fu) | g.colorCode;
}

void hd2_dmr_tx_call_stop(void)
{
    /* Finish the current super-frame, then terminate at the next frame-A
     * boundary (the pump flips DMR_VOICE -> DMR_TERMINATING when frameSeq
     * wraps). */
    if(g.phase == DMR_VOICE)
        g.stopReq = 1;
}

void hd2_dmr_set_voice_frame(const uint8_t ambe27[27])
{
    /* Clear-call voice source (proposal §4.3): the modem auto-reads the
     * encoded AMBE from the vocoder<->modem exchange buffer at MODEM_RAM 0x960.
     * The AMBE encoder fills it here, paced by the PCM IRQ -- NOT by the slot
     * pump.  (Relay/encrypted calls instead stage 27 B at MODEM_RAM_VOICE with
     * AUDIO_CONTROL bit0 = 1; not implemented yet.) */
    write_modem_ram(MODEM_RAM_VOCODER, ambe27, 27);
}

void hd2_dmr_tx_slot_pump(void)
{
    switch(g.phase)
    {
        case DMR_IDLE:
        default:
            return;

        case DMR_TERMINATING:
            /* Terminator-with-LC: frame type 0x68, LC word in the control
             * burst at modem-RAM offset 0x00 (proposal §3.4 / §11.2). */
            SOCSYS_LAYER2_SEND_TYPE = SEND_TYPE_TERM;
            write_modem_ram(MODEM_RAM_LC_HDR, g.lc, 9);
            layer2_rx_next_slot();
            g.phase = DMR_IDLE;
            return;

        case DMR_VOICE:
        {
            uint8_t s = g.frameSeq;

            if(s == 0)
            {
                /* Voice frame A: (re)stage the LC word + refresh the color
                 * code into LOCAL_CC (proposal §3.4). */
                write_modem_ram(MODEM_RAM_LC_HDR, g.lc, 9);
                SOCSYS_LOCAL_CC = (SOCSYS_LOCAL_CC & ~0x0fu) | g.colorCode;
            }
            if(s == 5)
            {
                /* Voice frame F: enable embedded-LC delivery in the F burst. */
                SOCSYS_VOICE_EMB_CTRL |= 0x02u;   /* burstf_emb_ctrl */
            }

            SOCSYS_LAYER2_SEND_TYPE = (uint32_t)((s << 4) | SEND_TYPE_VOICE);

            /* Stage the 27-byte VoiceBurst at MODEM_RAM 0x30.  VOICE_PATH bit0=1
             * (TX-RAM source), so the modem reads its 4FSK voice symbols from
             * here.  Until the software AMBE encoder is wired, fill it with a
             * NON-CONSTANT bench pattern so the 4FSK actually develops (a flat/
             * empty burst modulates nothing -> the thin-carrier symptom).  TODO
             * Phase 1: replace with real AMBE frames from hd2_dmr_set_voice_frame. */
            {
                uint8_t vb[27];
                for(unsigned k = 0; k < 27u; k++)
                    vb[k] = (uint8_t)(0x55u ^ (k * 0x1bu) ^ (s << 5));
                write_modem_ram(MODEM_RAM_VOICE, vb, 27);
            }
            layer2_tx_next_slot();
            g.bursts++;

            if(++g.frameSeq > 5)
            {
                g.frameSeq = 0;                   /* roll to the next A..F */
                if(g.stopReq)
                    g.phase = DMR_TERMINATING;
            }
            return;
        }
    }
}

int hd2_dmr_tx_active(void)
{
    return (g.phase != DMR_IDLE);
}

uint16_t hd2_dmr_tx_bursts(void)
{
    return g.bursts;
}

/* ====================================================================== *
 *  DMR TX bring-up / teardown -- the single canonical recipe shared by the
 *  OpMode path (radio_HD2.cpp hd2_dmr_enable_tx) and the bench diag ops
 *  ('L'/'j' in hd2_diag.cpp).  Matched to the clean-room proposal
 *  (§2.1/§3.2/§12) + B-keyup/C-burst-sched, with the two corrections the
 *  earlier hand-tuned diag sequence got wrong:
 *    - WORK_MODE = 0x6a (DMR L2 Tier-II TimeSlot), not 0x6e (§2.1).
 *    - LAYER2_SLOTON |= 0x851 (TX-master: dll_tx_slot_on + bits 4/6/11) is
 *      what STARTS the 30 ms slot engine (§3.2 / C-burst-sched §10); the diag
 *      'j' wrote 0xa0000011 (only bits 0/4) -> engine never clocked (b=0).
 *
 *  MMIO + AT1846S only.  Caller tunes the AT1846S frequency first (OpMode) or
 *  keys at the current tune (diag).  hd2_dmr_tx_call_start() must run before
 *  this to stage the LC for the prime burst.
 * ====================================================================== */

void hd2_dmr_tx_start(uint8_t modAmp)
{
    if(modAmp == 0u) modAmp = 0x19u;       /* vendor default MOD deviation */

    /* ---- Phase A: modem DMR datapath bring-up (modem_rx_mode_setup) ---- *
     * This is what the static carrier path (diag 'L' pre-consolidation) was
     * missing: without the 2-point rf_mode + MOD-bias deviation + clocks the
     * carrier can't spread to 4FSK, and without the slot-engine start it can't
     * clock bursts. */
    SOCSYS_SYS_SOFT_RSTN   = 0x000001fcu;          /* release modem (Phase-A) */
    SOCSYS_LAYER2_CONTROL  = 0u;
    SOCSYS_LAYER2_SLOTON   = 0u;
    SOCSYS_LAYER2_TXRX_CTRL = 0u;
    SOCSYS_REG2C          |= 0x00000700u;          /* modem tx/rx/mc clock enable */
    SOCSYS_ADC_CONTROL     = 0x000041c3u;
    for(unsigned i = 0; i < 14u; i++) SOCSYS_DMR_SYNC(i) = kDmrSync[i];  /* 0x12c..0x160 */
    /* VOICE_PATH: bit1 i2s_slave + bit4 ahb_wr_voice_en + bit0 tx_voice_source=1
     * (TX-RAM source).  bit0=1 makes the modem read each voice burst from
     * MODEM_RAM 0x30 (staged by the slot pump) instead of the vocoder auto-read
     * buffer 0x960 -- because we have no software AMBE encoder filling 0x960
     * yet.  With bit0=0 + no encoder, every burst was empty so only the sync
     * words modulated -> thin carrier (clean-room verification 2026-06-18). */
    SOCSYS_VOICE_PATH      = 0x00000013u;
    SOCSYS_PCM_MODE        = 0x00000003u;          /* AUDIO_BUFFER_CLR: write 3 to flush vocoder bufs (self-clears to 0) */
    SOCSYS_RF_MODE         = 0x034c9060u;          /* two-point modulation */
    SOCSYS_TX_IF_FREQ      = 0x000bb800u;
    SOCSYS_RF_CONTROL      = 0x00041f1au;
    SOCSYS_SIG_CENTER      = 0x6868e7e7u;
    /* RF_MOD_BIAS_CTRL (manual §9.2.2.6): [15:8]=sig_reduce, [7:0]=phase_reduce
     * -- a deviation REDUCTION (0=none .. 255=max), NOT a gain.  modAmp reduces
     * the MOD amplitude; the vendor static value is 0x01e71919 (reduce 0x19). */
    SOCSYS_RF_MOD_BIAS     = 0x01e70000u | ((uint32_t)modAmp << 8) | (uint32_t)modAmp;
    SOCSYS_DEV_LIMITER     = 0x0000ff00u;          /* TX deviation limit (reset default = full deviation) */
    SOCSYS_SLOT_GUARD      = 0x00000014u;
    SOCSYS_WORK_MODE       = WORK_MODE_DMR_L2;      /* 0x6a = DMR L2 Direct/DMO (bit2 is_repeater=0) */
    SOCSYS_LOCAL_CC       |= 0x00000100u;          /* cc_opt: DMR */
    SOCSYS_AF_GATE         = AF_GATE_FM_VENDOR;     /* 0x0001007f */
    /* NOTE: the vendor does NOT set FM_PTT (0x560) for DMR -- its keyed-DMR
     * capture reads 0x560=0 (that gate is the analog-FM modulator path).  DMR
     * routes the C7000 4FSK modulator via WORK_MODE/RF_MODE + DEV_LIMITER, so
     * we leave FM_PTT alone. */

    /* Layer-2 arm + slot-engine start (clean-room M-modem-rxsetup.md).
     *
     * The keyup SLOTON=0x851 is the manual §10.5.2.3 "open the TX slot FROM a
     * temporary reserved slot" path (bits 6 dll_txtmp_open_tx + 11
     * dll_tx_slot_frm_tmp), which REQUIRES "the temporary time slot must
     * already be established" (manual line 756).  We never armed that temp
     * slot -- so o_dll_tx_slot_on (0x404 bit31) never engaged and the counter
     * stayed frozen (HW: on404=0x20000811 -> bit1 dll_tx_slot_on_tmp=0 ->
     * bit30 o_dll_tx_slot_on_tmp=0; bit29 o_dll_rx_slot_on=1 proved the PHY/RX
     * engine + clock were already fine).  tx_master_mode (0xdb bit1) alone was
     * NOT sufficient.
     *
     * Step 1 -- arm the temporary TX slot (modem_rx_mode_setup tail,
     * ELF 0x0305efbc-efea): */
    SOCSYS_LAYER2_CONTROL   = 0x000000c0u;         /* txen+rxen transient        */
    SOCSYS_LAYER2_TXRX_CTRL = 0x00000040u;         /* rxnextsloten baseline      */
    SOCSYS_LAYER2_CONTROL  |= 0x00000010u;         /* bit4 txtmp_master_mode -> 0xd0 */
    SOCSYS_LAYER2_SLOT_CNT  = 0x20000000u;         /* temp-slot counter preload  */
    SOCSYS_LAYER2_SLOTON   |= 0x00000002u;         /* bit1 dll_tx_slot_on_tmp=1 <-- THE FIX */
    /* Wait for the temp slot to go active (o_dll_tx_slot_on_tmp = 0x404 bit30)
     * before the open-from-temp, bounded to ~one superframe. */
    for(unsigned i = 0; i < 40u && (SOCSYS_LAYER2_SLOTON & 0x40000000u) == 0u; i++)
        Thread::sleep(1);

    /* Step 2 -- open the real TX slot FROM the temp slot (modem_rx_dpath_set_a
     * @0x0305ea38, decomp 100989-100992). */
    SOCSYS_LAYER2_CONTROL  = 0x000000dbu;          /* full active TX: txen+rxen+tx_master */
    SOCSYS_LAYER2_SLOT_CNT = (((SOCSYS_LAYER2_STATUS & 0x03ffffffu) >> 18)
                              | 0x200u) << 10;     /* re-preload from live count */
    SOCSYS_LAYER2_TXRX_CTRL = 0x00000040u;
    SOCSYS_LAYER2_SLOTON   = (SOCSYS_LAYER2_SLOTON & ~1u) | 0x851u;  /* open-from-temp */

    /* ---- AT1846S keying (validated bench recipe: 0x30=0x4046 tx_on) ---- *
     * APC DAC-B = vendor DMR "Low", padrv reg 0x0a = 0x4420; reg 0x40 = 0x31
     * (DMR digital mod-path ENABLE, bit0 tx_path_en -- clean-room B-keyup.md,
     * ELF-verified tx_voice_path_enable @0x03058bb0 + radio_transition_rx_to_tx;
     * the modem 4FSK reaches the AT1846S only when this path is enabled.  Was
     * 0x30 (path off) -> the carrier keyed but the modulation stayed narrow). */
    DAC_PD_MODE_EN &= ~0x2u;
    DAC_PD_CTRL    &= ~0x2u;                        /* power up DAC ch B */
    DAC_DATA_B      = 0x00000400u;                  /* APC PA bias (Medium; was 0x106 Low -> faint) */
    hd2_at1846s_write(0x0a, (uint16_t)((0x0cu << 11) | 0x0420u));  /* padrv level 0x0c (was 0x08) */
    hd2_at1846s_write(0x40, 0x0031u);              /* DMR digital mod-path enable */
    hd2_at1846s_write(0x58, 0xbcfdu);              /* DMR filter regs */
    hd2_at1846s_write(0x44, 0x0ad1u);
    hd2_at1846s_write(0x33, 0x45f5u);
    hd2_at1846s_write(0x3a, 0x00c9u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);              /* tx_on (bit6) */

    /* ---- Prime: commit the staged LC-header burst (frame type 0x68); the
     * slot pump then takes over A..F.
     *
     * NOTE (L-slotclock-start.md): we deliberately do NOT do the old "Phase-B"
     * WORK_MODE=2 + LAYER2_CONTROL=0x82 flip -- those drop the modem into
     * physical-layer mode (clearing the L2 TimeSlot bit5 the counter needs and
     * the tx_master_mode bit just set), which would re-break the engine.  Keep
     * WORK_MODE=0x6a and LAYER2_CONTROL=0xdb from the arm. */
    SOCSYS_LAYER2_SEND_TYPE = 0x00000068u;
    SOCSYS_LAYER2_TXRX_CTRL = (SOCSYS_LAYER2_TXRX_CTRL & 0xffffff3fu) | 0x80u;

    g.bursts = 0;
}

void hd2_dmr_tx_stop(void)
{
    /* Dekey the AT1846S, stop the slot engine, and clear the modem L2 datapath
     * so no carrier lingers.  The caller (OpMode) re-establishes RX afterwards
     * via radio_enableRx; the diag wrapper restores rf_freeze. */
    hd2_at1846s_write(0x30, 0x4006u);              /* tx_on off (dekey) */
    DAC_DATA_B = 0u;
    SOCSYS_LAYER2_SLOTON    = 0u;
    SOCSYS_LAYER2_TXRX_CTRL = 0u;
    SOCSYS_LAYER2_CONTROL   = 0u;
    SOCSYS_LAYER2_SLOT_CNT  = 0u;
    SOCSYS_WORK_MODE        = 0u;                   /* leave digital modulator off */
    g.phase  = DMR_IDLE;
    g.bursts = 0;
}

/* ====================================================================== *
 *  M17 physical-layer (Layer-1) 4FSK TX -- raw symbol emit, NO DMR framing.
 *
 *  Clean-room verdict (hd2-clean-v2/m17_phy_output/M17_PHY_VERDICT.md): the
 *  HR_C7000 4FSK modulator is separable from the DMR L2 framer.  WORK_MODE
 *  layermode=0 (physical layer) clocks CPU-staged symbol BYTES out of the
 *  SEND_DATA0/1 ping-pong at the native 38.4 kHz/8 = 4800 sym/s -- no FEC,
 *  interleave, sync insertion or TDMA slot machinery (manual §9.5.1 "TX data
 *  transmitted directly without any encoding process"; §9.5.3 Table 48;
 *  ELF-proven in the vendor RF-align/BER-test path @0x030429e8).  M17 frame_t
 *  bytes are already in this PHY's 2-bit dibit format (M17 byteToSymbols LUT
 *  {+1,+3,-1,-3} MSB-first == manual §9.5.2), so the caller stages them verbatim.
 *
 *  Reuses the proven DMR two-point RF backend (RF_MODE, MOD-bias deviation,
 *  AT1846S digital-mod path) but NOT the L2 slot-engine start (the SLOTON=0x851
 *  temp-slot dance) -- physical-layer mode gates the L2 burst scheduler off.
 * ====================================================================== */
#define MODEM_RAM_SEND_DATA0  0x00u   /* PHY symbol bank A */
#define MODEM_RAM_SEND_DATA1  0x24u   /* PHY symbol bank B */

void hd2_m17_phy_tx_start(uint8_t dev)
{
    /* RF_MOD_BIAS is a deviation REDUCTION (0=none..255=max); M17's deviation is
     * ~1.235x DMR's, so a LOWER reduce than DMR's 0x19 gives more deviation. */
    if(dev == 0u) dev = 0x19u;

    /* Phase A: modem datapath + two-point RF backend (same as hd2_dmr_tx_start,
     * minus the L2 slot-engine start). */
    SOCSYS_SYS_SOFT_RSTN    = 0x000001fcu;          /* release modem */
    SOCSYS_LAYER2_CONTROL   = 0u;
    SOCSYS_LAYER2_SLOTON    = 0u;
    SOCSYS_LAYER2_TXRX_CTRL = 0u;
    SOCSYS_REG2C           |= 0x00000700u;          /* modem tx/rx/mc clock enable */
    SOCSYS_ADC_CONTROL      = 0x000041c3u;
    SOCSYS_VOICE_PATH       = 0x00000013u;
    SOCSYS_PCM_MODE         = 0x00000003u;
    SOCSYS_RF_MODE          = 0x034c9060u;          /* two-point modulation TX */
    SOCSYS_TX_IF_FREQ       = 0x000bb800u;
    SOCSYS_RF_CONTROL       = 0x00041f1au;
    SOCSYS_SIG_CENTER       = 0x6868e7e7u;
    SOCSYS_RF_MOD_BIAS      = 0x01e70000u | ((uint32_t)dev << 8) | (uint32_t)dev;
    SOCSYS_DEV_LIMITER      = 0x0000ff00u;
    SOCSYS_AF_GATE          = AF_GATE_FM_VENDOR;

    /* PHY-L1 arm -- the firmware's own L1-TX sites (wire_cmd03_rf_align,
     * hrc7000_set_tx_tgid) use WORK_MODE=0x02 CONTINUOUS, not timeslot: the
     * modulator streams the SEND_DATA banks directly at 38.4 kHz/8 = 4800 sym/s.
     * (WORK_MODE=0x22 timeslot -- with AND without the SLOTON slot-engine start --
     * both left the engine idle on hardware: strobes=0, tx_bit_cnt static.) */
    SOCSYS_WORK_MODE        = 0x02u;                /* physical-layer, continuous */
    SOCSYS_LAYER2_CONTROL   = 0x82u;                /* txen + tx_master */
    SOCSYS_LAYER2_TXRX_CTRL = 0x81u;                /* txnextsloten + autotest (phys-layer test en) */

    /* AT1846S digital-mod path + key (the proven DMR-TX recipe). */
    DAC_PD_MODE_EN &= ~0x2u;
    DAC_PD_CTRL    &= ~0x2u;
    DAC_DATA_B      = 0x00000400u;                  /* APC PA bias (Medium) */
    hd2_at1846s_write(0x0a, (uint16_t)((0x0cu << 11) | 0x0420u));
    hd2_at1846s_write(0x40, 0x0031u);               /* digital mod-path enable */
    hd2_at1846s_write(0x58, 0xbcfdu);
    hd2_at1846s_write(0x44, 0x0ad1u);
    hd2_at1846s_write(0x33, 0x45f5u);
    hd2_at1846s_write(0x3a, 0x00c9u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);               /* tx_on */
}

void hd2_m17_phy_stage(unsigned bank, const uint8_t *frame36)
{
    write_modem_ram(bank ? MODEM_RAM_SEND_DATA1 : MODEM_RAM_SEND_DATA0,
                    frame36, M17_PHY_FRAME_BYTES);
}

uint32_t hd2_m17_phy_status(void)
{
    return SOCSYS_LAYER2_STATUS;
}

void hd2_m17_phy_tx_stop(void)
{
    hd2_at1846s_write(0x30, 0x4006u);               /* tx_on off (dekey) */
    DAC_DATA_B = 0u;
    SOCSYS_LAYER2_SLOTON    = 0u;
    SOCSYS_LAYER2_TXRX_CTRL = 0u;
    SOCSYS_LAYER2_CONTROL   = 0u;
    SOCSYS_WORK_MODE        = 0u;
}

/* ---------------------------------------------------------------------- *
 *  TS_TX (vec 0x3e) interrupt wiring
 *
 *  The modem raises TS_TX_INTER every 30 ms once the slot engine is running.
 *  The handler acks the modem (SOCSYS_INT_STATUS bit0, mirroring the vendor
 *  isr_ts_tx `int_status |= 1`) and runs the burst pump inline -- the pump is
 *  MMIO-only and ~tens of byte stores, so no thread handoff is needed (the
 *  heavy AMBE encode is paced separately by the PCM IRQ filling 0x960).
 *  Registration via the proven Miosix IRQ API (cf. outputStream_HD2.cpp).
 * ---------------------------------------------------------------------- */

static void dmr_ts_tx_isr(void *)
{
    SOCSYS_INT_STATUS |= INT_STATUS_TS_TX_ACK;   /* ack TS_TX_INTER (bit0) */
    hd2_dmr_tx_slot_pump();
}

void hd2_dmr_irq_enable(void)
{
    GlobalIrqLock lock;
    IRQregisterIrq(lock, HD2_IRQ_DMR_TS_TX, &dmr_ts_tx_isr, nullptr);
}

void hd2_dmr_irq_disable(void)
{
    GlobalIrqLock lock;
    IRQunregisterIrq(lock, HD2_IRQ_DMR_TS_TX, &dmr_ts_tx_isr, nullptr);
}
