/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Broadcast-FM tuner driver for the Ailunce HD2 (HR_C7000).
 *
 * The HD2 "FM radio" (broadcast) feature is a DEDICATED FM tuner chip -- an RDA
 * Microelectronics RDA5802E (see vendor/RDA5802E-RDA.pdf) -- on the AT1846S's
 * bit-bang I2C bus (GPIOA SCL=PTA7 / SDA=PTA8) at 8-bit write address 0x20
 * (7-bit 0x10 = the RDA5802E *sequential* access address).  It is NOT the
 * AT1846S 2-way transceiver and NOT the HR_C7000 modem -- broadcast audio is
 * analog out of this chip, gated to the speaker by GPIO, no DSP/codec.
 *
 * Register model (RDA5802E): 16-bit registers, MSB first; writes always start
 * at reg 0x02 and auto-increment, reads start at reg 0x0A (no random access at
 * 0x20).  See RDA5802E_HD2.c.  (scripts/labels/fm_broadcast.py mis-modelled
 * this as 8-bit [reg][val] writes -- ignore that; the .c has the real decode.)
 *
 * GPIO control (LIVE-VERIFIED 2026-06-08, before/after MMIO diff while playing
 * broadcast out the speaker -- supersedes the GPIOB.0/PTB14 readings in
 * fm_broadcast.py, see hd2_regs.h):
 *   - FM_ENABLE_BIT  = PTB20 (event 0x34): LOW = active, HIGH = standby.
 *   - AUDIO_ROUTE_BIT = PTB10 (event 0x2a): LOW = broadcast audio -> speaker.
 *   (GPIOB.0 is the green LED and is NOT involved; PTB14 did not move.)
 */

#ifndef FM_BROADCAST_HD2_H
#define FM_BROADCAST_HD2_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Broadcast FM band edges, kHz (76.0 - 108.0 MHz, 100 kHz channel step). */
#define FM_BCAST_MIN_KHZ   76000u
#define FM_BCAST_MAX_KHZ  108000u
#define FM_BCAST_STEP_KHZ    100u

/**
 * Bring the bit-bang bus + FM-tuner GPIO lines up.  Idempotent; safe to call
 * before any other entry point.  Does not enable the chip.
 */
void fm_broadcast_init(void);

/**
 * Power up + initialise the tuner: enable/reset strobe (PTB20), 64-byte analog/
 * AGC init burst (reg 0xc6.., live-confirmed @0x0307ade4), band-select and the
 * audio-output strobe.  Leaves the chip in its running state.  Call tune()
 * afterwards to land on a station, and route_speaker() to hear it.
 */
void fm_broadcast_powerup(void);

/**
 * Tune to @p freq_khz (clamped to the 76-108 MHz band, snapped to 100 kHz).
 * channel = (freq_khz - 76000) / 100 ; writes reg 0xc2/0xc3/0xc4.
 * Returns the channel number programmed.
 */
uint16_t fm_broadcast_tune(uint32_t freq_khz);

/**
 * Read the chip status (I2C read addr 0x21, 4 bytes).  On return *tuned is set
 * iff the chip reports lock (byte0 bit6) and *channel (if non-NULL) holds the
 * read-back channel = ((byte0 & 3) << 8) | byte3.  Returns true on a clean read.
 */
bool fm_broadcast_status(bool *tuned, uint16_t *channel);

/**
 * Read RSSI[6:0] (RDA5802E read reg 0x0B bits 15:9) as a 0..127 level for the
 * UI meter.  Returns 0 on a bad read.
 */
uint8_t fm_broadcast_rssi(void);

/**
 * Route broadcast audio to the speaker: PTB10 LOW (route) + un-mute the speaker
 * amp (PTB4 active-low LOW).  Live-verified path.
 */
void fm_broadcast_route_speaker(void);

/**
 * Power the tuner down / hand audio back to the 2-way path: PTB20 HIGH (standby)
 * + PTB10 HIGH (route off).
 */
void fm_broadcast_powerdown(void);

#ifdef __cplusplus
}
#endif

#endif /* FM_BROADCAST_HD2_H */
