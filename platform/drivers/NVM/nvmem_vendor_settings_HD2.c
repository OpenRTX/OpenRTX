/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Accessor for the vendor-format HD2 radio-settings block (hd2_cps_settings.h)
 * on the W25Q512 SPI-NOR over HW SPI0.
 *
 * The 96-byte hd2_cps_settings_t is stored verbatim (pure vendor byte layout,
 * no OpenRTX wrapper) in a dedicated 4 kB sector at 0x00FEE000 -- the sector
 * immediately below the OpenRTX codeplug region (0x00FEF000) and two below
 * the OpenRTX settings sector (0x00FFF000).  A freshly-erased sector reads as
 * all-0xFF; that is the "blank" sentinel that triggers defaults.
 *
 * NOTE: this is OpenRTX-managed storage of the vendor *format*, not a write to
 * the radio's live factory-settings location (whose physical mapping behind the
 * vendor block protocol is not reliably known -- see hd2_cps_settings.h).
 *
 * The W25Q access primitives mirror nvmem_settings_HD2.c / cps_io_HD2.c.  There
 * are now three near-identical copies; factoring a shared flash_w25q_HD2 driver
 * (was previously copied per-driver).
 * already-validated drivers.
 */

#include "hd2_cps_settings.h"
#include "flash_w25q_HD2.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#define VSET_SECTOR     0x00FEE000u   /* 4 kB sector holding the blob */

/* ================================================================== *
 *  Public accessor                                                    *
 * ================================================================== */

void hd2_cps_settings_defaults(hd2_cps_settings_t *out)
{
    memset(out, 0, sizeof(*out));
    out->squelch    = 3;                       /* mid squelch              */
    out->backlight  = 5;                       /* 5 s backlight            */
    out->ab_time    = 5;                        /* 5 s A/B dwell           */
    out->mic_gain   = 0;                        /* +0 dB                    */
    out->brightness = 4;                        /* level 5 (0-indexed)      */
    out->menu_exit  = 10;                       /* 10 s menu auto-exit      */
    out->step       = HD2_STEP_12_5K;
    /* Key Beep on; English voice; 24h time; radio functions on. */
    out->flags4 |= HD2_VSET_F4_KEY_BEEP;
    out->flags6 |= HD2_VSET_F6_TIME_24H | HD2_VSET_F6_LANG_ENGLISH |
                   HD2_VSET_F6_RADIO_FUNCS;
    /* Programmable + emergency keys: factory-off / Emergency. */
    out->key_define[0] = HD2_KEYFN_OFF;
    out->key_define[1] = HD2_KEYFN_OFF;
    out->key_define[2] = HD2_KEYFN_OFF;
    out->key_define[3] = HD2_KEYFN_OFF;
    out->emergency_key_short = HD2_KEYFN_EMERGENCY;
    out->emergency_key_long  = HD2_KEYFN_EMERGENCY;
}

int hd2_cps_settings_load(hd2_cps_settings_t *out)
{
    if(!w25q_hd2_probe()) { hd2_cps_settings_defaults(out); return -1; }

    w25q_hd2_read(VSET_SECTOR, out, sizeof(*out));

    /* Blank sector (erased flash) -> all 0xFF.  Also reject obviously-bad
     * values that a torn write could leave (squelch is 0..9 in the vendor
     * format) before trusting the blob. */
    bool blank = true;
    const uint8_t *p = (const uint8_t *) out;
    for(size_t i = 0; i < sizeof(*out); ++i)
        if(p[i] != 0xFFu) { blank = false; break; }

    if(blank || out->squelch > 9)
    {
        hd2_cps_settings_defaults(out);
        return 1;                              /* defaults applied         */
    }
    return 0;
}

int hd2_cps_settings_save(const hd2_cps_settings_t *in)
{
    if(!w25q_hd2_probe()) return -1;
    if(w25q_hd2_eraseSector(VSET_SECTOR) < 0) return -1;
    if(w25q_hd2_program(VSET_SECTOR, in, sizeof(*in)) < 0) return -1;

    /* Read-back verify the first bytes so a silent program failure surfaces. */
    hd2_cps_settings_t check;
    w25q_hd2_read(VSET_SECTOR, &check, sizeof(check));
    if(memcmp(&check, in, sizeof(check)) != 0) return -1;
    return 0;
}
