/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Canonical HR_C7000 SoC register map for the Ailunce HD2 OpenRTX port.
 *
 * Standing rule for HD2 platform code: no raw "magic number" MMIO
 * addresses.  Every `*(volatile ...)0xADDR` access goes through a NAMED
 * macro defined here, so diffs read against the manual instead of hex.
 *
 * Addresses + bit roles are sourced from the existing HD2 driver code and
 * the HR_C7000 manual (reference_hr_c7000_manual).  This header is a pure
 * rename aggregation -- it introduces no new addresses or values.
 *
 * Conventions:
 *   <BLOCK>_BASE        base address of a peripheral block
 *   <BLOCK>_REG(off)    word (uint32_t) accessor at base+off
 *   <BLOCK>_<NAME>      a specific named register (already off-resolved)
 *
 * Most blocks are 32-bit, word-spaced (DesignWare-style).  The codec PCM
 * block at 0x16000900 is byte-addressed (CB(off) below) -- it ignores
 * partial word writes.
 */
#ifndef HD2_REGS_H
#define HD2_REGS_H

#include <stdint.h>

/* ===================================================================== *
 *  SOCSYS -- chip system control / pin-mux / audio routing (0x11000000)
 * ===================================================================== */
#define SOCSYS_BASE         0x11000000u
#define SOCSYS_REG(off)     (*(volatile uint32_t *)(SOCSYS_BASE + (off)))

/* System / clock control.  This block is the CLK_MGR clock manager
 * (HR_C7000 v2.7 manual ch.04), NOT a single PLL.  Two PLLs live here:
 * APLL (0x08) is the dedicated Modem/baseband+audio source clock; BPLL
 * (0x10) drives the SoC/CPU/AHB/APB. */
#define SOCSYS_SYS_SOFT_RSTN SOCSYS_REG(0x00u)   /* bit4 = codec reset (active-low) */
#define SOCSYS_CLK_CTRL     SOCSYS_REG(0x04u)    /* [31]pll_ld(RO) [30]clk_rdy(RO) [3]bclk_work_sel [2]aclk_work_sel [1]bclk_re_cfg [0]aclk_re_cfg */
#define SOCSYS_APLL         SOCSYS_REG(0x08u)    /* APLL: Modem/baseband+audio source PLL */
#define SOCSYS_REG10        SOCSYS_REG(0x10u)    /* BPLL: SoC/CPU/AHB/APB PLL (low 24 bits = cfg) */
#define SOCSYS_CLKDIV_18    SOCSYS_REG(0x18u)    /* [31:24]dac_clk_div [23:16]bclk_out_div [15:8]aclk_out_div [7:0]modem_clk_div */
#define SOCSYS_CLKDIV_1C    SOCSYS_REG(0x1cu)    /* 32K + 192K fractional dividers */
#define SOCSYS_CLKDIV_20    SOCSYS_REG(0x20u)    /* [31]mclk_sel [30:26]mclk_div [24:13]clk_8k_div [12:0]clk_512k -- codec clocks */
#define SOCSYS_REG24        SOCSYS_REG(0x24u)    /* PLL config (low nibble) */
#define SOCSYS_REG28        SOCSYS_REG(0x28u)    /* PLL config */
#define SOCSYS_REG2C        SOCSYS_REG(0x2cu)    /* gated-clock enable: [10]mc_clk_en [9]modem_rx [8]modem_tx [3]512k [2]8k */
#define SOCSYS_REG30        SOCSYS_REG(0x30u)    /* PLL config */

/* CLK_CTRL (0x04) bit defs */
#define CLK_CTRL_PLL_LD     0x80000000u          /* [31] RO: apll_ld & bpll_ld (PLL locked) */
#define CLK_CTRL_CLK_RDY    0x40000000u          /* [30] RO: clock-switch ready */
#define CLK_CTRL_ACLK_WORK_SEL 0x04u             /* [2] 1=baseband on xtal, 0=on APLL */
#define CLK_CTRL_BCLK_WORK_SEL 0x08u             /* [3] 1=SoC on xtal, 0=on BPLL */
#define CLK_CTRL_RE_CFG     0x03u                /* [1:0] aclk/bclk re-cfg trigger */

/* Known-good CLK_MGR values: writable bits captured from a vendor V2.1.3 unit
 * audibly playing FM (tmp/vendor_audio_clock_snapshot.json).  All four are the
 * chip reset defaults -- the vendor never reprograms them -- but our IAP hand-off
 * leaves them implicit, so we set them explicitly before engaging the PLLs. */
#define SOCSYS_APLL_CFG     0x05040eb2u          /* APLL writable bits (bit31 is RO lock) */
#define SOCSYS_CLKDIV_18_CFG 0x100a0c0cu         /* modem_clk_div=0x0c -> 9.8304 MHz */
#define SOCSYS_CLKDIV_1C_CFG 0x2e002900u
#define SOCSYS_CLKDIV_20_CFG 0xa5771177u         /* mclk_sel=1 (xtal), codec dividers */

