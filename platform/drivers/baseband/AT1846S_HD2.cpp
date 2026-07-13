/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * AT1846S driver for the HD2 (HD-GPS-HD2PA-C7000).  Ported verbatim
 * from the vendor V2.1.3 firmware (src/firmware/at1846s/at1846s.c) --
 * register sequences are preserved bit-exact; only decimal-vs-hex
 * literals were normalised.  See docs/superpowers/plans/
 * 2026-05-29-at1846s-hd2-openrtx-port.md for the full port plan.
 *
 * Vendor source addresses (V2.1.3 app image @ 0x0300d000):
 *   - at1846s_chip_init        @ 0x03058bd4
 *   - at1846s_load_audio_bank  @ 0x03058b28
 *   - at1846s_set_freq         @ 0x03058e24
 *   - at1846s_reg_write/read   @ 0x03058b60
 */

#include "interfaces/delays.h"
#include "drivers/i2c_csky.h"
#include "drivers/baseband/AT1846S.h"

/*
 * AT1846S audio / channel-filter "bank" tables, extracted live from the
 * V2.1.3 image:
 *
 *   - regList    : 23 register indices @ SAHB SRAM 0x18000a30..0x18000a47
 *   - bankValues : 2 x 23 u16 entries  @ flash 0x0307dee0
 *                  bank 0 = DMR / 12.5 kHz path
 *                  bank 1 = FM  / 25   kHz path
 *
 * Reg 0x7F is the AT1846S page-select register: writing 0x0001 paged in,
 * 0x0000 paged out -- the table's first and last entries do the wrapping
 * so the middle entries (0x06..0x12) hit page-1 AGC registers.
 */
static constexpr uint8_t  bankRegList[23] = {
    0x15, 0x32, 0x3A, 0x3C, 0x3F, 0x48, 0x60, 0x62, 0x65, 0x66,
    0x7F, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E,
    0x0F, 0x12, 0x7F
};

/*
 * CORRECTED 2026-06-01 (decomp dive): the bank LABELS were inverted.  Vendor
 * at1846s_load_audio_bank @0x03058b28 indexes table @0x0307dee0: param 0 =
 * BANK0 (the 0x1100/0x4495/0x00c3.../0xeb2e set) and the vendor's ANALOG-FM
 * path (rf_apply_channel_to_pll @0x03059090 FM-voice case, at1846s_set_freq_alt
 * @0x03058d68) calls load_audio_bank(0).  So index 0 is the FM audio-DSP bank
 * and index 1 is the DMR bank -- NOT the other way round.  We were loading the
 * DMR bank onto broadcast FM (setBandwidth(_25)->1) => DMR filtering on an FM
 * signal => no clean demod audio => only codec idle-hiss.  ELF-verified bytes.
 */
static constexpr uint16_t bankValues[2][23] = {
    /* bank 0 -- FM (analog / broadcast); vendor load_audio_bank(0) */
    { 0x1100, 0x4495, 0x00C3, 0x0407, 0x28D0, 0x20BE, 0x1BB7, 0x1425,
      0x2494, 0xEB2E, 0x0001, 0x0014, 0x020C, 0x0214, 0x030C, 0x0314,
      0x0324, 0x0344, 0x1344, 0x1B44, 0x3F44, 0xE0EB, 0x0000 },
    /* bank 1 -- DMR; vendor load_audio_bank(1) */
    { 0x1F00, 0x7564, 0x04C3, 0x1930, 0x29D2, 0x203E, 0x101E, 0x3767,
      0x248A, 0xFFAE, 0x0001, 0x0024, 0x0214, 0x0224, 0x0314, 0x0324,
      0x0344, 0x0384, 0x1384, 0x1B84, 0x3F84, 0xE0EB, 0x0000 }
};

