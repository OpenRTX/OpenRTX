/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 DMR TX burst scheduler (HR_C7000 Layer-2 modem).
 *
 * Faithful port of the vendor v2.1.3 per-timeslot voice-TX path, derived from
 * the clean-room signal-path proposal (CANONICAL reference:
 * hd2-clean-v2/dmr_research_output/DMR_PATH_PROPOSAL.md §3.4/§4.3/§5.2/§11.2
 * and _findings/C-burst-sched.md).
 *
 * Model: the modem is an autonomous 30 ms TDMA engine.  Once the slot engine
 * is running (LAYER2_SLOTON |= 0x851) the modem raises TS_TX_INTER (vec 0x3e)
 * every 30 ms; the handler calls hd2_dmr_tx_slot_pump(), which stages the next
 * burst's logical (un-FEC'd) fields into modem RAM and commits it via
 * LAYER2_TXRX_CTRL bit7 (txnextsloten).  The modem does FEC / interleave /
 * sync / 4FSK in hardware.
 *
 * Voice source (proposal §4.3): for a CLEAR call the modem auto-reads the
 * encoded AMBE from the vocoder exchange buffer at MODEM_RAM 0x960 (the
 * scheduler does NOT stage the voice payload); the software AMBE encoder fills
 * it via hd2_dmr_set_voice_frame(), paced by the PCM IRQ.  The scheduler only
 * drives per-slot LC, frame-type, color code and the commit.
 *
 * This module is MMIO-only (modem RAM + Layer-2 registers), so the pump is
 * safe to call from interrupt context.
 */
#ifndef DMR_HD2_H
#define DMR_HD2_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * DMR voice-call parameters, built from the channel/contact at key-up.
 */
typedef struct
{
    uint8_t  colorCode;   /**< Color code 0..15.                 */
    uint8_t  timeslot;    /**< 0 = TS1, 1 = TS2.                 */
    uint8_t  isPrivate;   /**< 0 = group call, 1 = private call. */
    uint32_t srcId;       /**< 24-bit own DMR ID (LC source).    */
    uint32_t dstId;       /**< 24-bit destination (TG / radio).  */
}
hd2_dmr_call_t;

/**
 * Begin a DMR voice call: build the LC word, latch the call parameters, and
 * arm the per-slot voice state machine (first burst = voice frame A).  Does
 * NOT start the slot engine or key RF -- the caller (radio driver) owns that.
 *
 * @param call: call parameters; copied internally.
 */
void hd2_dmr_tx_call_start(const hd2_dmr_call_t *call);

/**
 * Request call teardown: the next pumped slot emits the DMR terminator-with-LC
 * burst, after which the state machine returns to idle.
 */
void hd2_dmr_tx_call_stop(void);

/**
 * Supply the next 27-byte encoded AMBE voice block.  For a clear call this is
 * written to the vocoder<->modem exchange buffer (MODEM_RAM 0x960) that the
 * modem auto-reads.  Called by the AMBE encoder path (PCM-IRQ paced), not by
 * the slot pump.
 *
 * @param ambe27: 27 bytes (3x72-bit AMBE sub-frames = one DMR voice burst).
 */
void hd2_dmr_set_voice_frame(const uint8_t ambe27[27]);

/**
 * Per-slot burst pump.  Call once per TS_TX_INTER (vec 0x3e).  Stages and
 * commits the next burst (voice A..F cycling, or the terminator).  A no-op
 * when no call is active.
 */
void hd2_dmr_tx_slot_pump(void);

/**
 * @return non-zero while a DMR call is staging or terminating.
 */
int hd2_dmr_tx_active(void);

/**
 * Bring up the full HR_C7000 DMR-TX datapath and key the AT1846S: modem
 * Phase-A (clocks, 2-point rf_mode, MOD-bias deviation, sync words,
 * slot-engine start LAYER2_SLOTON|=0x851) + AT1846S keying (0x30=0x4046 +
 * APC/padrv) + keyup flip + prime burst.  This is the single canonical recipe
 * (clean-room §2.1/§3.2/§12), shared by the OpMode and the bench diag ops.
 *
 * hd2_dmr_tx_call_start() must run first (stages the LC).  The caller sets the
 * AT1846S TX frequency beforehand (OpMode) or keys the current tune (diag).
 *
 * @param modAmp: MOD1/MOD2 deviation amplitude (0 = vendor default 0x19).
 */
void hd2_dmr_tx_start(uint8_t modAmp);

/**
 * Dekey and tear down the DMR-TX datapath (stop slot engine, clear L2, AT1846S
 * tx_on off).  Caller restores RX afterwards.
 */
void hd2_dmr_tx_stop(void);

/**
 * @return number of TX bursts committed since the last hd2_dmr_tx_start()
 * (0 means the slot engine never clocked -- the key bring-up health metric).
 */
uint16_t hd2_dmr_tx_bursts(void);

/**
 * Register / unregister the TS_TX (vec 0x3e) interrupt that drives the slot
 * pump. Enable once the modem slot engine has been started (keyup); disable on
 * teardown.
 */
void hd2_dmr_irq_enable(void);
void hd2_dmr_irq_disable(void);

/* ---------------------------------------------------------------------- *
 *  M17 physical-layer (Layer-1) 4FSK TX -- raw symbol emit, no DMR framing.
 *
 *  Reuses the proven DMR two-point RF modulator backend but runs WORK_MODE in
 *  physical-layer mode (layermode=0), which clocks CPU-staged symbol BYTES out
 *  of the SEND_DATA0/1 ping-pong at the native 38.4 kHz/8 = 4800 sym/s with no
 *  L2 framer/FEC/sync/TDMA (clean-room verdict m17_phy_output/M17_PHY_VERDICT.md;
 *  manual §9.5.3 Table 48).  M17 frame_t bytes are already in this PHY's 2-bit
 *  dibit format, so the caller stages them verbatim.  Caller sets the AT1846S TX
 *  frequency first.  This is the M17-voice path (vs the dead-end 8 kHz FM pump).
 * ---------------------------------------------------------------------- */
#define M17_PHY_FRAME_BYTES  36u   /* one SEND_DATA bank = 36 B = 144 sym = 30 ms */

/** Arm the PHY-L1 4FSK modulator + key the AT1846S digital-mod path. @param dev
 *  = RF_MOD_BIAS reduce (0 = DMR default 0x19; LOWER = more deviation for M17). */
void hd2_m17_phy_tx_start(uint8_t dev);

/** Stage one 36-byte symbol frame into bank 0 (SEND_DATA0) or 1 (SEND_DATA1). */
void hd2_m17_phy_stage(unsigned bank, const uint8_t *frame36);

/** @return LAYER2_STATUS (0x41c): [31] tx_slot_choose (bank toggle), [8:0]
 *  tx_bit_cnt (engine-liveness counter). Polled to pace the ping-pong refill. */
uint32_t hd2_m17_phy_status(void);

/** Dekey + tear down the PHY-L1 datapath. */
void hd2_m17_phy_tx_stop(void);

#ifdef __cplusplus
}
#endif

#endif /* DMR_HD2_H */