/* Pin-mux "diplex" selectors (32-bit; byte-access returns low byte only). */
#define SOCSYS_IO_DIPLEX0   SOCSYS_REG(0x34u)    /* PTA mux: bit8 i2c1_sda_sel, bit11 uart2_rxd, bit12 uart2_txd, bit18 audio-mute */
#define SOCSYS_IO_DIPLEX1   SOCSYS_REG(0x38u)    /* PTA mux (low byte) */
#define SOCSYS_IO_DIPLEX2   SOCSYS_REG(0x3cu)    /* PTC mux: all-GPIO = 0x3ffffffb; ADC pins bits 19-21 */

/* DIPLEX2 whole-register values (manual 4.4.4.6).  Like DIPLEX0, upper bits
 * are write-only / read-as-zero -- NEVER RMW this register, write constants.
 *   PTC_GPIO: every PTC pad in GPIO mode (bit-bang LCD latch + keypad).
 *   LCD_I80:  LCD CS/RS/WR/RD/DB0-7 sel fields (bits 3..18) = 0 -> the HW
 *             i8080 controller @0x12000000 owns PTC3-6/7-14; the rest stays
 *             GPIO (PTC2 reset, ADC pads, DAC pads per the boot value).
 * The keypad matrix lives ON the LCD data pins (rows PTC7-10, cols
 * PTC11-14), so HW-LCD builds swap GPIO<->I80 around each matrix scan
 * (keyboard_HD2.c). */
#define HD2_DIPLEX2_PTC_GPIO 0x3ffffffbu
#define HD2_DIPLEX2_LCD_I80  0x3ff80003u

/* Audio / codec routing (verified live against vendor playing FM 107.5). */
#define SOCSYS_DAC_CONTROL  SOCSYS_REG(0x70u)    /* dac_control (bit31, pwda bit5) */
#define SOCSYS_ADC_CONTROL  SOCSYS_REG(0x74u)    /* adc_control (enadc0/1/enref = 0x41c3) */
#define SOCSYS_VOICE_PATH   SOCSYS_REG(0x80u)    /* voice_path: bit0=0 -> codec DAC */
#define SOCSYS_PCM_MODE     SOCSYS_REG(0x84u)    /* pcm_mode */
#define SOCSYS_LINEOUT_CTRL SOCSYS_REG(0x88u)    /* [0]line2out_en [1]line1out_en [31]standby_lo(RO) */
#define SOCSYS_CODEC_I2C_MUX SOCSYS_REG(0x8cu)   /* [7] codec-config iface: 0=MC (modem ctrl), 1=I2C */
#define SOCSYS_WORK_MODE    SOCSYS_REG(0x100u)   /* work_mode (FM-analog = 0x6e) */
#define SOCSYS_RF_MODE      SOCSYS_REG(0x104u)   /* baseband RF-interface mode; FM live value 0x034c9060 */

/* HR_C7000 PHY datapath TX regs (manual §9.2.2; values are vendor DMR-TX
 * captures, see docs/dmr_c6000_to_c7000_map.md + clean-room proposal §2.1). */
#define SOCSYS_SIG_CENTER   SOCSYS_REG(0x108u)   /* MOD centre/offset (DMR 0x6868e7e7) */
#define SOCSYS_TX_IF_FREQ   SOCSYS_REG(0x10cu)   /* TX IF frequency word (0x000bb800) */
#define SOCSYS_RF_CONTROL   SOCSYS_REG(0x110u)   /* tx_pre_on / rf_pre_on_tx (DMR 0x00041f1a) */
#define SOCSYS_RF_MOD_BIAS  SOCSYS_REG(0x114u)   /* two-point MOD1/MOD2 amplitudes (low 2 bytes = deviation) */
#define SOCSYS_DEV_LIMITER  SOCSYS_REG(0x118u)   /* TX freq-deviation limit (manual §9.2.2.7); reset default 0xff00 = full deviation allowed */
/* NOTE: 0x11c = RSSI_BIAS_VALUE (RO, manual §9.2.2.8) and 0x120 = THRESHOLD_VALUE
 * (RX sync-detection thresholds, §9.2.2.9) -- neither is a TX modulation control,
 * do NOT write them in the TX path (clean-room verification 2026-06-18). */
#define SOCSYS_SLOT_GUARD   SOCSYS_REG(0x168u)   /* TDMA slot guard (DMR 0x14) */

/* DMR sync-word block: 14 words at 0x12c..0x160 (SEND_DATA_SYNC / SEND_RC_SYNC /
 * recv sync patterns).  NOTE: 0x128 is PHASE_OUT (RO, manual §9.2.2.10) -- NOT a
 * sync word; the real sync block starts at 0x12c (clean-room verification). */