/*
 * Band-select classifier constants, extracted live from V2.1.3 image:
 *
 *   FREQ_BAND_MODULUS @ 0x03058ec4 = 11 375 000  (~11.375 MHz VCO subband)
 *   FREQ_BAND_THRESH  @ 0x03058ec8 = 11 374 498
 *   FREQ_BAND_SPECIAL @ 0x03058ecc =    455 000 000  (455 MHz -> 0x8643)
 *
 * Vendor at1846s_set_freq @ 0x03058e24 computes:
 *   if (THRESH < ((freq % MODULUS) - 0xFB))     reg5 = 0x8763
 *   elif (freq == SPECIAL)                      reg5 = 0x8643
 *   else                                        reg5 = 0x8543
 *
 * The (modulo - 0xFB) form, with unsigned underflow when (freq % MOD)
 * is below 0xFB (251 Hz), is intentional and selects the 0x8763 band
 * within 251 Hz on either side of any 11.375 MHz boundary.
 */
static constexpr uint32_t FREQ_BAND_MODULUS = 11375000u;
static constexpr uint32_t FREQ_BAND_THRESH  = 11374498u;
static constexpr uint32_t FREQ_BAND_SPECIAL = 455000000u;
static constexpr uint16_t FREQ_BAND_GUARD   = 0x00FBu;

