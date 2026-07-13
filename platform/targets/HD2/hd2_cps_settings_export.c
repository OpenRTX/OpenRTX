/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * HD2 strong override of the native->vendor CPS settings adapter (interfaces/
 * vendor_cps_settings.h): map the OpenRTX settings_t into the Ailunce/Dahua
 * vendor settings block (hd2_cps_settings.h) and persist it to the vendor-format
 * W25Q sector (nvmem_vendor_settings_HD2.c).  The radio-settings-block
 * counterpart of the vendor-backed channel codeplug (cps_io_HD2.c).
 *
 * settings_t is a small subset of the vendor block, so this is a LOAD-MODIFY-
 * SAVE: load the stored vendor blob (defaults on a blank sector), overlay only
 * the fields OpenRTX owns, and write it back.  Vendor-only fields -- the
 * programmable/emergency keys, zones, power-on password, VHF/UHF scan ranges,
 * mic gain, VOX delay -- are left untouched so a round-trip to the vendor PC
 * tool preserves them.
 *
 * Mapped fields (the honest native<->vendor overlap):
 *   sqlLevel       -> squelch         (OpenRTX 0..15  -> vendor 0..9)
 *   brightness     -> brightness      (OpenRTX 5..100 -> vendor level 0..9)
 *   display_timer  -> backlight       (standby enum   -> vendor seconds, 0..120)
 *   vpLevel        -> flags1 Voice bit (speech beyond a beep -> announcements on)
 * Vendor ranges are from vendor/hd2_infodump/docs/settings.md.
 */

#include "interfaces/vendor_cps_settings.h"
#include "core/settings.h"
#include "core/voicePrompts.h"   /* vpLevel enum (vpBeep, ...) */
#include "hd2_cps_settings.h"
#include <stdint.h>

/* ---- HD2 vendor CPS settings ranges (settings.md) ------------------- */
#define HD2_SQUELCH_MAX      9    /* 0x2970: direct 0..9                   */
#define HD2_BRIGHTNESS_MAX   9    /* 0x299C: 10 levels, 0-indexed (0..9)   */
#define HD2_BACKLIGHT_MAX_S  120  /* 0x2972: seconds; 0=continuous, max120 */
#define HD2_MIC_GAIN_MIN   (-10)  /* 0x299B: signed -10..+10               */
#define HD2_MIC_GAIN_MAX     10
#define HD2_VOX_DELAY_MAX    15   /* 0x2976 bits 3:0: x0.5 s (0..15)       */

/* OpenRTX-native source ranges (ui.c clamps). */
#define ORTX_SQL_MAX         15   /* sqlLevel 0..15                        */
#define ORTX_BR_MIN          5    /* brightness 5..100                     */
#define ORTX_BR_MAX          100

static const cps_settings_limits_t hd2_limits =
{
    .squelch_max     = HD2_SQUELCH_MAX,
    .brightness_max  = HD2_BRIGHTNESS_MAX,
    .backlight_max_s = HD2_BACKLIGHT_MAX_S,
    .mic_gain_min    = HD2_MIC_GAIN_MIN,
    .mic_gain_max    = HD2_MIC_GAIN_MAX,
    .vox_delay_max   = HD2_VOX_DELAY_MAX,
};

/* Scale v in [0, in_max] onto [0, out_max], rounded to nearest. */
static uint8_t scale_round(uint32_t v, uint32_t in_max, uint32_t out_max)
{
    if(v > in_max) v = in_max;
    return (uint8_t)((v * out_max + in_max / 2u) / in_max);
}

/* OpenRTX standby-timer enum -> vendor backlight timeout in seconds.  The
 * vendor field tops out at 120 s; longer OpenRTX timers clamp there.  Both
 * sides use "0 = stays on" (OpenRTX TIMER_OFF == vendor continuous). */
static uint16_t display_timer_seconds(uint8_t dt)
{
    switch(dt)
    {
        case TIMER_OFF: return 0;
        case TIMER_5S:  return 5;
        case TIMER_10S: return 10;
        case TIMER_15S: return 15;
        case TIMER_20S: return 20;
        case TIMER_25S: return 25;
        case TIMER_30S: return 30;
        case TIMER_1M:  return 60;
        case TIMER_2M:  return 120;
        default:        return HD2_BACKLIGHT_MAX_S;   /* >2 min: clamp */
    }
}

static void overlay_ortx_onto_vendor(const settings_t *s, hd2_cps_settings_t *v)
{
    /* Squelch: OpenRTX 0..15 -> vendor 0..9. */
    v->squelch = scale_round(s->sqlLevel, ORTX_SQL_MAX, HD2_SQUELCH_MAX);

    /* Brightness: OpenRTX 5..100 -> vendor level 0..9. */
    uint32_t br = s->brightness;
    if(br < ORTX_BR_MIN) br = ORTX_BR_MIN;
    v->brightness = scale_round(br - ORTX_BR_MIN, ORTX_BR_MAX - ORTX_BR_MIN,
                                HD2_BRIGHTNESS_MAX);

    /* Backlight standby timer: OpenRTX display_timer enum -> vendor seconds. */
    uint16_t secs = display_timer_seconds(s->display_timer);
    if(secs > HD2_BACKLIGHT_MAX_S) secs = HD2_BACKLIGHT_MAX_S;
    v->backlight = (uint8_t) secs;

    /* Voice announcements (flags1 bit5): on iff OpenRTX speaks beyond a beep. */
    if(s->vpLevel > vpBeep) v->flags1 |=  HD2_VSET_F1_VOICE;
    else                    v->flags1 &= (uint8_t) ~HD2_VSET_F1_VOICE;
}

/* ---- interfaces/vendor_cps_settings.h strong overrides -------------- */

bool vendor_cps_settings_supported(void)
{
    return true;
}

const cps_settings_limits_t *vendor_cps_settings_limits(void)
{
    return &hd2_limits;
}

int vendor_cps_settings_export(const settings_t *s)
{
    if(s == NULL) return -1;

    /* Load-modify-save: preserve every vendor-only field across the round-trip
     * (load fills defaults on a blank/invalid sector, which is fine to overlay). */
    hd2_cps_settings_t v;
    (void) hd2_cps_settings_load(&v);

    overlay_ortx_onto_vendor(s, &v);

    return hd2_cps_settings_save(&v);
}