#define SOCSYS_DMR_SYNC_BASE 0x12cu
#define SOCSYS_DMR_SYNC(i)  SOCSYS_REG(SOCSYS_DMR_SYNC_BASE + (i)*4u)
#define SOCSYS_MODEM_IRQ    SOCSYS_REG(0x398u)   /* modem RX IRQ latch (idle = 0x4000) */
#define SOCSYS_AF_GATE      SOCSYS_REG(0x39cu)   /* SYS_INTERP_MASK (was "FM-RX audio gate"): modem interrupt mask. Vendor FM value 0x1007f -- bit16 covers FM_TX_INTERP; with the boot 0x7f value the FM-TX engine wedges on its first unserviced interrupt -> clean DEAD carrier (2026-06-11). */
#define SOCSYS_FM_PTT       SOCSYS_REG(0x560u)   /* FM_PTT (manual ch.11): bit0=1 keys the modem FM-TX engine (mic -> codec ADC -> FM modulator -> MOD1/MOD2). HW-verified 2026-06-11: voice received on a second radio. */

#define WORK_MODE_FM_MOD    0x80u                /* work_mode bit7: FM analog modulator mode (0 = DMR) */
#define AF_GATE_FM_VENDOR   0x0001007fu          /* vendor FM SYS_INTERP_MASK value (see SOCSYS_AF_GATE) */

/* ===================================================================== *
 *  CPU DAC @ 0x140F0000 (manual §5.10 + memory map "DAC 64KB"; the
 *  chapter table's 0x140D base is a typo -- that's the ADC).  3 channels,
 *  12-bit data.  Channel B is the vendor's APC TX-power ramp target
 *  (FUN_0002f6c4 ramps it via FUN_0002f79c(2, val) on TX entry/exit);
 *  per manual ch.8 the APC outputs trim the MOD/PA drive amplitude.
 *  PD bits are active-HIGH power-DOWN (reset = all channels down).
 * ===================================================================== */
#define DAC_PD_CTRL         (*(volatile uint32_t *)(0x140f0000u))
#define DAC_PD_MODE_EN      (*(volatile uint32_t *)(0x140f0004u))
#define DAC_DATA_A          (*(volatile uint32_t *)(0x140f0008u))
#define DAC_DATA_B          (*(volatile uint32_t *)(0x140f000cu))
#define DAC_DATA_C          (*(volatile uint32_t *)(0x140f0010u))
#define SOCSYS_MODEM_IRQ_ACK SOCSYS_REG(0x3a0u)  /* write latch value back to ACK */

/* ===================================================================== *
 *  HR_C7000 DMR Layer-2 (data-link / MAC) control block (0x11000400+)
 *
 *  CORRECTION (2026-06-18, clean-room DMR RE — hd2-clean-v2/dmr_research_
 *  output/DMR_PATH_PROPOSAL.md §3.2): this block was previously mislabeled
 *  "modem RX datapath" (SOCSYS_MODEM_RXDP0/1).  It is in fact the DMR
 *  Layer-2 register file (HR_C7000 manual §10.2.2.16-.24).  The earlier
 *  "the 0x400 block is not the live control surface, drop it" Phase-0
 *  conclusion was a victim of the movih+bseti low-address decompiler
 *  artifact (memory hd2-decomp-movih-bseti-lowaddr-mistrack): the live
 *  probe poked literal RAM 0x00000400, not modem MMIO 0x11000400.
 *
 *  These registers are how the modem's autonomous 30 ms TDMA engine is
 *  started and how each burst is committed.  NOT YET DRIVEN by our port
 *  (DMR TX/RX is scaffold-only); names land here so the driver reads
 *  against the manual.
 * ===================================================================== */
#define SOCSYS_LAYER2_CONTROL  SOCSYS_REG(0x400u) /* [7]txen [6]rxen [1]tx_master_mode; DMR value 0xdb/0xcb (§10.2.2.16) */
#define SOCSYS_LAYER2_SLOTON   SOCSYS_REG(0x404u) /* [0]dll_tx_slot_on; |=0x851 STARTS the 30 ms slot engine (§10.2.2.17) */
#define SOCSYS_LAYER2_TXRX_CTRL SOCSYS_REG(0x408u)/* [7]txnextsloten(commit TX) [6]rxnextsloten [15]begin_v_layer2 (§10.2.2.18) */
#define SOCSYS_LAYER2_SLOT_CNT SOCSYS_REG(0x40cu) /* TX/RX slot-counter preload (§10.2.2.19) */
#define SOCSYS_LAYER2_SEND_TYPE SOCSYS_REG(0x418u)/* [7:4]localdatatype [3]localvod(0=data,1=voice) [1:0]locallcss; written each slot (§10.2.2.22) */
#define SOCSYS_LAYER2_STATUS   SOCSYS_REG(0x41cu) /* RO: [31]tx_slot_choose [8:0]tx_bit_cnt (§10.2.2.23) */
#define SOCSYS_LAYER2_SLOT_PRE SOCSYS_REG(0x424u) /* processing-interrupt advance, rdy_interp_pre_tx (§10.2.2.24) */