void AT1846S::init()
{
    /* Verbatim port of at1846s_chip_init @ 0x03058bd4.  Do NOT reorder
     * or fold these writes -- the 0x40A4 -> 0x40A6 -> 0x4006 sequence
     * with 100 ms gaps is a VCO calibration dance that has to run as-is.
     *
     * Register-meaning annotations (2026-06-10) come from the sibling
     * OpenRTX drivers for the SAME chip (AT1846S_GDx.cpp / _UV3x0.cpp,
     * both with working FM audio); "GDx:" notes the GD77 value where it
     * differs from the vendor-HD2 value used here.
     *
     * Registers the GD77 init programs that this vendor init NEVER
     * touches (candidate gaps for the silent-AFOUT hunt; see
     * scripts/tmp_gdx_at1846s_bringup.py for the live A/B experiment):
     *   0x09=0x03AC, 0x24=0x0001, 0x3F (RSSI 3 threshold),
     *   0x48/0x60 (noise 1/2 thresholds), 0x49 (RSSI SQL thresholds),
     *   0x4E=0x0082->0x2082 (soft-mute ctrl), 0x62 (mod-detect thresh).
     *
     * 2026-06-10: the 0x09/0x24/0x4E (pre-cal) and 0x49/0x60/0x48
     * (post-cal) gaps are now filled below as explicit "GDx supplement"
     * blocks APPENDED to the vendor sequence -- no vendor write was
     * replaced.  The GDx PGA/mixer overrides (0x0A=0x7BA0 / 0x59=0x09D2)
     * are deliberately NOT applied here; they are a live-switchable A/B
     * profile instead (hd2_at1846s_profile, loader op 'o'). */

    /* Re-assert radio-bus ownership (pad mux + controller config).  The
     * constructor's i2c_init() runs at static-init time, BEFORE platform_init,
     * whose IO_DIPLEX0 write would otherwise leave the pins muxed for the
     * other transport (the 2026-06-12 HW-I2C wedge root cause).  i2c0_init is
     * idempotent and cheap; calling it at bring-up makes the bus state depend
     * only on this sequence, not on boot ordering. */
    i2c_init();

    i2c_writeReg16(0x30, 0x0001);   // Soft reset
    delayMs(200);
    i2c_writeReg16(0x30, 0x0004);   // Chip enable

    i2c_writeReg16(0x04, 0x0FD0);   // 26 MHz crystal frequency (same as GDx)
    i2c_writeReg16(0x0A, 0x4C20);   // PGA gain (GDx: 0x7BA0); decimal '10' in source
    i2c_writeReg16(0x0F, 0x8A24);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x1F, 0x1001);   // GPIO config (GDx: 0x1000 = gpio6 squelch out)
    i2c_writeReg16(0x31, 0x0031);   // (same as GDx)
    i2c_writeReg16(0x33, 0x44A5);   // AGC number (GDx: 0x45F5 from the start)
    i2c_writeReg16(0x34, 0x2B87);   // RX digital gain (GDx: 0x2B89)
    i2c_writeReg16(0x41, 0x470F);   // TX digital gain (same as GDx)
    i2c_writeReg16(0x42, 0x1036);   // (same as GDx)
    i2c_writeReg16(0x43, 0x00BB);   // (same as GDx)
    i2c_writeReg16(0x44, 0x07F7);   // TX/RX gain; HD2 runtime uses low byte as RX volume
    i2c_writeReg16(0x3A, 0x00C3);   // Mod-detect / AF-source select; GDx-FM=0x44C3,
                                    //   vendor-HD2-FM-RX runtime=0x80E1, tone mux=[14:12]
    i2c_writeReg16(0x59, 0x0B90);   // Mixer gain (GDx: 0x09D2)
    i2c_writeReg16(0x47, 0x7F2F);   // Soft mute (same as GDx)
    i2c_writeReg16(0x4F, 0x2C62);   // (same as GDx)
    i2c_writeReg16(0x53, 0x0094);   // (same as GDx)
    i2c_writeReg16(0x54, 0x2A18);   // (GDx: 0x2A3C; vendor FM-RX runtime also 0x2A3C)
    i2c_writeReg16(0x55, 0x0081);   // (same as GDx)
    i2c_writeReg16(0x56, 0x0B22);   // (GDx: 0x0B02)
    i2c_writeReg16(0x57, 0x1C00);   // Bypass RSSI low-pass (same as GDx)
    i2c_writeReg16(0x58, 0x8405);   // Filter/emphasis ctrl (GDx: 0xBCCD)
    i2c_writeReg16(0x5A, 0x0ADD);   // SQ detection time (GDx: 0x4935)
    i2c_writeReg16(0x63, 0x3FFF);   // Mod-detect thresh (GDx: 0x16AD); decimal '99' in source
    i2c_writeReg16(0x79, 0xD932);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x68, 0x05E5);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x6B, 0x02FE);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x71, 0x0F1C);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x73, 0x0A3E);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x74, 0x090E);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x75, 0x0833);   // HD2-specific VCO/PLL fine-tune
    i2c_writeReg16(0x76, 0x0806);   // HD2-specific VCO/PLL fine-tune

    /* --- GDx supplement (pre-cal) ----------------------------------------
     * Registers the working GD77 driver (AT1846S_GDx.cpp init(), same chip,
     * working FM audio) programs BEFORE its calibration that the vendor HD2
     * init never touches.  Appended after the vendor pre-cal block; no
     * vendor value above is replaced. */
    i2c_writeReg16(0x09, 0x03AC);   // GDx supplement: undocumented bias/regulator setup (vendor never writes it)
    i2c_writeReg16(0x24, 0x0001);   // GDx supplement: PLL/synth enable (vendor never writes it)
    i2c_writeReg16(0x4E, 0x2082);   // GDx supplement: soft-mute control, GDx final value (vendor never
                                    //   writes it at init; the vendor FM-RX path later rewrites
                                    //   0x4E=0x6002 at channel config -- that stays.  This is just a
                                    //   sane init value, matching GDx's 0x0082 -> 0x2082 staging.)

    /* VCO calibration -- delays are bit-exact with vendor. */
    i2c_writeReg16(0x30, 0x40A4);
    delayMs(100);
    i2c_writeReg16(0x30, 0x40A6);   // Start calibration
    delayMs(100);
    i2c_writeReg16(0x30, 0x4006);   // Stop calibration
    delayMs(200);

    /* Post-calibration tweaks.  (GDx post-cal additionally re-stages PGA
     * 0x0A, mixer 0x59, TX gain 0x41/0x44 steps, noise/RSSI/SQL thresholds
     * 0x48/0x60/0x3F/0x49 -- the vendor HD2 init does none of that.) */
    i2c_writeReg16(0x58, 0xBCFD);   // Filter/emphasis ctrl (GDx post-cal: 0xBCED)
    i2c_writeReg16(0x40, 0x0031);   // AF-DSP ctrl (runtime: 0x0030 listen / 0x0011 scan-mute)
    i2c_writeReg16(0x33, 0x45F5);   // AGC number (now matches GDx)

    /* --- GDx supplement (post-cal) ---------------------------------------
     * GDx post-calibration writes the vendor HD2 init never does.  Appended
     * after the vendor post-cal tweaks; no vendor value above is replaced.
     * (The GDx 0x0A/0x59 PGA/mixer overrides are NOT here -- see the
     * hd2_at1846s_profile A/B switch.) */
    i2c_writeReg16(0x49, 0x0C96);   // GDx supplement: RSSI SQL thresholds (open/close) (vendor never writes it)
    i2c_writeReg16(0x60, 0x1A32);   // GDx supplement: noise-2 threshold (vendor never writes it)
    i2c_writeReg16(0x48, 0x1A32);   // GDx supplement: noise-1 threshold (vendor never writes it at init;
                                    //   the per-bandwidth bank load overwrites 0x48 later -- fine)
}

