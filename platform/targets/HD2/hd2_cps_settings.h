/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Byte-exact layout of the Ailunce HD2 (Dahua) factory codeplug "radio
 * settings" sub-block, reverse-engineered in vendor/hd2_infodump/docs/
 * settings.md (bytes isolated by toggling one CPS setting at a time and
 * diffing flash dumps).
 *
 * This mirrors the on-flash VENDOR format so firmware can read/write the
 * factory settings the radio's own menu uses -- it is NOT the OpenRTX
 * settings_t (core/settings.h).  The settings region is radio-address
 * 0x2900..0x5F00 (vendor "b1=0x0F" 128-byte-read space); this struct covers
 * the contiguous radio-settings core at 0x2970..0x29D0 (96 bytes).  The
 * larger sibling sub-blocks in the same region -- FM presets (0x3000),
 * encryption keys (0x3D00/0x3E00), radio-ID table (0x4000), DTMF (0x4400) --
 * are separate structures, declared as TODO at the bottom.
 *
 * Packed-flag bytes are kept as plain uint8_t with HD2_VSET_*_<name> bit
 * masks (NOT C bitfields, whose ordering is implementation-defined) so the
 * layout matches the documented bit numbers exactly on any compiler.  The
 * _Static_assert block at the end pins every field to its documented vendor
 * address; if a field is mis-sized the build fails rather than silently
 * writing the wrong byte to the radio's flash.
 */

#ifndef HD2_CPS_SETTINGS_H
#define HD2_CPS_SETTINGS_H

#include <stdint.h>
#include <stddef.h>

/* Radio-address anchors (vendor logical addressing). */
#define HD2_VSET_REGION_ADDR  0x2900u   /* settings region base            */
#define HD2_VSET_CORE_ADDR    0x2970u   /* first documented settings byte  */

/* ---- enums (multi-value fields) ----------------------------------- */

/* VFO frequency step (byte 0x297F).  Note: 20 kHz is out of sequence at
 * 0x08 -- appended to firmware later than the other seven. */
typedef enum
{
    HD2_STEP_2_5K  = 0x00, HD2_STEP_5K   = 0x01, HD2_STEP_6_25K = 0x02,
    HD2_STEP_10K   = 0x03, HD2_STEP_12_5K= 0x04, HD2_STEP_25K   = 0x05,
    HD2_STEP_30K   = 0x06, HD2_STEP_50K  = 0x07, HD2_STEP_20K   = 0x08,
} hd2_step_t;

/* Scan mode (byte 0x2973 bits 1:0). */
typedef enum { HD2_SCAN_TO = 0, HD2_SCAN_CO = 1, HD2_SCAN_SE = 2 } hd2_scanmode_t;

/* Power-save ratio (byte 0x2973 bits 7:5). */
typedef enum
{
    HD2_SAVE_OFF = 0, HD2_SAVE_1_1 = 1, HD2_SAVE_1_2 = 2,
    HD2_SAVE_1_3 = 3, HD2_SAVE_1_4 = 4,
} hd2_save_t;

/* Power-save wake speed (byte 0x2976 bits 5:4). */
typedef enum { HD2_WAKE_FAST = 0, HD2_WAKE_MED = 1, HD2_WAKE_SLOW = 2 } hd2_wakespeed_t;

/* CH-Mode display (byte 0x2978, 2-bit enum {bit5,bit0}). */
typedef enum { HD2_CHMODE_FREQ = 0, HD2_CHMODE_CH = 1, HD2_CHMODE_NAME = 2 } hd2_chmode_t;

/* Lock mode (byte 0x299F bits 6:5). */
typedef enum { HD2_LOCK_KEY = 0, HD2_LOCK_KEY_CH = 1, HD2_LOCK_KEY_CH_PTT = 2 } hd2_lockmode_t;

/* Programmable-key / emergency-key function (bytes 0x29A8..0x29AB and
 * 0x29AE/0x29AF).  Full enum from CPS diffs + the v208 menu pointer table. */