/* Deprecated aliases (old "modem RX datapath" names) — kept to avoid
 * breaking any out-of-tree reference; prefer the LAYER2_* names above. */
#define SOCSYS_MODEM_RXDP0  SOCSYS_LAYER2_CONTROL
#define SOCSYS_MODEM_RXDP1  SOCSYS_LAYER2_TXRX_CTRL

/* DMR L2 framing / color-code / privacy (0x384-0x38c, §10.2.2.11-.13). */
#define SOCSYS_LOCAL_CC     SOCSYS_REG(0x384u)   /* [3:0]cc [8]cc_opt [16]encryption_type; CC nibble ORed in per slot */
#define SOCSYS_VOICE_EMB_CTRL SOCSYS_REG(0x388u) /* [1]burstf_emb_ctrl (embedded-LC at voice-F) [0]pi_bit */
#define SOCSYS_SCRAMBLE_REG SOCSYS_REG(0x38cu)   /* Basic-Privacy scramble key (tx[15:0]/rx[31:16]) */

/* TX data/RC sync patterns (PHY block, §9.2.2.11-.14). MS data sync =
 * {d5 d7 f7 7f d7 57}. Programmed once at DMR channel retune. */
#define SOCSYS_SEND_DATA_SYNC_H SOCSYS_REG(0x12cu)
#define SOCSYS_SEND_DATA_SYNC_L SOCSYS_REG(0x130u)
#define SOCSYS_SEND_RC_SYNC_H   SOCSYS_REG(0x134u)
#define SOCSYS_SEND_RC_SYNC_L   SOCSYS_REG(0x138u)

/* WORK_MODE (0x100) DMR value: layermode=1 (Layer-2), Tier-II, TimeSlot.
 * Distinct from the analog-FM modulator path (WORK_MODE_FM_MOD bit7). */
#define WORK_MODE_DMR_L2    0x6au                 /* DMR L2 Tier-II TimeSlot, simplex/direct (manual §4.6.5.7; bit2=is_repeater=0; vendor live 0x6e was a repeater unit) */

/* AUDIO_CONTROL voice-source select (SOCSYS_VOICE_PATH bit0, §4.6.5.3):
 * for a CLEAR DMR voice call this stays 0 so the modem auto-reads the
 * vocoder exchange buffer at MODEM_RAM 0x960; it is set to 1 (TX-RAM) only
 * for relay/encrypted calls where the CPU stages the VoiceBurst at 0x30. */
#define VOICE_PATH_SRC_TXRAM 0x01u                /* bit0: 0=Codec ADC->vocoder, 1=TX-RAM VoiceBurst */

/* HR_C7000 modem shared RAM (manual §10.4): two 8-bit x 1200-deep banks.
 * Used for DMR burst staging; byte-addressed.  TX-RAM frame map:
 *   +0x00 LC header/term/CSBK, +0x29 EMB-F, +0x30 VoiceBurst A-F (27 B),
 *   +0x960 vocoder<->modem AMBE exchange buffer (clear-voice source). */
#define MODEM_RAM_TX_BASE   0x16000000u
#define MODEM_RAM_RX_BASE   0x160004b0u
#define MODEM_RAM_TX(off)   (*(volatile uint8_t *)(MODEM_RAM_TX_BASE + (off)))
#define MODEM_RAM_LC_HDR    0x00u                 /* 9-byte LC word / control burst */
#define MODEM_RAM_VOICE     0x30u                 /* 27-byte VoiceBurst (relay/enc path) */
#define MODEM_RAM_VOCODER   0x960u                /* vocoder<->modem AMBE exchange (clear path) */

/* Modem interrupt status / PCM frame handshake latch (docs/pcm_stream_playback.md).
 * Write-1 bits; the CPU only ever ORs, the modem auto-clears.  bits0-3 = the
 * TS_TX/TS_RX/RF_TX/RF_RX ISR acks; bits 4/5 = the PCM frame handshakes. */
#define SOCSYS_INT_STATUS   SOCSYS_REG(0x3b0u)
#define INT_STATUS_PCM_CAP_ACK   0x10u           /* mic capture frame consumed  */
#define INT_STATUS_PCM_PLAY_ACK  0x20u           /* playback frame supplied     */

/* voice_path (0x80) bits (vendor voice_path_tx_set/rx_set; the tx/rx suffixes
 * in the vendor decomp are direction-swapped -- see docs/pcm_stream_playback.md) */
#define VOICE_PATH_PCM_EN   0x01u                /* bit0: 1 = PCM bridge, 0 = direct codec DAC */
#define VOICE_PATH_CAP      0x10u                /* mic-capture side enable     */
#define VOICE_PATH_PLAY     0x20u                /* playback side enable        */