void AT1846S::setBandwidth(const AT1846S_BW band)
{
    /*
     * Vendor at1846s_load_audio_bank @ 0x03058b28: walks a 23-entry
     * reg list and writes corresponding values from bank0 (12.5 kHz /
     * DMR) or bank1 (25 kHz / FM).  Reg 0x7F appears twice in the list
     * to page registers 0x06..0x12 in and back out (AT1846S page model).
     */
    /* RE-CONFIRMED 2026-06-09: our bankValues ARE the vendor's at1846s_load_audio_bank
     * table (reg-list + bank0/bank1 byte-identical to ELF @0x0307dee0).  FM path loads
     * BANK 0, DMR loads BANK 1 (the 2026-06-01 correction was right).  The vendor does
     * NOT set reg 0x30 bits[12:13] for bandwidth (FM=0x4xxx, DMR=0x7xxx) -- so no
     * maskSet(0x30,...) here.  (A UV3x0-values experiment was reverted: those were the
     * DMR bank applied to FM.) */
    const uint8_t bank = (band == AT1846S_BW::_25) ? 0 : 1;
    for (uint32_t i = 0; i < 23; ++i)
        i2c_writeReg16(bankRegList[i], bankValues[bank][i]);

    reloadConfig();
}

void AT1846S::setOpMode(const AT1846S_OpMode mode)
{
    /*
     * On the HD2, the vendor firmware ties op-mode (DMR vs FM) to the
     * audio-bank selection rather than treating it as a separate axis
     * -- bank 0 programs the FM audio-DSP chain, bank 1 the DMR one
     * (corrected 2026-06-01).  FM uses the _25 path -> bank 0; DMR uses
     * the _12P5 path -> bank 1.
     *
     * NOTE: full DMR support also needs reg 0x33/0x40-0x44/0x58/0x57
     * tweaks that the vendor handles in the radio layer (out of scope
     * for this port).  For now this is a thin convenience over
     * setBandwidth() so the OpenRTX core's setOpMode() call has a home.
     */
    setBandwidth((mode == AT1846S_OpMode::DMR) ? AT1846S_BW::_12P5
                                               : AT1846S_BW::_25);
}