typedef enum
{
    HD2_KEYFN_OFF = 0x00, HD2_KEYFN_POWER = 0x01, HD2_KEYFN_SCAN = 0x02,
    HD2_KEYFN_RADIO_FM = 0x03, HD2_KEYFN_WAKEUP = 0x04, HD2_KEYFN_RELAY = 0x05,
    HD2_KEYFN_KEYCALL1 = 0x06, HD2_KEYFN_KEYCALL2 = 0x07, HD2_KEYFN_KEYCALL3 = 0x08,
    HD2_KEYFN_KEYCALL4 = 0x09, HD2_KEYFN_KEYCALL5 = 0x0A, HD2_KEYFN_KEYCALL6 = 0x0B,
    HD2_KEYFN_VOX = 0x0C, HD2_KEYFN_STUN = 0x0D, HD2_KEYFN_KILL = 0x0E,
    HD2_KEYFN_TX_DSW = 0x0F, HD2_KEYFN_MONITOR = 0x10, HD2_KEYFN_TX1000 = 0x11,
    HD2_KEYFN_TX1450 = 0x12, HD2_KEYFN_TX1750 = 0x13, HD2_KEYFN_TX2100 = 0x14,
    HD2_KEYFN_ZONE_PLUS = 0x15, HD2_KEYFN_ZONE_MINUS = 0x16, HD2_KEYFN_DMR_SLOT = 0x17,
    HD2_KEYFN_PROMISC = 0x18, HD2_KEYFN_MANUAL_DIAL = 0x19, HD2_KEYFN_CHMODE = 0x1A,
    HD2_KEYFN_REVERSE = 0x1B, HD2_KEYFN_BLUETOOTH = 0x1C, HD2_KEYFN_HALF_W = 0x1D,
    HD2_KEYFN_FM_CALL = 0x1E, HD2_KEYFN_VOLTAGE = 0x1F, HD2_KEYFN_NOAA = 0x20,
    HD2_KEYFN_EMERGENCY = 0x21, HD2_KEYFN_KEYBEEP = 0x22, HD2_KEYFN_ENCRYPTION = 0x23,
    HD2_KEYFN_FACTORY_OFF = 0xFF,
} hd2_keyfn_t;

/* ---- packed-flag bit masks (named per settings.md) ----------------- */

/* flags1 @ 0x2971 */
#define HD2_VSET_F1_VOICE          0x20u  /* voice announcements          */
#define HD2_VSET_F1_AUTO_KEYLOCK   0x04u  /* auto key lock after 15 s     */
#define HD2_VSET_F1_ROGER          0x02u  /* transmit end (roger) tone    */
#define HD2_VSET_F1_KEYLOCK_PWRON  0x01u  /* key lock at power on         */

/* flags2 @ 0x2973 */
#define HD2_VSET_F2_SAVE_SHIFT     5u     /* bits 7:5 -> hd2_save_t       */
#define HD2_VSET_F2_SAVE_MASK      0xE0u
#define HD2_VSET_F2_DOUBLE_PTT     0x08u
#define HD2_VSET_F2_RADIO_DW       0x10u  /* FM-broadcast dual watch      */
#define HD2_VSET_F2_FMWORK_VFO     0x04u  /* set=VFO, clear=CH            */
#define HD2_VSET_F2_SCANMODE_MASK  0x03u  /* bits 1:0 -> hd2_scanmode_t   */

/* flags3 @ 0x2975 */
#define HD2_VSET_F3_ACCEPT_WAKE    0x80u
#define HD2_VSET_F3_ACCEPT_KILL    0x40u
#define HD2_VSET_F3_FM_TX_BEEP     0x20u
#define HD2_VSET_F3_DMR_TX_BEEP    0x10u
#define HD2_VSET_F3_NO_TX_INFO     0x02u
#define HD2_VSET_F3_ZONE_NAME      0x01u

/* wakeup_vox @ 0x2976 */
#define HD2_VSET_WV_WAKESPD_SHIFT  4u     /* bits 5:4 -> hd2_wakespeed_t  */
#define HD2_VSET_WV_WAKESPD_MASK   0x30u
#define HD2_VSET_WV_VOXDELAY_MASK  0x0Fu  /* bits 3:0, value x 0.5 s      */

/* flags4 @ 0x2977 */
#define HD2_VSET_F4_KEY_BEEP       0x80u
#define HD2_VSET_F4_PTT_CANCEL_VOX 0x20u
#define HD2_VSET_F4_HEADSET_VOX    0x10u
#define HD2_VSET_F4_PWRON_PASSWD   0x08u
#define HD2_VSET_F4_NOISE_TAIL     0x04u
#define HD2_VSET_F4_DW_BDR_OFF     0x02u  /* INVERTED: set = off          */
#define HD2_VSET_F4_NTAIL_REVCODE  0x01u

/* flags5 @ 0x2978 (CH-Mode 2-bit enum = {bit5,bit0}) */
#define HD2_VSET_F5_CHMODE_HI      0x20u  /* bit5 of hd2_chmode_t         */
#define HD2_VSET_F5_NIGHT_MODE     0x04u
#define HD2_VSET_F5_CHMODE_LO      0x01u  /* bit0 of hd2_chmode_t         */