/* SYS_SOFT_RSTN (0x00) bits 3/4: PCM blocks -- cleared = held in reset
 * (vendor modem_voice_state_save_mutate clears them around transitions). */
#define SOFT_RSTN_PCM_BITS  0x18u

/* SAHB shared-SRAM PCM mailbox windows: 80 x s16 @ 8 kHz, one frame per
 * 10 ms PCM IRQ (docs/pcm_stream_playback.md; pointers proven from vendor
 * FUN_00014ab4 literals 0x14b00/0x14b04). */
#define SAHB_PCM_PLAY       ((volatile uint16_t *)0x180000a0u)  /* CPU writes, codec DACs */
#define SAHB_PCM_CAP        ((volatile uint16_t *)0x18000000u)  /* mic ADC frames appear  */
#define PCM_FRAME_SAMPLES   80u

/* PIC source numbers for the PCM frame IRQs (CK803S vec = src + 0x20;
 * vendor isr_pcm_rd vec 0x3b = playback feed, isr_pcm_wr vec 0x3c = capture). */
#define HD2_IRQ_PCM_PLAY    0x1bu
#define HD2_IRQ_PCM_CAP     0x1cu

/* PIC sources for the DMR Layer-2 slot/sys interrupts (vec = src + 0x20;
 * vendor isr_sysint_dmr vec 0x3d, isr_ts_tx vec 0x3e, isr_ts_rx vec 0x3f). */
#define HD2_IRQ_DMR_SYSINT  0x1du                /* vec 0x3d */
#define HD2_IRQ_DMR_TS_TX   0x1eu                /* vec 0x3e: 30 ms TX slot tick */
#define HD2_IRQ_DMR_TS_RX   0x1fu                /* vec 0x3f: 30 ms RX slot tick */

/* SOCSYS_INT_STATUS (0x3b0) ack bits for the DMR slot ISRs (bits 0-3 are the
 * TS_TX/TS_RX/RF_TX/RF_RX acks; isr_ts_tx does `int_status |= 1`). */
#define INT_STATUS_TS_TX_ACK 0x01u
#define INT_STATUS_TS_RX_ACK 0x02u

/* CPU-bus DAC (DAC_MCU), base 0x140F0000 (manual §5.10; the "DAC @0x140D" in the
 * reg-table directory is a doc typo).  3-ch 12-bit.  In AF-receive mode one channel
 * provides the single-point bias for the baseband AF-ADC's VINM/VCM (manual §8.2.3.1)
 * -- without it the AF ADC never samples (adc_testout stays 0). */
#define DAC_MCU_REG(off)    (*(volatile uint32_t *)(0x140F0000u + (off)))
#define DAC_MCU_PD_CTRL     DAC_MCU_REG(0x00u)    /* [2:0] C/B/A power: 1=power-down, 0=power-up */
#define DAC_MCU_PD_MODE_EN  DAC_MCU_REG(0x04u)
#define DAC_MCU_DATA_A      DAC_MCU_REG(0x08u)
#define DAC_MCU_DATA_B      DAC_MCU_REG(0x0Cu)
#define DAC_MCU_DATA_C      DAC_MCU_REG(0x10u)    /* AF-receive bias value (vendor 0x6E2) */

/* SOCSYS bit/value defs */
#define DIPLEX0_AUDIO_MUTE  0x40000u             /* IO_DIPLEX0 bit18: PWM-audio mute path */
#define DIPLEX0_UART2_RXD   (1u << 11)           /* uart2 rxd select (0 = UART2 RXD on PTA11) */
#define DIPLEX0_UART2_TXD   (1u << 12)           /* uart2 txd select (0 = UART2 TXD on PTA12) */

/* ===================================================================== *
 *  GPIO banks (DesignWare DW_apb_gpio: DR +0x00, DDR +0x04, EXT_PORT +0x50)
 * ===================================================================== */
#define GPIOA_BASE          0x14020000u
#define GPIOB_BASE          0x14100000u
#define GPIOC_BASE          0x14110000u

#define GPIOA_REG(off)      (*(volatile uint32_t *)(GPIOA_BASE + (off)))
#define GPIOB_REG(off)      (*(volatile uint32_t *)(GPIOB_BASE + (off)))
#define GPIOC_REG(off)      (*(volatile uint32_t *)(GPIOC_BASE + (off)))

#define GPIO_DR_OFF         0x00u
#define GPIO_DDR_OFF        0x04u
#define GPIO_EXT_PORT_OFF   0x50u

#define GPIOA_DR            GPIOA_REG(GPIO_DR_OFF)
#define GPIOA_DDR           GPIOA_REG(GPIO_DDR_OFF)
#define GPIOA_EXT_PORT      GPIOA_REG(GPIO_EXT_PORT_OFF)

#define GPIOB_DR            GPIOB_REG(GPIO_DR_OFF)
#define GPIOB_DDR           GPIOB_REG(GPIO_DDR_OFF)
#define GPIOB_EXT_PORT      GPIOB_REG(GPIO_EXT_PORT_OFF)

