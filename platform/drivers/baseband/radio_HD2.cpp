/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * OpenRTX radio.h driver for the Ailunce HD2 (HR_C7000 / CSKY V2 ck803s).
 *
 * SCOPE: analog-FM RX control path + FM voice TX (2026-06-11).
 *   - tune (radio_setVcoFrequency-equivalent via radio_enableRx)
 *   - RSSI read (AT1846S reg 0x1B)
 *   - CTCSS/DCS digital squelch (AT1846S reg 0x3A/0x1C)
 *   - opmode / opstatus bookkeeping
 *
 * The RF squelch decision itself lives in the OpenRTX core (OpMode_FM),
 * which turns rtx_getRssi() + sqlLevel into an open/close decision; the
 * driver only has to surface a calibrated RSSI and the tone-squelch flag.
 *
 * Modeled on platform/drivers/baseband/radio_GDx.cpp (the other
 * AT1846S-direct OpenRTX target).  Differences vs GDx:
 *   - No HR_C6000 baseband modem (GDx drives DMR/audio via HR_C6000; on
 *     the HD2 that path is the separate HR_C7000 modem -- DMR deferred).
 *   - No MK22 DAC / APC voltage / PA-drive: TX is intentionally disabled
 *     in this RX-only bring-up, so the TX power chain is a no-op.
 *   - No GDx flash calibration blob: nvm_readCalibData() has no HD2
 *     backend yet (codeplug/W25Q parked, #73).  Squelch / noise / RSSI
 *     thresholds use the AT1846S power-on defaults from at1846s.init()
 *     until a calibration source lands.
 *   - Board-level RX-enable GPIO (vendor at1846s_rx_path_open does
 *     gpio_out_set(0x24) == GPIOB.4) is NOT driven here: the HD2 OpenRTX
 *     GPIO driver only exposes GPIOA/GPIOC today (gpio_csky.h).  RX is
 *     enabled chip-side via AT1846S reg 0x30 bit 5 (setFuncMode(RX)),
 *     which is sufficient for the AT1846S to demodulate; wiring the
 *     GPIOB RX/audio-enable is a TODO for the audio agent.
 *
 * Vendor V2.1.3 references (app image @ 0x0300d000):
 *   - at1846s_rx_path_open   @ 0x030405a0 : reg0x30 |= 0x20 ; gpio_out_set(0x24)
 *   - at1846s_rx_path_close  @ 0x03040564 : reg0x30 &= ~0x20 ; spkr unmute
 *   - rx_squelch_monitor_tick@ 0x03040a6c : reads reg 0x1C bit0 = carrier/sql
 *   - FUN_03040994           @ 0x03040994 : reads reg 0x1B, (v & 0x7FFF) >> 8 = RSSI
 *   - at1846s_set_freq       @ 0x03058e24 : ported in AT1846S_HD2.cpp
 */

#include "interfaces/radio.h"
#include "interfaces/delays.h"
#include "drivers/baseband/AT1846S.h"
#include "drivers/baseband/dmr_HD2.h"
#include "drivers/i2c_csky.h"
#include "radioUtils.h"
#include "hd2_regs.h"
#ifdef HD2_DMR_TX_LIVE
#include "core/state.h"
#include "interfaces/cps_io.h"
#endif

/* Own DMR ID (LC source) for DMR TX.  The vendor Radio-ID table (codeplug
 * 0x4000, active index settings.radio_id @0x29A3) is not wired into the HD2
 * codeplug yet (only the 96-byte settings core is stored), so the source ID
 * comes from this build-time value.  Default = AI5QZ (radioid.net 3209433);
 * override with  cmake -DHD2_DMR_TX_LIVE=ON -DHD2_DMR_DEFAULT_OWN_ID=<id> . */
#ifndef HD2_DMR_DEFAULT_OWN_ID
#define HD2_DMR_DEFAULT_OWN_ID 3209433u
#endif

// Exact-value AT1846S writes for the TX key/dekey sequence (this file (below);
// same helper the proven diag op 'Y' uses).  The AT1846S class API would give
// near-equivalent values via setFuncMode, but TX was brought up and verified
// with these exact words -- keep them bit-identical.
extern "C" void     hd2_at1846s_write(uint8_t reg, uint16_t val);
extern "C" uint16_t hd2_at1846s_read(uint8_t reg);

// AT1846S RX-audio (AF-DSP) listen config -- proven path lives in
// this file (below) (file-static bit-bang helper).  Applied once per RX entry.
extern "C" void hd2_at1846s_rx_audio_config(void);

// ONE-TIME modem RX + baseband-IF-ADC + codec bring-up (this file (below)).
// Called once from radio_init (before any RX); does the SYS_SOFT_RSTN modem
// reset (narrowed to spare the CPU ADC) + codec + modem FM gate config.
extern "C" void hd2_modem_fm_boot_init(void);

// RF-freeze flag (this file (below), loader op 'z').  While set, every
// radio_* entry point that would touch the AT1846S skips its chip I/O so a
// host-side live experiment isn't overwritten.  rtx_task() is already gated
// at the top (hd2_rtx.c); the guards here are defense-in-depth for any other
// caller.  radio_getRssi returns the last cached value while frozen.
extern "C" { extern volatile uint32_t g_rf_freeze; }

static const rtxStatus_t *config;                // Radio configuration pointer
static enum opstatus       radioStatus;          // Current operating status
static Band                currRxBand = BND_NONE; // Current RX band

/*
 * APC TX-power level (CPU DAC channel B, 12-bit; vendor ramps this on
 * every TX entry from a per-band power cal table we don't parse yet).
 * Tunable live between key-ups via the router target RT 16 ('S' op).
 * Conservative default: quarter scale.
 */
static volatile uint16_t apcLevel = 0x400;

extern "C" void     hd2_apc_set(uint16_t level) { apcLevel = level & 0xfff; }
extern "C" uint16_t hd2_apc_get(void)           { return apcLevel; }

/*
 * Map the current codeplug TX power (mW) to the two hardware drive controls:
 * APC DAC level (ch B) and AT1846S reg 0x0a padrv field.  Four discrete levels
 * matching the UI (Extra Low / Low / Medium / High).  Shared so any TX path
 * (radio_enableTx, the APRS diag op) honours the UI-selected level.  Values are
 * conservative -- even "High" sits well below the DAC ceiling (the PA runs hot
 * near full drive).
 */
extern "C" void hd2_txpower_levels(uint16_t *apc, uint16_t *padrv)
{
    const uint32_t p = (config != nullptr) ? config->txPower : 1000u;
    if(p <= 100u)        { *apc = 0x060u; *padrv = 0x08u; }  /* Extra Low */
    else if(p <= 1000u)  { *apc = 0x100u; *padrv = 0x08u; }  /* Low       */
    else if(p <= 2500u)  { *apc = 0x200u; *padrv = 0x0Fu; }  /* Medium    */
    else                 { *apc = 0x400u; *padrv = 0x0Fu; }  /* High      */
}

static AT1846S& at1846s = AT1846S::instance();    // AT1846S driver (singleton)

#if defined(HD2_M17_TXTEST) || defined(HD2_M17_VOICE)
/* M17 PHY bring-up (diag op 'f' / voice op 'g'): force the AT1846S into analog-FM, 25 kHz wide,
 * at a fixed test frequency + VHF/UHF band-select -- so the M17 baseband TX test
 * does not depend on the idle channel (which may be DMR -> wrong freq + narrow
 * filters that mangle 4FSK).  Mirrors radio_enableTx's FM path; the carrier
 * keying + deviation (FM_DEV_COEF) are done by m17_txtest itself. */
extern "C" void hd2_m17_fm_tune(uint32_t hz)
{
    /* NARROW (12.5 kHz) -- M17 is 4FSK @ 4800 sym/s, +/-2.4 kHz outer deviation,
     * essentially DMR-like.  The wide-25k voice config over-deviates the 4FSK
     * (~2x) and m17-demod can't recover the symbols; the DMR/narrow config (which
     * gave the best decode) matches M17 far better. */
    at1846s.setOpMode(AT1846S_OpMode::FM);
    at1846s.setBandwidth(AT1846S_BW::_12P5);
    at1846s.setFrequency(static_cast<freq_t>(hz));
    hd2_at1846s_write(0x59, 0x0C90u);             /* narrow-FM TX deviation (no tone) */
    if(hz >= 300000000u) gpio_atomic_set(&GPIOB_DR, (1u << 19));
    else                 gpio_atomic_clear(&GPIOB_DR, (1u << 19));
}
#endif

void radio_init(const rtxStatus_t *rtxState)
{
    config      = rtxState;
    radioStatus = OFF;

    /*
     * Bring up the AT1846S.  init() runs the full vendor V2.1.3
     * chip-init + VCO calibration dance and leaves the chip configured
     * for the FM (25 kHz) audio bank with RX and TX both off.
     *
     * The I2C transport (GPIOA bit-bang, SCL=PTA7 / SDA=PTA8, slave
     * 0xE2) is initialised lazily by the AT1846S constructor via
     * i2c0_init(); no extra wiring is needed here.
     */
    at1846s.init();

    // ONE-TIME HR_C7000 modem RX + baseband-IF-ADC + codec bring-up, here at init
    // (before any RX, on the rtx thread at startup).  This is the ONLY place the
    // modem SYS_SOFT_RSTN reset + modem FM gate config happen -- doing them per RX
    // entry (radio_enableRx) hung the bus (2026-06-09).  The reset is narrowed to
    // spare the CPU/volume ADC the UI is already using.
    hd2_modem_fm_boot_init();

    // Keep AF output muted until the core opens the squelch.
    radio_disableAfOutput();
}

void radio_terminate()
{
    radioStatus = OFF;
    radio_disableRtx();
}

/* ====================================================================== *
 *  DMR TX (HR_C7000 Layer-2 modem path)
 *
 *  The full bring-up recipe (modem Phase-A datapath + AT1846S keying + slot
 *  engine + burst pump) is the SINGLE canonical implementation in
 *  dmr_HD2.cpp (hd2_dmr_tx_start / _stop / _call_start / _slot_pump), matched
 *  to the clean-room proposal (§2.1/§3.2/§12) and shared verbatim with the
 *  bench diag ops ('L'/'j' in hd2_diag.cpp) so the register sequence lives in
 *  exactly one place.  This file only adds the OpMode glue: frequency tune,
 *  band select, and building the call descriptor from the codeplug.
 *
 *  The RF-emitting steps are still gated behind HD2_DMR_TX_LIVE (a default
 *  build configures the chip for DMR but never keys), because DMR voice audio
 *  needs the unported AMBE encoder feeding MODEM_RAM 0x960.
 * ====================================================================== */

#ifdef HD2_DMR_TX_LIVE
/*
 * Own DMR ID (LC source).  See HD2_DMR_DEFAULT_OWN_ID: the vendor Radio-ID
 * table is not wired into the codeplug yet, so this returns the build-time
 * default.  TODO: read codeplug 0x4000[settings.radio_id] once that sub-block
 * is stored/mapped.
 */
static uint32_t hd2_dmr_own_id(void)
{
    return HD2_DMR_DEFAULT_OWN_ID;
}

/*
 * Build the DMR call descriptor from the active channel + its contact
 * (Phase 4).  Color code and timeslot come from state.channel.dmr; the
 * destination ID and call type (group/private) from the codeplug contact at
 * dmr.contact_index; the source from the radio's own DMR ID.
 */
static void hd2_dmr_build_call(hd2_dmr_call_t *out)
{
    const channel_t *ch = &state.channel;

    out->colorCode = ch->dmr.txColorCode;
    out->timeslot  = (ch->dmr.dmr_timeslot >= 2u) ? 1u : 0u;  /* 1/2 -> 0/1 */
    out->srcId     = hd2_dmr_own_id();

    contact_t ct;
    if(cps_readContact(&ct, ch->dmr.contact_index) == 0)
    {
        out->dstId     = ct.info.dmr.id;
        out->isPrivate = (ct.info.dmr.contactType == PRIVATE) ? 1u : 0u;
    }
    else
    {
        /* No resolvable contact: leave a benign group call to TG 0. */
        out->dstId     = 0u;
        out->isPrivate = 0u;
    }
}
#endif /* HD2_DMR_TX_LIVE */

static void hd2_dmr_enable_tx(void)
{
    if(getBandFromFrequency(config->txFrequency) == BND_NONE)
        return;

    /* No FM +12.5 kHz carrier fudge here: the DMR-mode offset has not been
     * characterised on hardware -- revisit during live bring-up. */
    at1846s.setFrequency(config->txFrequency);

    /* Band-select switch (same PTB19 steering as FM TX). */
    if(config->txFrequency >= 300000000u)
        gpio_atomic_set(&GPIOB_DR, (1u << 19));
    else
        gpio_atomic_clear(&GPIOB_DR, (1u << 19));

#ifdef HD2_DMR_TX_LIVE
    /* Full DMR-TX bring-up via the shared canonical recipe -- the SAME code
     * the bench diag 'L'/'j' ops exercise (dmr_HD2.cpp).  Build the call from
     * the codeplug, stage the LC, bring up the modem + key + start the slot
     * engine, then hook the vec-0x3e ISR to drive the per-slot pump.  (Voice
     * audio still needs the AMBE encoder filling MODEM_RAM 0x960 -- Phase 1.) */
    hd2_dmr_call_t call;
    hd2_dmr_build_call(&call);
    hd2_dmr_tx_call_start(&call);
    hd2_dmr_tx_start(0);                    /* amp = vendor default */
    hd2_dmr_irq_enable();
#endif

    radioStatus = TX;
}

void radio_setOpmode(const enum opmode mode)
{
    if(g_rf_freeze != 0u) return;          /* rf_freeze: no AT1846S writes */

    switch(mode)
    {
        case OPMODE_FM:
            at1846s.setOpMode(AT1846S_OpMode::FM);
            break;

        case OPMODE_DMR:
            /*
             * On channel-select, only program the AT1846S DMR filter bank
             * (harmless, chip-side).  The HR_C7000 modem is left in its
             * FM-RX-capable state and is reconfigured to DMR Layer-2 only at
             * TX keyup (hd2_dmr_enable_tx -> hd2_modem_dmr_setup); doing it
             * here wedged boot when a DMR channel was the power-on channel.
             */
            at1846s.setOpMode(AT1846S_OpMode::DMR);
            at1846s.setBandwidth(AT1846S_BW::_12P5);
            break;

        case OPMODE_FM_BCAST:
            /* Broadcast FM uses the separate RDA5802E tuner, not the AT1846S.
             * Leave the transceiver as-is (idle); OpMode_FMBroadcast mutes the
             * AT1846S AF on the shared analog node via the tuner HAL. */
            break;

        default:
            break;
    }
}

bool radio_checkRxDigitalSquelch()
{
    // CTCSS/DCS tone detection via AT1846S reg 0x3A/0x1C.
    if(config->rxToneType == TONE_CTCSS)
        return at1846s.rxCtcssDetected();
    return at1846s.rxCdcssDetected(config->rxToneType == TONE_DCS_I);
}

void radio_enableAfOutput()
{
    /*
     * Unmute the AT1846S RX audio output (reg 0x30 bit 7).  NOTE: the
     * actual speaker amp / audio routing on the HD2 is a board-level
     * concern owned by the audio agent (vendor spkr_amp_unmute +
     * gpio_out_set(0x24)); here we only release the chip-side mute.
     */
    if(g_rf_freeze != 0u) return;          /* rf_freeze: no AT1846S writes */
    at1846s.unmuteRxOutput();
}

void radio_disableAfOutput()
{
    if(g_rf_freeze != 0u) return;          /* rf_freeze: no AT1846S writes */
    at1846s.muteRxOutput();
}

void radio_enableRx()
{
    if(g_rf_freeze != 0u)                  /* rf_freeze: skip tune + AF-DSP */
        return;

    if(currRxBand == BND_NONE)
        return;

    // Tune the AT1846S VCO to the RX frequency and enable the RX path.
    at1846s.setFrequency(config->rxFrequency);
    at1846s.setFuncMode(AT1846S_FuncMode::RX);

    // Enable RX-side CTCSS/DCS detection if the channel requests it.
    if(config->rxToneEn)
    {
        if(config->rxToneType == TONE_CTCSS)
            at1846s.enableRxCtcss(config->rxTone);
        else
            at1846s.enableRxCdcss(config->rxTone, config->rxToneType == TONE_DCS_I);
    }

    // Apply the AT1846S RX-audio (AF-DSP) config ONCE here -- the heavy I2C
    // burst belongs at RX entry, not on every squelch crossing.  Then release
    // the chip-side AF mute (reg 0x30 bit7) so the board speaker-amp (PTB4),
    // toggled by the audio matrix, is the only per-squelch gate.
    hd2_at1846s_rx_audio_config();
    at1846s.unmuteRxOutput();

    // The HR_C7000 modem RX + codec FM bring-up is done ONCE in radio_init
    // (hd2_modem_fm_boot_init), NOT here -- per-RX-entry modem writes/resets hung the
    // bus (2026-06-09).  radio_enableRx is AT1846S-only: tune + AF-DSP config + chip
    // unmute; the board speaker-amp (PTB4) is the per-squelch gate.

    radioStatus = RX;
}

/* ---------------------------------------------------------------------- *
 *  Canonical analog-FM TX key / unkey -- the SINGLE register recipe.
 *
 *  Shared by radio_enableTx (the OpMode_FM / OpMode_M17 path) and the M17/FM
 *  baseband-pump bench diag op ('g' in hd2_diag.cpp) so the keying sequence
 *  lives in exactly ONE place -- the same one-recipe pattern DMR TX uses
 *  (hd2_dmr_tx_start).  A bench op that hand-rolled its own keying both drifts
 *  from what ships and is a prime fidelity-bug suspect (M17 LICH-no-LSF).
 *
 *  NOT gated on g_rf_freeze: the caller owns bus arbitration.  radio_enableTx
 *  checks the freeze (and txDisable / opMode / band) before calling; the diag
 *  op sets g_rf_freeze=1 to quiesce the rtx thread, then calls this directly
 *  (exactly as the DMR bench ops call hd2_dmr_tx_start).
 *
 *  Does NOT select the modulator source: the OpMode path leaves the FM engine
 *  on the mic ADC (mic -> codec ADC -> C7000 FM modulator -> MOD1/MOD2 ->
 *  AT1846S varactors); the M17 pump instead sets AUDIO_CONTROL tx_voice_source
 *  = TX-RAM itself (its SINK_RTX layer).  `narrow` picks the reg-0x59 deviation
 *  bank (true = 12.5 kHz).  Do NOT touch SIG_CENTER/RF_MOD_BIAS -- the boot cal
 *  modulates cleanest (mid-scale guesses audibly degraded the audio).
 * ---------------------------------------------------------------------- */
extern "C" void hd2_fm_tx_key(uint32_t txFreq, bool narrow, uint8_t txToneEn,
                              uint8_t txToneType, uint16_t txTone, uint8_t tailElim)
{
    // Tune the VCO to the TX frequency -- RAW (2026-06-18).  The old +12.5 kHz
    // FM-TX "fudge" is REMOVED: the DMR carrier keys ~on-frequency, so the synth
    // is accurate and a hardcoded per-path shift is the wrong model.  Residual
    // error becomes a proper per-band/per-radio CALIBRATION (shared FM + DMR)
    // once nvm_readCalibData has an HD2 backend -- not a magic constant here.
    at1846s.setFrequency(txFreq);

    // Arm the C7000 FM-TX engine.  SYS_INTERP_MASK must hold the vendor FM value:
    // with the boot 0x7f mask the engine wedges on its first unserviced
    // FM_TX_INTERP and the carrier stays dead-quiet.  Masking (bit16) or acking
    // 0x3b0 works; we mask (no service thread).
    SOCSYS_AF_GATE    = AF_GATE_FM_VENDOR;
    SOCSYS_WORK_MODE |= WORK_MODE_FM_MOD;
    SOCSYS_FM_PTT     = 1u;

    // Band-select switch (PTB19, from the vendor spkr/mic_path_set_by_freq: high
    // band -> set, low band -> clear).  Steers the RF path / PA chain.
    if(txFreq >= 300000000u)
        gpio_atomic_set(&GPIOB_DR, (1u << 19));
    else
        gpio_atomic_clear(&GPIOB_DR, (1u << 19));

    // TX power level -> APC DAC drive (ch B) + AT1846S reg 0x0a padrv.  txPower
    // selects one of four discrete UI levels; APC is the PA bias the vendor ramps
    // each PTT edge, padrv is reg 0x0a bits 14:11 (with 0x420 = pga_gain 0x10 +
    // pabias 0).  Conservative -- even "High" sits below the DAC ceiling (0xfff).
    uint16_t apc, padrv;
    hd2_txpower_levels(&apc, &padrv);          // map current txPower -> APC + padrv
    apcLevel = apc;                            // (apcLevel is volatile; copy via temp)

    DAC_PD_MODE_EN &= ~0x2u;                   // ch B low-power mode off
    DAC_PD_CTRL    &= ~0x2u;                   // ch B power up
    DAC_DATA_B      = apcLevel;

    // Vendor TX parity: mic-path gate PTB3 high (vendor mic_path_set_by_freq).
    gpio_atomic_set(&GPIOB_DR, (1u << 3));

    hd2_at1846s_write(0x0a, (uint16_t)((padrv << 11) | 0x0420u));

    // TX FM deviation (reg 0x59 -- shared with the RX mixer gain, restored in
    // radio_disableRtx() / hd2_fm_tx_unkey).  Pick by bandwidth and whether a
    // sub-audio tone is active (the low 6 bits set the CTCSS/DCS deviation).
    {
        uint16_t dev59;
        if(narrow)
            dev59 = txToneEn ? 0x0B11u : 0x0C90u;
        else
            dev59 = txToneEn ? 0x0C62u : 0x0C50u;
        hd2_at1846s_write(0x59, dev59);
    }

    // AT1846S: TX-side AF-DSP ctrl, then key the carrier (exact words from the
    // verified bring-up), then the vendor tx_pa_enable (reg 0x30 |= 0x80 -- the
    // datasheet calls bit7 "mute", but the vendor sets it on every key-up and
    // clears it on every dekey: it is the PA-stage gate in TX context).
    hd2_at1846s_write(0x40, 0x0030u);
    hd2_at1846s_write(0x30, 0x4006u);
    hd2_at1846s_write(0x30, 0x4046u);          // tx_on
    hd2_at1846s_write(0x30, 0x40c6u);          // + bit7: PA on (vendor tx_pa_enable)

    // Sub-audio encode: the AT1846S sums its own CTCSS/CDCSS generator into the
    // TX modulation; disableCtcss/disableCdcss() in radio_disableRtx() clears it.
    // Tail elimination (reverse burst) is armed here and fired on dekey.
    if(txToneEn)
    {
        if(txToneType == TONE_CTCSS)
            at1846s.enableTxCtcss(txTone);
        else
            at1846s.enableTxCdcss(txTone, txToneType == TONE_DCS_I);
        if(tailElim)
            at1846s.setTxTailShift(AT1846S::TAIL_180);
    }
}

extern "C" void hd2_fm_tx_unkey(void)
{
    DAC_DATA_B   = 0u;                     // APC drive to zero first
    hd2_at1846s_write(0x30, 0x4046u);      // PA off (bit7 clear) first...
    hd2_at1846s_write(0x30, 0x4006u);      // ...then tx_on off (dekey)
    hd2_at1846s_write(0x0a, 0x4c20u);      // restore padrv to chip-init default
    DAC_PD_CTRL |= 0x2u;                   // APC DAC ch B power down
    gpio_atomic_clear(&GPIOB_DR, (1u << 3));   // mic gate off
    SOCSYS_FM_PTT     = 0u;
    SOCSYS_WORK_MODE &= ~WORK_MODE_FM_MOD;
    hd2_at1846s_write(0x40, 0x0031u);      // RX-side AF-DSP ctrl value
    hd2_at1846s_write(0x59, 0x0b90u);      // restore RX mixer gain (TX set 0x59 deviation)
}

void radio_enableTx()
{
    /*
     * FM voice TX -- HW-verified 2026-06-11 (voice received on a second
     * radio; brought up via diag op 'Y', see docs/audio_paths.md §5).
     *
     * Architecture: mic -> codec ADC -> C7000 FM modulator engine ->
     * MOD1/MOD2 pins (two-point modulation) -> AT1846S varactors.  The
     * AT1846S only keys the carrier (its FM bank parks voice_sel=000);
     * the mic feed into the engine is pure hardware -- no CPU sample
     * pumping.  Do NOT touch SIG_CENTER/RF_MOD_BIAS: the boot/reset cal
     * modulates cleanest (mid-scale guesses audibly degraded the audio,
     * and readback shows the RX bank, so values can't be verified).
     */
    if(g_rf_freeze != 0u)
        return;

    if(config->txDisable == 1)
        return;

    // DMR TX uses the HR_C7000 Layer-2 modem burst path, not the analog-FM
    // modulator engine -- handled by its own (scaffold) sequence.
    if(config->opMode == OPMODE_DMR)
    {
        hd2_dmr_enable_tx();
        return;
    }

    // Everything below is the analog-FM voice TX path.
    if(config->opMode != OPMODE_FM)
        return;

    // AT1846S hardware band sanity (134-174 / 400-527 MHz).
    if(getBandFromFrequency(config->txFrequency) == BND_NONE)
        return;

    // Key the analog-FM TX path via the shared canonical recipe (the SINGLE
    // register sequence -- see hd2_fm_tx_key).  This is the exact code the M17/FM
    // baseband-pump bench diag op keys, so the recipe lives in one place.
    hd2_fm_tx_key(config->txFrequency, config->bandwidth == BW_12_5,
                  config->txToneEn, config->txToneType, config->txTone,
                  config->tailElim);

    radioStatus = TX;
}

void radio_disableRtx()
{
    if(g_rf_freeze != 0u)                  /* rf_freeze: no AT1846S writes */
    {
        radioStatus = OFF;                 /* keep the bookkeeping honest  */
        return;
    }

    /*
     * TX teardown first (no-ops when not transmitting): dekey the AT1846S
     * carrier, then stop the C7000 FM-TX engine and drop back to the
     * RX-side work_mode.  Order mirrors the verified bring-up: carrier
     * off before the modulator so we don't radiate an unmodulated blip.
     */
    // DMR TX teardown (scaffold): stop the slot engine and dekey the chip.
    // Kept separate from the FM teardown below, which restores FM-modulator
    // and AF-DSP register state that DMR never touched.
    if((radioStatus == TX) && (config->opMode == OPMODE_DMR))
    {
#ifdef HD2_DMR_TX_LIVE
        hd2_dmr_irq_disable();                 // unhook the TS_TX slot pump
        hd2_dmr_tx_stop();                     // dekey + stop engine + clear L2
#endif
        at1846s.setFuncMode(AT1846S_FuncMode::OFF);
        radioStatus = OFF;
        return;
    }

    if(radioStatus == TX)
        hd2_fm_tx_unkey();                     // shared canonical FM-TX teardown

    // Drop both RX and TX on the chip side and silence any tone output.
    at1846s.disableTone();
    at1846s.disableCtcss();
    at1846s.disableCdcss();
    at1846s.setFuncMode(AT1846S_FuncMode::OFF);
    radioStatus = OFF;
}

void radio_updateConfiguration()
{
    if(g_rf_freeze != 0u)                  /* rf_freeze: no AT1846S writes */
        return;

    currRxBand = getBandFromFrequency(config->rxFrequency);

    if(currRxBand == BND_NONE)
        return;

    /*
     * Analog-FM bandwidth select.  No flash calibration backend yet
     * (#73), so the per-band noise/RSSI/squelch threshold tuning that
     * GDx pulls from calData is left at the AT1846S init defaults.  Once
     * an HD2 calibration source exists, mirror radio_GDx.cpp's
     * setNoise1/2Thresholds + setRssiThresholds + setAnalogSqlThresh.
     */
    if(config->opMode == OPMODE_FM)
    {
        switch(config->bandwidth)
        {
            case BW_12_5:
                at1846s.setBandwidth(AT1846S_BW::_12P5);
                break;

            case BW_25:
                at1846s.setBandwidth(AT1846S_BW::_25);
                break;

            default:
                break;
        }

        // Drive the AT1846S RSSI squelch threshold (reg 0x49) from the
        // squelch level.  Applied AFTER setBandwidth (0x49 is not bank-managed,
        // so it survives the band switch).  The software RSSI window in
        // hd2_rtx.c remains the audio-gate authority (de-thrash hysteresis).
        at1846s.setSquelchLevel(config->sqlLevel);
    }

    /*
     * Re-apply the VCO frequency / RX path if we are already receiving,
     * so a config change (e.g. a new RX frequency) takes effect without
     * the core having to cycle the op-status.  Mirrors radio_GDx.cpp.
     */
    if(radioStatus == RX)
        radio_enableRx();
}

rssi_t radio_getRssi()
{
    /*
     * AT1846S reg 0x1B upper byte holds the RSSI level; the generic
     * driver maps it to dBm as (-137 + (reg >> 8)).  Vendor V2.1.3
     * FUN_03040994 reads the same register ((v & 0x7FFF) >> 8) for its
     * 5-level signal-bar mapping, confirming the field location.
     *
     * rf_freeze: even a *read* is bit-bang bus traffic that can corrupt a
     * concurrent host transaction, so return the last cached value while
     * frozen (the S-meter just holds still).
     *
     * Peak-hold (instant rise, slow fall ~2 dB/call @ ~33 Hz): with NO carrier
     * the chip refreshes reg-0x1B rssi_db only periodically and reads near-zero
     * between updates (HW 2026-06-14), so a plain read flickers the S-meter to
     * 0.  Peak-hold latches the periodic true value and ignores the stale dips,
     * while a real signal drop still falls.  Done here (not the rtx task) so the
     * de-jitter survives the OpMode_FM convergence (hd2_rtx.c -> rtx.cpp).
     */
    static rssi_t held = -127;

    if(g_rf_freeze == 0u)
    {
        rssi_t raw = static_cast<rssi_t>(at1846s.readRSSI());
        if(raw >= held)             held = raw;
        else if((held - raw) > 2)   held = static_cast<rssi_t>(held - 2);
        else                        held = raw;
    }

    return held;
}

enum opstatus radio_getStatus()
{
    return radioStatus;
}

/* ============================================================== *
 *  AT1846S live register access + analog-FM audio/modem bring-up *
 *  (merged from the former radio_test_HD2.cpp, 2026-06-13).  The   *
 *  hd2_* externs declared at the top of this file are defined     *
 *  here: live reg read/write, RX-audio config, the one-time modem *
 *  FM boot init, the RSSI-sweep selftest, and the g_fm_ / g_rf_   *
 *  host-pokeable globals.                                         *
 * ============================================================== */
/* AT1846S I2C slave address (== 0x71 << 1), matches AT1846S_HD2.cpp. */
static constexpr uint8_t AT1846S_ADDR = 0xE2;

/*
 * GPIOB (DW_apb_gpio, base 0x14100000) FM RX audio gates -- GPIOB_DR/DDR +
 * SPKR_AMP_BIT (GPIOB.4) + AUDIO_ROUTE_BIT (GPIOB.10) come from hd2_regs.h
 * (from vendor spkr_amp_unmute 0x03040fa4 + audio_route_rx_unmute 0x03041ec0):
 *   GPIOB.4  (pin 0x24) = speaker-amp MUTE, active-LOW: LOW = playing.
 *   GPIOB.10 (pin 0x2a) = RX audio route (default path), clear = routed.
 */
#define SPKR_MUTE_BIT  SPKR_AMP_BIT

/*
 * Audio on/off, host-pokeable via 'W'.  1 (default) = run the FM-RX audio
 * enable (unmute amp + route + AT1846S reg40); 0 = leave speaker muted
 * (RSSI-only sweep, as before).
 */
extern "C" { volatile uint32_t g_fm_audio_on = 1u; }

/* RX audio volume (AT1846S reg 0x44 low byte; reg = 0x900 | vol).  Vendor
 * sources this from g_tune_data[2]; our loader has no volume setting, so it
 * defaults near-max and is host-pokeable via 'W' for a live sweep. */
extern "C" { volatile uint32_t g_fm_volume = 0x3fu; }

/* Diagnostic: OR of every modem RX IRQ-latch (0x11000398) value seen during the
 * servicing loop. Host reads it after 'a' to tell if the modem RX FSM is alive
 * (value changes from idle 0x4000) or clock-dead. */
extern "C" { volatile uint32_t g_modem_irq_acc = 0u; }

/*
 * RF FREEZE (loader op 'z').  While 1, ALL firmware-initiated AT1846S I2C
 * traffic and audio-GPIO/diplex rewrites are suspended so a host can run live
 * chip experiments over 'q'/'Q'/'S' without the firmware overwriting them
 * within seconds.  Gated call sites (threads keep running, they just skip
 * chip I/O):
 *   - hd2_rtx.c rtx_task()            (33 Hz RSSI poll + squelch audio gate
 *                                      + reconfigure -> radio_* AT1846S writes)
 *   - radio_HD2.cpp radio_enableRx / radio_disableRtx / radio_setOpmode /
 *     radio_updateConfiguration / radio_enableAfOutput / radio_disableAfOutput
 *     / radio_getRssi (returns last cached value while frozen)
 *   - audio_HD2.c audio_connect / audio_disconnect  (PTB4/PTB10 amp+route)
 *   - platform.c platform_beepStart / platform_beepStop  (PTB4/PTB10 +
 *     DIPLEX0 bit18 PWM-audio mute; beepStop still stops the PWM channel)
 *   - hd2_fm_broadcast.cpp fmThread       (broadcast-tuner I2C on the SAME
 *                                      GPIOA bit-bang bus + PTB10/PTB20)
 * Host-initiated diag ops (q/Q/S/I/o/m/u) are intentionally NOT gated.
 */
extern "C" { volatile uint32_t g_rf_freeze = 0u; }

/*
 * HOST-TUNABLE sweep frequency, in Hz.  Poke via the loader 'W' command at
 * this global's .map address, then issue 'a' to tune + read RSSI.  Default
 * 103.600 MHz (a strong broadcast station found this session) so the audio
 * test lands on a real signal.  For broadcast FM, sweep ~87.5e6 .. 108e6.
 */
extern "C" { volatile uint32_t g_fm_test_freq = 103600000u; }

/* Re-init only once; sweep points after the first are fast retunes. */
static bool g_radio_test_inited = false;

/* Coarse settle delay (~ms) so the RSSI detector tracks the new tune. */
static void rssi_settle(void)
{
    for (volatile uint32_t i = 0; i < 200000u; ++i) { }
}

/*
 * Read one 16-bit AT1846S register over the GPIOA bit-bang bus, using the
 * exact same primitives + big-endian byte order as AT1846S::i2c_readReg16.
 */
static uint16_t at1846s_read_reg(uint8_t reg)
{
    uint16_t value = 0;

    i2c0_lockDeviceBlocking();
    i2c0_write(AT1846S_ADDR, &reg, 1, false);
    i2c0_read(AT1846S_ADDR, &value, 2);
    i2c0_releaseDevice();

    /* AT1846S sends register data big-endian on the wire. */
    return __builtin_bswap16(value);
}

/* Write one 16-bit AT1846S register (reg, then value big-endian), same wire
 * shape as AT1846S::i2c_writeReg16. */
static void at1846s_write_reg(uint8_t reg, uint16_t value)
{
    uint8_t buf[3] = { reg, (uint8_t)(value >> 8), (uint8_t)(value & 0xff) };
    i2c0_lockDeviceBlocking();
    i2c0_write(AT1846S_ADDR, buf, 3, true);
    i2c0_releaseDevice();
}

/*
 * HR_C7000 PCM/audio-codec block (byte-addressed MMIO @ 0x16000900, verified
 * alive + readable on hardware).  Port of boot_init_modem_audio_path
 * (0x0305e62c) + the analog-FM tail of boot_init_audio_route (0x0305e7d8).
 * MUST use byte writes -- the block ignores partial word writes.
 */
/* CODEC_BASE + CODEC_BYTE(off) come from hd2_regs.h (byte-addressed PCM
 * block).  Keep the short local CB() name this file already uses. */
#define CB(off)      CODEC_BYTE(off)
#define G_TUNE_DATA  (*(volatile uint8_t  *)0x00043974u)

/* Codec DAC output gain (GCR_DACL @ codec 0xdf): vendor sources the low 6 bits
 * (godl) from codeplug calibration g_tune_data[2] | 0x80.  Our loader has no
 * codeplug, so G_TUNE_DATA reads garbage (0x55 -> 0xd5) -- a WRONG DAC gain
 * that attenuates the codec output (live-diffed 2026-06-02: vendor playing FM
 * had 0xdf=0xb4=0x34|0x80, ours 0xd5).  Use the vendor's known-good value so
 * the codec DAC isn't mis-gained.  See [[project_hd2_fm_rx_tune_verified]]. */
#define CODEC_DACL_GAIN  0xb4u   /* = g_tune_data 0x34 | 0x80 (vendor FM) */

/* HR_C7000 socsys audio/codec control registers (base 0x11000000) all come
 * from hd2_regs.h (SOCSYS_SYS_SOFT_RSTN, SOCSYS_DAC_CONTROL, SOCSYS_ADC_CONTROL,
 * SOCSYS_VOICE_PATH, SOCSYS_PCM_MODE, SOCSYS_LINEOUT_CTRL, SOCSYS_WORK_MODE,
 * SOCSYS_AF_GATE, SOCSYS_MODEM_RXDP0/1, SOCSYS_MODEM_IRQ/_ACK).  Offsets
 * verified live against the vendor while it played FM 107.5 -- see
 * tmp/vendor_fm_playing_state.md. */
#define SYS_SOFT_RSTN       SOCSYS_SYS_SOFT_RSTN
/* 0x088 standby_lo (bit31) doubles as the codec-DAC-enable handshake flag used
 * during bring-up; the codec init below waits on it under this same name. */
#define PCM_HANDSHAKE       SOCSYS_LINEOUT_CTRL

static void delay_ms(unsigned ms)
{
    for (unsigned m = 0; m < ms; ++m)
        for (volatile unsigned i = 0; i < 8000u; ++i) { }
}

static bool g_codec_inited = false;

static void hd2_codec_audio_init(void)
{
    /* PCM block reset pulse + codec soft-reset (matches vendor exactly). */
    CB(0xd2) |= 0x03;
    SYS_SOFT_RSTN &= 0xffffffefu;      /* clear bit4 = codec reset */
    delay_ms(2);
    CB(0xd2) &= ~0x01;
    delay_ms(100);
    CB(0xd2) &= ~0x02;
    CB(0xd3) = 0x40;
    CB(0xcb) = 0x00;
    CB(0xcc) |= 0x40;
    CB(0xc9) = 0xc0;
    CB(0xcf) &= ~0x10;
    delay_ms(100);
    CB(0xcf) &= ~0x80;
    CB(0xc8) = 0xc0;
    CB(0xcd) &= ~0x10;

    /* Wait for the PCM handshake (socsys 0x88 bit31 clear), bounded. */
    for (uint32_t g = 0; g < 200u; ++g) {
        if ((PCM_HANDSHAKE & 0x80000000u) == 0u) break;
        delay_ms(10);
    }

    CB(0xcd) &= ~0x80;
    delay_ms(100);
    CB(0xdf) = CODEC_DACL_GAIN;
    CB(0xe5) = 0x8b;
    PCM_HANDSHAKE = 1;

    /* boot_init_audio_route analog tail (same block). */
    CB(0xc8) = 0xc0;
    CB(0xcd) = 0x20;
    CB(0xe5) = 0x8b;
    CB(0xdf) = CODEC_DACL_GAIN;
    SOCSYS_PCM_MODE |= 0x02;           /* _hrc7000_pcm_mode |= 2 */
}

/*
 * Warm up the HR_C7000 codec audio-OUTPUT path ONCE: codec bring-up + the socsys
 * audio gate, with NO AT1846S RF tune and NO modem-RX servicing loop.  On the
 * OpenRTX bench unit the PWM-ch1 beep mixes THROUGH the codec (DAC -> Mercury PWM
 * lineout -> speaker), so the codec must be initialised before a beep is audible
 * -- proven 2026-06-01: beeps were silent until 'a' ran, then rang clearly.  This
 * is the minimal subset of 'a' needed for that, minus the 3 s servicing loop that
 * wedges the loader.  Called lazily from platform_beepStart.  GPIOB.4/.10 (amp +
 * route) are left to platform_beepStart so they re-assert per beep. */
extern "C" void hd2_audio_out_warm(void)
{
    static bool warmed = false;
    if (warmed) return;
    if (!g_codec_inited) { hd2_codec_audio_init(); g_codec_inited = true; }
    SOCSYS_DAC_CONTROL  = 0x8000001fu;
    SOCSYS_ADC_CONTROL  = 0x000041c3u;
    SOCSYS_VOICE_PATH   = 0x00000002u;
    SOCSYS_PCM_MODE     = 0x00000000u;
    SOCSYS_LINEOUT_CTRL = 0x00000003u;   /* both lineouts; speaker is on LINE2OUT (see FM path) */
    SOCSYS_WORK_MODE    = 0x0000006eu;
    SOCSYS_AF_GATE      = 0x0001007fu;
    SOCSYS_MODEM_RXDP0  = 0x000000c4u;
    SOCSYS_MODEM_RXDP1  = 0x00000040u;
    warmed = true;
}

/*
 * AT1846S RX-audio (AF-DSP) config -- the chip-side half of the proven
 * 'a'-command listen sequence, WITHOUT the codec/socsys gate and WITHOUT any
 * board GPIO.  Call ONCE per RX entry (radio_enableRx); it's a ~12-write I2C
 * burst + a setBandwidth filter-bank load, far too heavy to run on every
 * squelch crossing (that wedged the bit-bang bus -- task: squelch lockup).
 *
 * Sets reg 0x40=0x30 (AF-DSP LISTEN, not 0x11 scan-mute), apply_rx_audio_config
 * (filter/AGC/output + VOLUME via g_fm_volume), then reg 0x7a + the FM filter
 * bank (else the DMR bank runs on FM -> no demod audio).  Uses the file-static
 * at1846s_write_reg helper (the proven bit-bang path).  No codec: the analog
 * AF reaches the speaker over the same route the broadcast tuner uses.
 *
 * KEEP IN SYNC with the AT1846S writes in hd2_radio_selftest's g_fm_audio_on
 * block below (the live-verified reference).
 */
extern "C" void hd2_at1846s_rx_audio_config(void)
{
    at1846s_write_reg(0x40, 0x0030);         /* RX AF-DSP LISTEN (NOT 0x11 scan-mute) */

    at1846s_write_reg(0x41, 0x471e);
    at1846s_write_reg(0x44, 0x0900u | (uint16_t)(g_fm_volume & 0xff));
    at1846s_write_reg(0x33, 0x44a5);
    at1846s_write_reg(0x54, 0x2a3c);
    at1846s_write_reg(0x63, 0x16ad);
    at1846s_write_reg(0x58, 0x8405);
    at1846s_write_reg(0x4e, 0x6002);

    at1846s_write_reg(0x7a, 0xa00a);
    AT1846S::instance().setBandwidth(AT1846S_BW::_25);   /* FM bank 0 (vendor-confirmed) */

    /* 0x3a (voice-channel/audio-path select) MUST come AFTER setBandwidth: the
     * FM bank table includes 0x3a=0x00c3 and silently clobbers the vendor FM-RX
     * value (live readback 2026-06-10: our radio held 0x00c3, vendor uses 0x80e1).
     * NOTE: re-poking 0x80e1 live did NOT restore audio by itself -- kept for
     * vendor-state parity, not as the audio fix. */
    at1846s_write_reg(0x3a, 0x80e1);

    /* Vendor FM-RX reg-0x30 enable (RE: rf_apply_channel_to_pll FM branch does
     * 0x4806 then 0x4826).  setFuncMode left it at 0x4026 -- missing bit11 (0x800),
     * the RX-side path bit.  Persist it here (a live poke got overwritten on RX
     * re-entry).  0x4826 = band(0x4000)+RX-side(0x800)+RX_ON(0x20)+0x06. */
    at1846s_write_reg(0x30, 0x4806);
    at1846s_write_reg(0x30, 0x4826);
    /* NOTE: reverted the SA8x8 voice-channel (0x3A|=0x4000) + 0x57 pokes -- the vendor
     * HD2 FM path never does them (RE 2026-06-09). */
}

/*
 * Complete analog-FM RX audio bring-up on the HR_C7000 side: codec DAC power-up
 * (once) + the modem analog-FM audio gate (socsys).  This is what lets the
 * AT1846S-IF FM demod reach the codec DAC -> lineout -> speaker.  Call once per
 * RX entry (radio_enableRx), AFTER hd2_at1846s_rx_audio_config().
 *
 * EVERY value here was verified LIVE against a vendor radio playing FM out the
 * speaker, by word-diffing its socsys/codec block over dbgshell (2026-06-09):
 *   work_mode 0x6e, af_gate 0x1007f, rf_mode(0x104) 0x034c9060, codec DAC out of
 *   standby with gain 0xb4.  Our threaded build otherwise leaves the codec in
 *   sleep/standby (CR_VIC=0x0f, CR_DAC=0xb0, GCR_DACL=0x00) -> only amp noise,
 *   no demod audio; and it NEVER programmed rf_mode (the baseband RF-interface
 *   mode) so the modem had no path to route the demod onto the DAC.
 *
 * Codec init is latched (g_codec_inited) -> runs once.  The socsys gate is set
 * unconditionally every RX entry (it must NOT depend on the hd2_audio_out_warm
 * 'warmed' latch, which a boot beep can consume first, leaving the FM gate
 * unset).  Codec config goes through the file-static hd2_codec_audio_init (the
 * proven MC-interface access; the codec block can't be poked from a cold host).
 */
extern "C" void hd2_modem_fm_boot_init(void)
{
    /* ONE-TIME modem RX + baseband-IF-ADC + codec bring-up.  Call ONCE from
     * radio_init (rtx thread startup), BEFORE any RX -- the modem isn't in use yet,
     * so resetting it can't hang a mid-flight AHB transaction, and the config is
     * written once (NOT per RX entry, which hung the bus, 2026-06-09).
     *
     * Reset value 0x1d0 (SYS_SOFT_RSTN, active-low, auto-releasing) holds
     * protocol[0]+phy[1]+fm[2]+audio[3]+adc[5] in reset, RELEASES codec[4]+
     * adc_ctrl[6]+sys[7]+cpu[8].  This is the vendor's boot value 0x190 MINUS bit6
     * (adc_ctrl): we must NOT reset the CPU/volume ADC at 0x140d0000 -- the UI/main
     * threads are already polling it for the volume knob + battery, and resetting it
     * mid-run hung the whole system.  bit5 (the baseband IF ADC) IS reset -- that's
     * the one the modem demod needs, and the one the earlier failed 0x1f8 test omitted. */
    /* FM-RX CLOCK GATE (the missing piece, 2026-06-10).  CLK_MGR @0x2c boot value
     * 0xfff0ff3c (BSP clk_init) leaves [7]modem_clk_fmrx_en + [6]modem_clk_fmtx_en
     * OFF -- so the analog-FM-RX baseband datapath has NO CLOCK and stays
     * configured-but-not-running (adc_testout=0; PHASE_OUT reads wedge the AHB --
     * a read into an unclocked block hangs the bus).  [9]modem_clk_rx_en is ON but
     * that's the DMR/baseband RX clock, NOT FM.  Manual §4.2.5.11 [7]=FM-RX clock
     * gate, active-high.  Blind write of boot-value|0xC0 (enable both FM clocks):
     * no read (modem-MMIO reads wedge our loader) and no clock gets disabled, so
     * safe from the rtx thread.  Must precede the reset + datapath config below. */
    SOCSYS_REG2C = 0xfff0fffcu;

    SOCSYS_SYS_SOFT_RSTN = 0x000001d0u;

    /* Codec power-up (the audio block was just reset). */
    g_codec_inited = false;
    hd2_codec_audio_init();
    g_codec_inited = true;

    /* AF-RECEIVE BIAS (the fix, RE 2026-06-09): in AF mode (RF_MODE[24]=1) the
     * AT1846S audio enters the baseband ADC's VINP, and the ADC's VINM/VCM must be
     * biased by the CPU-bus DAC (manual §8.2.3.1; ADC_CONTROL adc_extcm=1+semode=1).
     * Our build never programmed it, so the AF ADC had no operating point and never
     * sampled (adc_testout=0 -> no demod audio).  Vendor dac_controller_init @0x0305960C:
     * route the DAC analog-out pins (DIPLEX2 PTC-mux bits 28/29 clear), power up DAC
     * channels B+C, and drive channel C to the AF bias 0x6E2. */
    SOCSYS_IO_DIPLEX2 &= 0xCFFFFFFFu;    /* route DAC out pins (clear PTC mux bits 28-29) */
    DAC_MCU_PD_MODE_EN = 0x00000001u;
    DAC_MCU_DATA_A     = 0x00000000u;
    DAC_MCU_DATA_B     = 0x00000000u;
    DAC_MCU_DATA_C     = 0x00000000u;
    DAC_MCU_PD_CTRL    = 0x00000001u;    /* power-down A (bit0=1), power-up B+C */
    DAC_MCU_DATA_C     = 0x000006E2u;    /* AF-receive single-point bias (vendor value) */

    /* Modem analog-FM datapath gate + RF/IF/AGC -- written ONCE here, after the reset.
     * Values verified live against a vendor radio playing FM (dbgshell, 2026-06-09). */
    SOCSYS_DAC_CONTROL  = 0x8000001fu;
    SOCSYS_ADC_CONTROL  = 0x000041c3u;   /* enadc0[8]+enadc1[7]+adc_enref[6] (baseband IF ADC) */
    SOCSYS_VOICE_PATH   = 0x00000002u;   /* bit0=0 -> audio via codec DAC (FM) */
    SOCSYS_PCM_MODE     = 0x00000000u;
    SOCSYS_LINEOUT_CTRL = 0x00000001u;   /* line2out only -- vendor live value (was 0x03) */
    SOCSYS_WORK_MODE    = 0x0000006eu;   /* FM-analog (vendor live) */
    SOCSYS_RF_MODE      = 0x034c9060u;   /* RF_MODE: AF-receive (vendor live) */
    SOCSYS_REG(0x110u)  = 0x00041f1au;   /* RF_CONTROL (vendor live) */
    SOCSYS_REG(0x114u)  = 0x01e80000u;   /* RF/IF reg (vendor live; was 0 on ours) */
    SOCSYS_REG(0x120u)  = 0x0978786fu;   /* THRESHOLD_VALUE: arrival/timing-sync detect
                                          * (vendor live; we never set it -- §8.2.2 ties it
                                          * to ADC-receive; may gate the AF datapath). */
    SOCSYS_REG(0x168u)  = 0x00000014u;   /* slot_guard (vendor live) */
    SOCSYS_REG(0x1b0u)  = 0x000bb800u;   /* RX_IF_FREQ = 768 kHz */
    SOCSYS_REG(0x1b4u)  = 0x000036b0u;   /* RX_AGC (vendor live) */
    SOCSYS_AF_GATE      = 0x0001007fu;   /* FM-RX audio gate (vendor live) */
    SOCSYS_MODEM_RXDP0  = 0x000000c4u;   /* modem RX datapath shadow */
    SOCSYS_MODEM_RXDP1  = 0x00000040u;
}

extern "C" void hd2_radio_selftest(uint16_t out[4])
{
    AT1846S& at = AT1846S::instance();

    if (!g_radio_test_inited)
    {
        /* Full chip init (VCO calibration dance, ~700 ms of delays). */
        at.init();
        at.setOpMode(AT1846S_OpMode::FM);
        at.setBandwidth(AT1846S_BW::_25);    /* widest filter (closest to BCFM) */

        /* Make the two audio-gate GPIOs outputs; start with the speaker
         * amp MUTED (GPIOB.4 HIGH), like the vendor does while tuning. */
        GPIOB_DDR |= (SPKR_MUTE_BIT | SPKR_GAIN_BIT | AUDIO_ROUTE_BIT);
        GPIOB_DR  |= SPKR_MUTE_BIT;

        g_radio_test_inited = true;
    }

    /* 1. reg 0x33 readback proves the I2C R/W path + that init ran (0x45F5
     *    region after calibration; 0x0000/0xFFFF => dead bus). */
    out[0] = at1846s_read_reg(0x33);

    /* 2. Tune to the host-selected frequency and enable RX. */
    at.setFrequency(g_fm_test_freq);
    at.setFuncMode(AT1846S_FuncMode::RX);
    rssi_settle();

    /* 3. FM RX audio path (vendor radio_transition_tx_to_rx + audio_route_rx_unmute):
     *    AT1846S reg 0x40 = 0x30, route RX audio (GPIOB.10 LOW), unmute the
     *    speaker amp (GPIOB.4 LOW).  Gated by g_fm_audio_on so the host can
     *    A/B against a muted RSSI-only sweep. */
    if (g_fm_audio_on)
    {
        if (!g_codec_inited) { hd2_codec_audio_init(); g_codec_inited = true; }

        /* AT1846S RX AF-DSP enable. reg0x40=0x30 is the vendor LISTEN value
         * (radio_transition_tx_to_rx @0x030415e8); bit 0x20 = RX AF-DSP path on.
         * 0x11 (what we had) is the vendor SCAN-MUTE value (AF attenuated between
         * scan dwells, task_rf_retune @0x030403a4) -- copying it onto the listen
         * path gave faint hiss / no program audio. */
        at1846s_write_reg(0x40, 0x0030);     /* vendor FM-RX LISTEN (AF-DSP enable) */

        /* AT1846S RX audio output config -- vendor at1846s_apply_rx_audio_config
         * (0x03058f84), no-tone FM path: analog audio filter/AGC + output + VOLUME. */
        at1846s_write_reg(0x3a, 0x80e1);
        at1846s_write_reg(0x41, 0x471e);
        at1846s_write_reg(0x44, 0x0900u | (uint16_t)(g_fm_volume & 0xff));
        at1846s_write_reg(0x33, 0x44a5);
        at1846s_write_reg(0x54, 0x2a3c);
        at1846s_write_reg(0x63, 0x16ad);
        at1846s_write_reg(0x58, 0x8405);
        at1846s_write_reg(0x4e, 0x6002);

        /* Vendor analog-FM order (rf_apply_channel_to_pll @0x03059090): AFTER
         * apply_rx_audio_config, write reg0x7a then (re)load the FM audio-DSP
         * bank.  apply_rx_audio_config left reg0x3a=0x80e1; the FM bank restores
         * it to 0x00c3 (+ the 0x06..0x12 page-1 EQ coefficients).  WITHOUT this
         * the chip ran the DMR filter bank on an FM signal -> no demod audio. */
        at1846s_write_reg(0x7a, 0xa00a);
        at.setBandwidth(AT1846S_BW::_25);     /* now selects FM bank (index 0) */

        /* HR_C7000 analog-FM audio gate -- EXACT vendor values captured LIVE
         * via dbgshell word-reads from the vendor radio *while it played FM
         * 107.5* (2026-06-01).  Every socsys reg below now matches the
         * vendor's playing state; our prior guessed values (WORK_MODE=0x22,
         * af_gate=0x800, pcm_mode=3, voice_path bit0-clear, LINEOUT unset)
         * were all wrong.  KEY: the modem RX latch (0x11000398) reads 0 on the
         * vendor *while playing FM* -> FM broadcast is pure-analog through the
         * codec; the digital modem need not run (kills the modem-run theory).
         * No soft-reset hold needed -- vendor steady-state is 0x1ff. */
        SOCSYS_DAC_CONTROL  = 0x8000001fu;
        SOCSYS_ADC_CONTROL  = 0x000041c3u;
        SOCSYS_VOICE_PATH   = 0x00000002u;   /* bit1 set                  */
        SOCSYS_PCM_MODE     = 0x00000000u;
        SOCSYS_LINEOUT_CTRL = 0x00000003u;   /* line1out_en|line2out_en: HW-verified
                                              * 2026-06-02 the HD2 speaker is on LINE2OUT
                                              * (bit0). Writing 0x01 alone gets auto-flipped
                                              * to 0x02 (line1out) by the modem -> wrong pin
                                              * -> silence. Forcing both keeps line2out driven:
                                              * total silence -> audible codec output. */
        SOCSYS_WORK_MODE    = 0x0000006eu;   /* vendor FM-analog value    */
        SOCSYS_AF_GATE      = 0x0001007fu;   /* vendor FM-RX audio gate   */
        SOCSYS_MODEM_RXDP0  = 0x000000c4u;
        SOCSYS_MODEM_RXDP1  = 0x00000040u;

        GPIOB_DR &= ~AUDIO_ROUTE_BIT;        /* GPIOB.10 LOW -> route RX audio */
        GPIOB_DR &= ~SPKR_MUTE_BIT;          /* GPIOB.4  LOW -> speaker unmute  */
        GPIOB_DR |=  SPKR_GAIN_BIT;          /* GPIOB.17 HIGH -> amp full gain  */

        /* CONTINUOUS modem-RX servicing (the piece our superloop lacks): mimic
         * task_sysint -- drain the modem RX IRQ latch (0x11000398) -> ACK
         * (0x110003a0) every tick so the modem FSM keeps pumping the hardware
         * FM-audio datapath.  Run ~3 s so the user can listen during the call.
         * g_modem_irq_acc accumulates every latch value seen -> if it stays
         * 0x4000 (idle) the FSM never raised an RX IRQ = clock-dead (#90/#94);
         * if it changes, the modem RX FSM is alive. */
        /* Short settle drain only. FM broadcast is pure-analog through the codec
         * (modem latch reads 0 on the vendor while playing FM), so the continuous
         * modem-RX servicing is NOT needed for audio -- and a long busy-wait here
         * would freeze the threaded-build UI. The FM worker thread keeps audio
         * alive after this returns. (Was 300*10ms=3s; now ~0.2s.) */
        g_modem_irq_acc = 0;
        for (uint32_t k = 0; k < 20u; ++k)
        {
            uint32_t v = SOCSYS_MODEM_IRQ;
            g_modem_irq_acc |= v;
            SOCSYS_MODEM_IRQ_ACK = v;                  /* ACK */
            delay_ms(10);
        }
    }
    else
    {
        GPIOB_DR |= SPKR_MUTE_BIT;           /* keep speaker muted */
    }

    /* 4. RSSI (reg 0x1B upper byte = level) + mode/status (0x30) + 2nd RSSI. */
    out[1] = at1846s_read_reg(0x1B);
    out[2] = at1846s_read_reg(0x30);
    out[3] = at1846s_read_reg(0x1B);
}

/* Live AT1846S register access for the loader 'q'/'Q' commands.  Lets us
 * verify the loaded audio bank (read reg 0x15: 0x1100=FM / 0x1f00=DMR) and
 * binary-search the FM AF-output path while listening, without a reflash.
 * The chip must have been inited first (run 'a' once). */
/* Route through the file-static at1846s_read_reg/at1846s_write_reg helpers,
 * NOT the AT1846S class methods.  The class i2c_readReg16 -- though textually
 * identical -- returned a stuck 0x0f1c for every register as an isolated
 * loader command, while these same-file helpers (used by the selftest, which
 * reads real RSSI live) work.  Likely a per-TU codegen difference in the
 * timing-sensitive bit-bang.  Use the proven path. */
extern "C" uint16_t hd2_at1846s_read(uint8_t reg)
{
    return at1846s_read_reg(reg);
}
extern "C" void hd2_at1846s_write(uint8_t reg, uint16_t val)
{
    at1846s_write_reg(reg, val);
}

/*
 * VOX detector control over the proven bit-bang path (the AT1846S class i2c
 * reads are flaky in the live loop -- see the q/Q caution below).  Detect-
 * during-RX mode: reg 0x64 open/shut thresholds + reg 0x30 bit4 vox_on; result
 * in reg 0x1C bit1 (vox_cmp).  Used by hd2_rtx.c's VOX key path.
 */
extern "C" void hd2_vox_enable(uint8_t thHigh, uint8_t thLow)
{
    if(g_rf_freeze != 0u) return;
    at1846s_write_reg(0x64, (uint16_t)(((thHigh & 0x7F) << 7) | (thLow & 0x7F)));
    at1846s_write_reg(0x30, (uint16_t)(at1846s_read_reg(0x30) | 0x0010u));
}
extern "C" void hd2_vox_disable(void)
{
    if(g_rf_freeze != 0u) return;
    at1846s_write_reg(0x30, (uint16_t)(at1846s_read_reg(0x30) & ~0x0010u));
}
extern "C" bool hd2_vox_detected(void)
{
    if(g_rf_freeze != 0u) return false;
    if((at1846s_read_reg(0x30) & 0x0010u) == 0u) return false;   // vox_on off
    return ((at1846s_read_reg(0x1C) & 0x0002u) != 0u);           // vox_cmp
}

/*
 * RF carrier/squelch detect via the AT1846S's OWN comparator: reg 0x1C bit0 =
 * sq_cmp, the chip's RSSI+noise decision with built-in hi/lo hysteresis
 * (thresholds in 0x48/0x49, set from the squelch level by setSquelchLevel).
 * This is MUCH steadier than thresholding our raw reg-0x1B RSSI, which reads
 * ~40 dB low on ~10-20% of transfers and flutters the audio gate.  Mirrors the
 * vendor's rx_squelch_monitor_tick (@0x03040a6c, reads 0x1C bit0).  Bit-bang
 * read path (the AT1846S-class i2c reads are flaky in the live loop).
 */
extern "C" bool hd2_rx_carrier_detected(void)
{
    if(g_rf_freeze != 0u) return false;
    return ((at1846s_read_reg(0x1C) & 0x0001u) != 0u);           // sq_cmp
}

/*
 * Strong override of the OpMode_FM hardware RF-squelch hook (radio.h): the HD2
 * has a real on-chip squelch comparator, so report sq_cmp instead of letting
 * OpMode_FM threshold the jittery raw RSSI.  Lets HD2 use the portable
 * OpMode_FM RX path without re-introducing the squelch flutter (the same fix
 * hd2_rtx.c applies today via rtx_rxSquelchOpen).
 */
extern "C" bool radio_checkRxRfSquelch(bool *open)
{
    *open = hd2_rx_carrier_detected();
    return true;
}

/* ---- FM TX-extras HAL (interfaces/radio.h) -- strong overrides of OpMode_FM's
 * weak hooks, ported from the hd2_rtx.c TX path.  Tone1 generator (reg 0x35
 * freq, 0x3A[14:12] source, 0x79[15:14] output); tail-elim reverse burst
 * (reg 0x30[11]); VOX via the bit-bang hd2_vox_* ops (defined below). ---- */

extern "C" void radio_fmToneBurst(void)
{
    if(g_rf_freeze != 0u) return;
    hd2_at1846s_write(0x35, 17500u);                                                /* 1750.0 Hz */
    hd2_at1846s_write(0x3A, (uint16_t)((hd2_at1846s_read(0x3A) & ~0x7000u) | 0x1000u)); /* tone1 src */
    hd2_at1846s_write(0x79, (uint16_t)((hd2_at1846s_read(0x79) & ~0xF000u) | 0xC000u)); /* tone out */
    sleepFor(0u, 750u);                                                             /* ~0.75 s burst */
    hd2_at1846s_write(0x3A, (uint16_t)((hd2_at1846s_read(0x3A) & ~0x7000u) | 0x4000u)); /* back to mic */
    hd2_at1846s_write(0x79, (uint16_t)(hd2_at1846s_read(0x79) & ~0xF000u));          /* tone off */
}

extern "C" void radio_fmTailElim(void)
{
    if(g_rf_freeze != 0u) return;
    /* Engage the reverse-burst (reg 0x30[11]) with the carrier still keyed; the
     * caller dekeys after this returns (radio_disableRtx clears the bit). */
    hd2_at1846s_write(0x30, (uint16_t)(hd2_at1846s_read(0x30) | 0x0800u));
    sleepFor(0u, 180u);                                                             /* ~180 ms hold */
}

extern "C" void radio_fmVoxArm(uint8_t level)
{
    if(g_rf_freeze != 0u) return;
    /* Map VOX level 1..5 to an AT1846S reg-0x64 threshold (higher = more
     * sensitive = lower threshold); 0 / out-of-range disables. */
    static const uint8_t code[5] = { 0x45u, 0x48u, 0x4Cu, 0x52u, 0x58u };
    if(level == 0u || level > 5u) { hd2_vox_disable(); return; }
    uint8_t th = code[level - 1u];
    hd2_vox_enable(th, th);
}

extern "C" bool radio_fmVoxDetected(void)
{
    return hd2_vox_detected();
}

/*
 * One-shot full AT1846S RX bring-up from firmware (loader op 'I', at_reinit):
 * chip init() (vendor sequence + GDx supplement; ~700 ms of calibration
 * delays -- BLOCKS the calling diag thread that long) + FM filter bank 0 +
 * retune + vendor FM-RX reg-0x30 enable + the RX AF-DSP audio config.  Best
 * run with rf_freeze=1 so the rtx thread doesn't interleave bit-bang traffic
 * mid-init (the GPIOA bus has no real lock; see the q/Q caution in
 * hd2_diag.cpp).  freq_hz: the RX frequency to re-apply (the diag op passes
 * the rtx thread's current config).
 */
extern "C" void hd2_at1846s_reinit(uint32_t freq_hz)
{
    AT1846S& at = AT1846S::instance();

    at.init();                                /* vendor chip init + VCO cal  */
    at.setBandwidth(AT1846S_BW::_25);         /* FM bank 0 (vendor-confirmed) */
    at.setFrequency(freq_hz);                 /* PLL retune (leaves 0x30=0x4046) */

    /* Vendor FM-RX reg-0x30 enable pattern (rf_apply_channel_to_pll FM
     * branch): 0x4806 then 0x4826 = band + RX-side path bit11 + RX_ON. */
    at1846s_write_reg(0x30, 0x4806);
    at1846s_write_reg(0x30, 0x4826);

    /* RX AF-DSP listen config (re-asserts 0x4806/0x4826 at its tail). */
    hd2_at1846s_rx_audio_config();

    /* The chip is fully inited now -- keep the 'a' selftest from running a
     * second cold init on its next use. */
    g_radio_test_inited = true;
}

/*
 * A/B audio-gain profile switch, live (loader op 'o', at_profile):
 *   0 = vendor HD2 values (what init()+rx_audio_config program),
 *   1 = the GD77 driver's values for the same chip (working FM audio there).
 * Writes ONLY these four registers -- no other state is touched, so it can be
 * flipped while listening.  0x44 vendor form is 0x0900 | volume (g_fm_volume,
 * host-pokeable), matching hd2_at1846s_rx_audio_config.
 */
extern "C" void hd2_at1846s_profile(uint32_t profile)
{
    if (profile == 0u)
    {
        at1846s_write_reg(0x0a, 0x4c20);     /* PGA gain        (vendor) */
        at1846s_write_reg(0x59, 0x0b90);     /* mixer gain      (vendor) */
        at1846s_write_reg(0x33, 0x44a5);     /* AGC number      (vendor FM-RX) */
        at1846s_write_reg(0x44, (uint16_t)(0x0900u | (g_fm_volume & 0xffu)));
    }
    else
    {
        at1846s_write_reg(0x0a, 0x7ba0);     /* PGA gain        (GD77) */
        at1846s_write_reg(0x59, 0x09d2);     /* mixer gain      (GD77) */
        at1846s_write_reg(0x33, 0x45f5);     /* AGC number      (GD77) */
        at1846s_write_reg(0x44, 0x05cc);     /* TX/RX gain      (GD77 final) */
    }
}

/*
 * Set/clear AT1846S reg 0x30 bit7 -- the documented RX AF mute (loader op
 * 'm', at_mute).  Read-modify-write so the band/RX-enable bits are kept.
 * Returns the resulting reg 0x30 value (post-write readback).
 */
extern "C" uint16_t hd2_at1846s_afmute(uint32_t mute)
{
    uint16_t r = at1846s_read_reg(0x30);
    if (mute != 0u) r |= (uint16_t)0x0080u;
    else            r &= (uint16_t)~0x0080u;
    at1846s_write_reg(0x30, r);
    return at1846s_read_reg(0x30);
}