/* flags6 @ 0x2979 */
#define HD2_VSET_F6_TIME_24H       0x08u  /* set=24h, clear=12h           */
#define HD2_VSET_F6_LANG_ENGLISH   0x04u  /* set=English, clear=Chinese   */
#define HD2_VSET_F6_RADIO_FUNCS    0x01u

/* flags7 @ 0x299D */
#define HD2_VSET_F7_SD_MODE        0x04u  /* single/dual band standby     */
#define HD2_VSET_F7_EALARM_LOCAL   0x02u  /* clear=Remote, set=Local      */
#define HD2_VSET_F7_MISS_CALL      0x01u

/* flags8 @ 0x299F */
#define HD2_VSET_F8_RXINFO_BRIGHT  0x80u
#define HD2_VSET_F8_LOCKMODE_SHIFT 5u     /* bits 6:5 -> hd2_lockmode_t   */
#define HD2_VSET_F8_LOCKMODE_MASK  0x60u
#define HD2_VSET_F8_VFO_LOCK       0x10u
#define HD2_VSET_F8_BAND_A_PWRON   0x01u  /* set=A freq, clear=B freq     */

/* ---- the settings core (radio addr 0x2970..0x29D0) ----------------- */

typedef struct
{
    uint8_t squelch;              /* 0x2970  Main#1, 0..9                  */
    uint8_t flags1;               /* 0x2971  see HD2_VSET_F1_*             */
    uint8_t backlight;            /* 0x2972  Main#14, seconds (0=cont.)    */
    uint8_t flags2;               /* 0x2973  see HD2_VSET_F2_*             */
    uint8_t ab_time;              /* 0x2974  Main#3, 1..10 s               */
    uint8_t flags3;               /* 0x2975  see HD2_VSET_F3_*             */
    uint8_t wakeup_vox;           /* 0x2976  wake speed + VOX delay        */
    uint8_t flags4;               /* 0x2977  see HD2_VSET_F4_*             */
    uint8_t flags5;               /* 0x2978  CH-Mode enum + night mode     */
    uint8_t flags6;               /* 0x2979  see HD2_VSET_F6_*             */
    uint8_t priv_group_call_resp; /* 0x297A  0..10 (0=OFF)                 */
    uint8_t _resv0[2];            /* 0x297B..0x297C                        */
    uint8_t fm_radio_work_ch;     /* 0x297D  0-based FM preset index       */
    uint8_t _resv1;               /* 0x297E                                */
    uint8_t step;                 /* 0x297F  hd2_step_t                    */
    uint8_t _resv2[5];            /* 0x2980..0x2984                        */
    uint8_t shift_freq[3];        /* 0x2985  Ch#18, BCD LE kHz             */
    uint8_t repeater_connect;     /* 0x2988  1..10                         */
    uint8_t power_on_password[6]; /* 0x2989  6 ASCII digits                */
    uint8_t _resv3;               /* 0x298F                                */
    uint8_t alarm_tx_time;        /* 0x2990  value / 2 (s)                 */
    uint8_t alarm_idle_time;      /* 0x2991  value - 5 (s)                 */
    uint8_t _resv4[6];            /* 0x2992..0x2997                        */
    uint8_t lone_worker_resp;     /* 0x2998  value - 1                     */
    uint8_t lone_worker_prealarm; /* 0x2999  value - 1                     */
    uint8_t rx_info_disp_time;    /* 0x299A  value - 1 (s)                 */
    int8_t  mic_gain;             /* 0x299B  Main#10, signed -10..+10      */
    uint8_t brightness;           /* 0x299C  Main#15, 0-indexed (0=lvl 1)  */
    uint8_t flags7;               /* 0x299D  see HD2_VSET_F7_*             */
    uint8_t menu_exit;            /* 0x299E  Main#26, seconds (0=OFF)      */
    uint8_t flags8;               /* 0x299F  see HD2_VSET_F8_*             */
    uint8_t version_byte;         /* 0x29A0  unknown counter; safe = 0x00  */
    uint8_t _resv5[2];            /* 0x29A1..0x29A2                        */
    uint8_t radio_id;             /* 0x29A3  active Radio-ID index         */
    uint8_t default_zone_a;       /* 0x29A4  0-based zone index            */
    uint8_t _resv6;               /* 0x29A5                                */
    uint8_t default_zone_b;       /* 0x29A6  0-based zone index            */
    uint8_t _resv7;               /* 0x29A7                                */
    uint8_t key_define[4];        /* 0x29A8  Key1S,Key1L,Key2S,Key2L (hd2_keyfn_t) */
    uint8_t priority_scan_ch;     /* 0x29AC  0-based channel index         */
    uint8_t _resv8;               /* 0x29AD                                */
    uint8_t emergency_key_short;  /* 0x29AE  hd2_keyfn_t (default 0x21)    */
    uint8_t emergency_key_long;   /* 0x29AF  hd2_keyfn_t (default 0x21)    */
    uint8_t _resv9[16];           /* 0x29B0..0x29BF                        */
    uint8_t vhf_scan_range[4];    /* 0x29C0  start+stop, BCD LE x100 kHz   */
    uint8_t _resv10[4];           /* 0x29C4..0x29C7 (0xFF in samples)      */
    uint8_t uhf_scan_range[4];    /* 0x29C8  start+stop, BCD LE x100 kHz   */
    uint8_t _resv11[4];           /* 0x29CC..0x29CF (0xFF in samples)      */
}
__attribute__((packed)) hd2_cps_settings_t; /* 96 bytes; ends at 0x29D0 */