#define GPIOC_DR            GPIOC_REG(GPIO_DR_OFF)
#define GPIOC_DDR           GPIOC_REG(GPIO_DDR_OFF)
#define GPIOC_EXT_PORT      GPIOC_REG(GPIO_EXT_PORT_OFF)

/* GPIOA bit roles (AT1846S bit-bang I2C). */
#define I2C_SCL_BIT         (1u << 7)            /* PTA7 */
#define I2C_SDA_BIT         (1u << 8)            /* PTA8 */

/* GPIOB bit roles. */
#define LED_GREEN_BIT       0u                   /* PTB0, active-high */
#define LED_RED_BIT         1u                   /* PTB1, active-high */
#define SPKR_AMP_BIT        (1u << 4)            /* PTB4: speaker-amp mute, active-low (LOW=on). NOTE: across two live FM-broadcast captures (tmp/fm_probe, 2026-06-08) this bit was inconsistent (HIGH then LOW) while audio kept playing out the speaker -- it flutters (audio/squelch-gated) and is NOT a deterministic FM-control bit. Do not key the FM path off it. */
#define AUDIO_ROUTE_BIT     (1u << 10)           /* PTB10: RX-audio route, LOW=routed (vendor gpio event 0x2a). Live FM-broadcast diff (reproduced x2): THIS is the pin that toggles for broadcast speaker audio (LOW while FM plays, HIGH in standby). */
#define SPKR_GAIN_BIT       (1u << 17)           /* PTB17: speaker-amp GAIN/ENABLE, active-HIGH. ROOT CAUSE of the "2-way FM RX faint/no audio" saga (live A/B 2026-06-11): with PTB17 LOW all AFOUT audio is ~barely audible at max chip gain; PTB17 HIGH alone restores full loudness (demod voice + static). Vendor holds it HIGH while playing (GPIOB baseline 0x5a6015). PTB19 was A/B'd the same way: NO audible effect (vendor holds it high for something else). All AT1846S-side theories (0x47 soft-mute, 0x42/0x43 scaler, 0x44 vgain) were falsified live -- chip config was never the problem. */
#define AUDIO_ROUTE_WIDE_BIT (1u << 14)          /* PTB14: event 0x2e. CORRECTION (live diff 2026-06-08): PTB14 did NOT move when broadcast FM was toggled on/off out the speaker -- the prior "broadcast uses THIS not PTB10" note is WRONG; broadcast routes on PTB10 (above). Kept for the narrow/wide RX distinction pending re-RE. */
/* PTB20 (event 0x34): FM-broadcast tuner ENABLE/RESET strobe. Live diff: LOW
 * while FM plays, HIGH in 2-way standby. This is the gpio_write(0x34) in the
 * vendor fm_tune_state_machine -- NOT GPIOB.0 (fm_broadcast.py mis-derived the
 * enable as event 0x20 = GPIOB.0, which is actually the green LED and did not
 * move in the diff). */
#define FM_ENABLE_BIT       (1u << 20)
#define PTT_BIT             (1u << 11)           /* PTB11: PTT button, active-LOW. Live-verified 2026-06-11 (run-length sweep: held press = solid 20 s low on GPIOB.11; the earlier "PTA11" finding was GPS-NMEA noise on the shared UART2-RXD pad, and the "PWM ch1 ctrl bit-1" theory was wrong). Matches the original press_detect.py note in keyboard_HD2.c. */
#define PWR_KNOB_BIT        (1u << 12)           /* PTB12: power/volume knob sense (LOW=on, HIGH=off) */
#define PWR_HOLD_BIT        (1u << 13)           /* PTB13: power self-latch (HIGH=hold, LOW=cut) */

/* Atomic read-modify-write on a GPIO data register.  GPIOB_DR is poked from
 * several threads in the threaded build (audio_HD2, hd2_router, the FM RSSI
 * worker); a plain |=/&= can lose an update if a context switch lands between
 * the load and the store, and the worst-case lost bit is PTB13 power-hold.
 * Single core => disabling IRQs around the RMW makes it atomic (no preemption
 * can occur).  Uses the CK803S PSR (cr<0,0>) save + `psrclr ie` idiom already
 * used in targets/HD2/main.c; restoring the saved PSR is nesting-safe and a
 * harmless no-op effect in the single-threaded bare-loader build. */
static inline uint32_t hd2_irq_save(void)
{
    uint32_t psr;
    __asm__ volatile ("mfcr %0, cr<0, 0>\n\tpsrclr ie" : "=r"(psr) : : "memory");
    return psr;
}
static inline void hd2_irq_restore(uint32_t psr)
{
    __asm__ volatile ("mtcr %0, cr<0, 0>" : : "r"(psr) : "memory");
}
static inline void gpio_atomic_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t s = hd2_irq_save(); *reg |= mask;  hd2_irq_restore(s);
}
static inline void gpio_atomic_clear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t s = hd2_irq_save(); *reg &= ~mask; hd2_irq_restore(s);
}