void AT1846S::setFrequency(const freq_t freq)
{
    /*
     * Verbatim port of at1846s_set_freq @ 0x03058e24.  See the constant
     * block above for the band-select classifier rationale.
     */
    const uint32_t f = static_cast<uint32_t>(freq);
    const uint32_t modRem = (f % FREQ_BAND_MODULUS);

    uint16_t reg5;
    if (FREQ_BAND_THRESH < (modRem - FREQ_BAND_GUARD))
    {
        reg5 = 0x8763u;
    }
    else if (f == FREQ_BAND_SPECIAL)
    {
        reg5 = 0x8643u;
    }
    else
    {
        reg5 = 0x8543u;
    }
    i2c_writeReg16(0x05, reg5);

    /* PLL divider: vendor computes (freq * 4) / 250 == (freq * 16) / 1000,
     * matching AT1846S datasheet's 1/16-kHz-per-bit step.  Use 64-bit
     * intermediate to avoid the 32-bit overflow at freq > ~268 MHz. */
    const uint64_t val = ((uint64_t)f * 16ULL) / 1000ULL;
    i2c_writeReg16(0x29, static_cast<uint16_t>((val >> 16) & 0xFFFFu));
    i2c_writeReg16(0x2A, static_cast<uint16_t>(val & 0xFFFFu));

    /*
     * Vendor at1846s_set_freq tail reloads reg 0x30 with one of two
     * patterns:
     *
     *   chan & 0x40 == 0 && (chan+0xa9) & 0xc0 != 0   -> 0x7006, 0x7046
     *   otherwise                                     -> 0x4006, 0x4046
     *
     * The chan_struct flag selecting between them lives in the radio
     * layer (out of scope here).  Default to the 0x4006/0x4046 path,
     * which is what the analog FM bring-up needs.
     *
     * HD2 has at1846s_set_freq_alt @ 0x03058d68 (uses 0x4026/0x7026 and
     * also reloads the audio bank); TODO when the channel layer lands.
     */
    i2c_writeReg16(0x30, 0x4006);
    i2c_writeReg16(0x30, 0x4046);
}

void AT1846S::setFuncMode(const AT1846S_FuncMode mode)
{
    /*
     * Vendor uses RX/TX path open/close helpers (at1846s_rx_path_open
     * @ 0x030405a0 / close @ 0x03040564) which also touch board-level
     * PA/LNA GPIOs and the speaker mute -- out of scope for this port.
     *
     * The chip-side reg 0x30 bits 5/6 (RX/TX enable) match the base
     * AT1846S header behaviour, so use the same simple mask.  If a
     * later live-trace shows reg 0x30 needs the 0x4006/0x4046 double
     * write here too, revisit -- see plan §8 Q4.
     */
    uint16_t value = static_cast<uint16_t>(mode) << 5;
    maskSetRegister(0x30, 0x0060, value);
}


/*
 * I2C transport layer.  AT1846S sits on the GPIOA bit-banged bus driven
 * by platform/mcu/CSKY_V2/drivers/i2c_csky.c (SDA=GPIOA.8, SCL=GPIOA.7).
 * Slave address is 0xE2 (== 0x71 << 1).  Same wire-protocol shape as
 * AT1846S_GDx.cpp.
 */

static constexpr uint8_t devAddr = 0xE2;

void AT1846S::i2c_init()
{
    i2c0_init();
}

void AT1846S::i2c_writeReg16(uint8_t reg, uint16_t value)
{
    /* AT1846S register payload is big-endian (MSB first on the wire). */
    uint8_t buf[3];
    buf[0] = reg;
    buf[1] = (value >> 8) & 0xFF;
    buf[2] = value & 0xFF;

    i2c0_lockDeviceBlocking();
    i2c0_write(devAddr, buf, 3, true);
    i2c0_releaseDevice();
}

uint16_t AT1846S::i2c_readReg16(uint8_t reg)
{
    uint16_t value = 0;

    i2c0_lockDeviceBlocking();
    i2c0_write(devAddr, &reg, 1, false);
    i2c0_read(devAddr, &value, 2);
    i2c0_releaseDevice();

    /* AT1846S sends register data in big-endian on the wire. */
    return __builtin_bswap16(value);
}