/* ---- layout guards: each field pinned to its documented vendor addr -- */
#define HD2_VSET_OFF(field) (offsetof(hd2_cps_settings_t, field) + HD2_VSET_CORE_ADDR)
_Static_assert(sizeof(hd2_cps_settings_t) == 0x60, "settings core must be 96 bytes");
_Static_assert(HD2_VSET_OFF(squelch)             == 0x2970, "squelch offset");
_Static_assert(HD2_VSET_OFF(backlight)           == 0x2972, "backlight offset");
_Static_assert(HD2_VSET_OFF(step)                == 0x297F, "step offset");
_Static_assert(HD2_VSET_OFF(shift_freq)          == 0x2985, "shift_freq offset");
_Static_assert(HD2_VSET_OFF(power_on_password)   == 0x2989, "password offset");
_Static_assert(HD2_VSET_OFF(mic_gain)            == 0x299B, "mic_gain offset");
_Static_assert(HD2_VSET_OFF(brightness)          == 0x299C, "brightness offset");
_Static_assert(HD2_VSET_OFF(flags8)              == 0x299F, "flags8 offset");
_Static_assert(HD2_VSET_OFF(key_define)          == 0x29A8, "key_define offset");
_Static_assert(HD2_VSET_OFF(emergency_key_short) == 0x29AE, "emergency key offset");
_Static_assert(HD2_VSET_OFF(vhf_scan_range)      == 0x29C0, "vhf scan offset");
_Static_assert(HD2_VSET_OFF(uhf_scan_range)      == 0x29C8, "uhf scan offset");

/* ---- TODO: sibling sub-blocks in the same settings region ----------
 * (declare as their own packed structs when wired)
 *   0x29D0  Key Calls 1-6      (7-byte stride x 6)
 *   0x3000  FM Radio presets   (32 channels, 136 B)
 *   0x3D00  Normal Enc keys    (2 B BE BCD x 16)
 *   0x3E00  Enhanced Enc keys  (16 B x 16)
 *   0x4000  Radio-ID table     (20 B stride: 4 B DMR ID LE + 16 B name)
 *   0x4400  DTMF config + codes
 *   0x5018  AES/ARC4 keys      (49 B stride x 48)
 * ------------------------------------------------------------------- */

/* ---- accessor (nvmem_vendor_settings_HD2.c) ------------------------
 * The vendor-format settings blob is stored in an OpenRTX-managed W25Q
 * sector (0x00FEE000), NOT at the live vendor flash location: the vendor
 * physical mapping behind its block protocol (settings.md radio addr
 * 0x2900) is not reliably known, and this radio runs OpenRTX.  The struct
 * keeps vendor *format* compatibility (round-trippable to a vendor CPS tool)
 * while the storage is ours.
 */
#ifdef __cplusplus
extern "C" {
#endif

/* Fill `out` with sane vendor-equivalent defaults (used on a blank sector). */
void hd2_cps_settings_defaults(hd2_cps_settings_t *out);

/* Read the stored blob into `out`.  On a blank/invalid sector, fills
 * defaults and returns 1 (caller may choose to save).  Returns 0 on a
 * valid read, -1 on flash error. */
int  hd2_cps_settings_load(hd2_cps_settings_t *out);

/* Erase the sector and program `in`.  Returns 0 on success, -1 on error. */
int  hd2_cps_settings_save(const hd2_cps_settings_t *in);

#ifdef __cplusplus
}
#endif

#endif /* HD2_CPS_SETTINGS_H */