/* ===================================================================== *
 *  UART blocks (DesignWare 16550-compatible, 32-bit word-spaced regs)
 *    UART0  0x14030000  (loader / debug console)
 *    UART2  0x14050000  (GPS NMEA module)
 * ===================================================================== */
#define UART0_BASE          0x14030000u
#define UART2_BASE          0x14050000u

#define UART0_REG(off)      (*(volatile uint32_t *)(UART0_BASE + (off)))
#define UART2_REG(off)      (*(volatile uint32_t *)(UART2_BASE + (off)))

/* Register offsets (DLL/DLH alias RBR/IER when LCR DLAB is set). */
#define UART_RBR_OFF        0x00u                /* RX buffer (read) / DLL / THR */
#define UART_THR_OFF        0x00u
#define UART_DLL_OFF        0x00u
#define UART_DLH_OFF        0x04u                /* divisor high / IER */
#define UART_IER_OFF        0x04u
#define UART_FCR_OFF        0x08u
#define UART_LCR_OFF        0x0cu
#define UART_LSR_OFF        0x14u

#define UART0_RBR           UART0_REG(UART_RBR_OFF)
#define UART0_THR           UART0_REG(UART_THR_OFF)
#define UART0_DLL           UART0_REG(UART_DLL_OFF)
#define UART0_DLH           UART0_REG(UART_DLH_OFF)
#define UART0_LCR           UART0_REG(UART_LCR_OFF)
#define UART0_LSR           UART0_REG(UART_LSR_OFF)

#define UART2_RBR           UART2_REG(UART_RBR_OFF)
#define UART2_THR           UART2_REG(UART_THR_OFF)
#define UART2_LSR           UART2_REG(UART_LSR_OFF)

/* UART bit/value defs */
#define LSR_RX_RDY          0x01u                /* LSR: RBR holds a received byte */
#define LSR_TX_RDY          0x20u                /* LSR: THR empty, ok to send */
#define LCR_DLAB            0x80u                /* divisor-latch access */
#define LCR_8N1             0x03u                /* 8 data bits, 1 stop, no parity */

/* ===================================================================== *
 *  PWM block (0x140c0000).  Channel stride 0x20:
 *    ch0 = +0x00 (backlight), ch1 = +0x20 (audio), ch2 = +0x40
 *  Per-channel: CTRL +0x00, prescaler +0x04, period +0x08, duty +0x0c,
 *  gating words +0x10..+0x1c.
 * ===================================================================== */
#define PWM_BASE            0x140c0000u
#define PWM_CH_STRIDE       0x20u
#define PWM_CH0_BASE        ((volatile uint32_t *)(PWM_BASE + 0u * PWM_CH_STRIDE))   /* 0x140c0000 backlight */
#define PWM_CH1_BASE        ((volatile uint32_t *)(PWM_BASE + 1u * PWM_CH_STRIDE))   /* 0x140c0020 audio */
#define PWM_CH2_BASE        ((volatile uint32_t *)(PWM_BASE + 2u * PWM_CH_STRIDE))   /* 0x140c0040 */

/* PWM ch0 (backlight) duty-gating control words (V2.1.3 = 0x300 each). */
#define PWM_CH0_GATE0       (*(volatile uint32_t *)(PWM_BASE + 0x10u))
#define PWM_CH0_GATE1       (*(volatile uint32_t *)(PWM_BASE + 0x14u))
#define PWM_CH0_GATE2       (*(volatile uint32_t *)(PWM_BASE + 0x18u))
#define PWM_CH0_GATE3       (*(volatile uint32_t *)(PWM_BASE + 0x1cu))

/* Post-PLL timer source feeding the PWM block. */
#define PWM_TIMER_HZ        42000000u

/* ===================================================================== *
 *  ADC block (HR_C7000 on-chip ADC, 0x140d0000).  Offsets per manual
 *  table 4.22 + vendor FUN_0305ae94 / FUN_0305af7c.
 * ===================================================================== */
#define ADC_BASE            0x140d0000u
#define ADC_REG(off)        (*(volatile uint32_t *)(ADC_BASE + (off)))
#define ADC_CTRL            ADC_REG(0x00u)
#define ADC_INTR            ADC_REG(0x08u)
#define ADC_START           ADC_REG(0x14u)       /* write 1 to start a single conversion */
#define ADC_CTRL_STATE      ADC_REG(0x18u)       /* bit0 BUSY, bits[5:1] SAMP_FSM_STATE */
#define ADC_CTRL_STOP       ADC_REG(0x20u)
#define ADC_CH_VLD          ADC_REG(0x24u)       /* channel-enable bitmask (1<<ch) */
#define ADC_SEOC_TIME       ADC_REG(0x28u)
#define ADC_P2S_EN          ADC_REG(0x2cu)
#define ADC_DATA_AB         ADC_REG(0x30u)       /* ch0 bits[9:0] / ch1 bits[25:16] */
#define ADC_DATA_CD         ADC_REG(0x34u)       /* ch2 / ch3 (battery on ch2) */
#define ADC_DATA_EF         ADC_REG(0x38u)       /* ch4 / ch5 */
#define ADC_DATA_GH         ADC_REG(0x3cu)       /* ch6 / ch7 */

/* ===================================================================== *
 *  I2C2 (RTC bus) -- DesignWare DW_apb_i2c (0x14080000).
 * ===================================================================== */
#define I2C2_BASE           0x14080000u
#define I2C2_REG(off)       (*(volatile uint32_t *)(I2C2_BASE + (off)))
#define IC_CON              I2C2_REG(0x00u)      /* master cfg */
#define IC_TAR              I2C2_REG(0x04u)      /* target slave address */
#define IC_DATA_CMD         I2C2_REG(0x10u)      /* tx data / rx cmd */
#define IC_FS_SCL_HCNT      I2C2_REG(0x1cu)      /* fast-mode SCL high */
#define IC_FS_SCL_LCNT      I2C2_REG(0x20u)      /* fast-mode SCL low */
#define IC_INTR_MASK        I2C2_REG(0x30u)      /* interrupt mask */
#define IC_RX_TL            I2C2_REG(0x38u)      /* rx FIFO threshold */
#define IC_TX_TL            I2C2_REG(0x3cu)      /* tx FIFO threshold */
#define IC_ENABLE           I2C2_REG(0x6cu)      /* controller enable */
#define IC_STATUS           I2C2_REG(0x70u)      /* fifo / activity flags */
#define IC_START            I2C2_REG(0xa0u)      /* HR_C7000-specific transfer trigger */

/* ===================================================================== *
 *  Codec / PCM block (HR_C7000, byte-addressed @ 0x16000900).
 *  MUST use byte accesses -- the block ignores partial word writes.
 * ===================================================================== */
#define CODEC_BASE          0x16000900u
#define CODEC_BYTE(off)     (*(volatile uint8_t *)(CODEC_BASE + (off)))

/* AFE config registers (HR_C7000 manual Table 26; config-reg base 0x160009C0,
 * i.e. CODEC_BYTE(0xC0)).  Roles verified live against a vendor unit playing
 * FM -- see tmp/audio_path_findings.md.  "Enabled / unmuted / awake" are the
 * ZERO states on this part (active-low standby bits). */
#define CODEC_DAC_CFG       CODEC_BYTE(0xc8u)    /* [5]master(0) [4]dac_en(0=on) [1:0]iface(00=parallel PCM) */
#define CODEC_ADC_CFG       CODEC_BYTE(0xc9u)    /* [5]master(0) [4]adc_en(0=on) [1:0]iface(00=parallel PCM) */
#define CODEC_DAC_MUTE      CODEC_BYTE(0xcdu)    /* [7]DAC_SOFT_MUTE(0=unmuted) [4]SB_DAC standby(0=on) */
#define CODEC_SLEEP         CODEC_BYTE(0xd2u)    /* [1]SB_SLEEP(0=normal, 1=sleep) */
#define CODEC_MIXER_LATCH   CODEC_BYTE(0xefu)    /* mixer write latch: 0x40/0x80 select before CODEC_MIXER_VAL */
#define CODEC_MIXER_VAL     CODEC_BYTE(0xf0u)    /* mixer value (input-MUX AIPL1/AINL1 line-in select) */

/* CODEC AFE bit defs (write the bit CLEAR to enable; SET to standby/mute). */
#define CODEC_DAC_CFG_EN_BIT   (1u << 4)         /* DAC enable (0=on) */
#define CODEC_ADC_CFG_EN_BIT   (1u << 4)         /* ADC enable (0=on) */
#define CODEC_DAC_SOFT_MUTE    (1u << 7)         /* DAC_SOFT_MUTE (0=unmuted) */
#define CODEC_DAC_SB_BIT       (1u << 4)         /* SB_DAC standby (0=on) */
#define CODEC_SLEEP_SB_BIT     (1u << 1)         /* SB_SLEEP (0=normal) */

/* ===================================================================== *
 *  i8080 LCD parallel-bus controller (0x12000000) -- alternate LCD path
 *  the loader can route the LCD pins to (vs the GPIOC bit-bang latch).
 * ===================================================================== */
#define I8080_LCD_BASE      0x12000000u
#define I8080_LCD_REG(off)  (*(volatile uint32_t *)(I8080_LCD_BASE + (off)))

/* ===================================================================== *
 *  DW timer + PIC (referenced by docs / drivers; included for completeness)
 * ===================================================================== */
#define DW_TIMER_BASE       0x14000000u
#define PIC_BASE            0x17000000u

#endif /* HD2_REGS_H */
