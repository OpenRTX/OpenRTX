/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Byte-exact layout of the Ailunce HD2 (Dahua) factory codeplug channel and
 * contact records, reverse-engineered in vendor/hd2_infodump/docs/
 * {channels,records}.md.  Companion to hd2_cps_settings.h.
 *
 * These mirror the on-flash VENDOR record formats so firmware can READ the
 * factory channel/contact database (e.g. to import it into the OpenRTX
 * codeplug).  Records live in the vendor "channels" family-0x31 region; the
 * physical W25Q byte offsets are block_index * 0x400 (validated in
 * nvmem_HD2.c): priority contacts at 0x006E1000, channel slots at 0x006F3000.
 * The accessor (hd2_cps_records.c) is READ-ONLY -- the OpenRTX codeplug
 * (cps_io_HD2.c) is the write path; rewriting vendor records in place is out
 * of scope.
 *
 * As in hd2_cps_settings.h, packed-flag bytes use uint8_t + named bit masks
 * (not implementation-defined C bitfields), and _Static_assert pins every
 * field to its documented record offset.
 */

#ifndef HD2_CPS_RECORDS_H
#define HD2_CPS_RECORDS_H

#include <stdint.h>
#include <stddef.h>

/* ================================================================== *
 *  Contact (Priority Contact / Address Book) -- 72 bytes             *
 * ================================================================== */

typedef enum
{
    HD2_CT_GROUP   = 0x04,
    HD2_CT_PRIVATE = 0x05,
    HD2_CT_ALL     = 0x06,
} hd2_contact_type_t;

typedef struct
{
    uint32_t dmr_id;          /* 0x00  LE; 0xFFFFFFFF = all-call          */
    uint8_t  type;            /* 0x04  hd2_contact_type_t                 */
    char     name[16];        /* 0x05  ASCII, null-padded                 */
    char     city[16];        /* 0x15  ASCII (display only)               */
    char     province[16];    /* 0x25  ASCII (display only)               */
    char     country[16];     /* 0x35  ASCII (display only)               */
    uint8_t  term[3];         /* 0x45  0xFF terminator                    */
}
__attribute__((packed)) hd2_vendor_contact_t; /* 0x48 = 72 bytes */

/* ================================================================== *
 *  Channel slot -- 176 bytes (0xB0)                                  *
 * ================================================================== */

/* +0x20 option bits */
#define HD2_CH_O20_TXGPS      0x10u
#define HD2_CH_O20_VOX_EN     0x20u   /* low nibble then = vox_level - 1  */
#define HD2_CH_O20_WORKALONE  0x40u
#define HD2_CH_O20_GPS_EN     0x80u

/* +0x21 option bits (THE mode bit is 0x40) */
#define HD2_CH_O21_SCAN_ADD   0x01u
#define HD2_CH_O21_TALKAROUND 0x02u
#define HD2_CH_O21_POWER_MASK 0x0Cu   /* bits 3:2; + O29? see power note  */
#define HD2_CH_O21_POWER_SHIFT 2u
#define HD2_CH_O21_DMR_MODE   0x40u   /* set=DMR, clear=FM (authoritative)*/
#define HD2_CH_O21_RELAY      0x80u

/* +0x28 encryption / 2nd-TX */
#define HD2_CH_O28_2ND_TX     0x80u
#define HD2_CH_O28_ENCFAM_MASK 0x60u  /* bits 6:5 -> hd2_encfam_t         */
#define HD2_CH_O28_ENCFAM_SHIFT 5u
#define HD2_CH_O28_KEYIDX_MASK 0x0Fu  /* 0..15 (UI 1..16)                 */

/* +0x29 promiscuous / TX-authority / bandwidth */
#define HD2_CH_O29_PROMISC    0x01u   /* RxAll CC                         */
#define HD2_CH_O29_TXAUTH_MASK 0x30u  /* analog TX authority (bits 5:4)   */
#define HD2_CH_O29_BW_WIDE    0x40u   /* set=wide, clear=narrow           */

/* +0x2A color code / timeslot / DMR mode (DMR channels) */
#define HD2_CH_O2A_CC_MASK    0xF0u   /* high nibble = color code 0..15   */
#define HD2_CH_O2A_CC_SHIFT   4u
#define HD2_CH_O2A_TS2        0x01u   /* set=Slot 2, clear=Slot 1         */
#define HD2_CH_O2A_DMRMODE_MASK 0x0Au /* Simplex 0x00 / Repeater 0x02 / Double 0x08 */

/* +0x2B */
#define HD2_CH_O2B_RXGPSINFO  0x20u
#define HD2_CH_O2B_BUSYLOCK_MASK 0xC0u /* bits 7:6, hd2_busylock_t        */
#define HD2_CH_O2B_BUSYLOCK_SHIFT 6u

typedef enum { HD2_PWR_LOW = 0, HD2_PWR_MID = 1, HD2_PWR_HIGH = 2, HD2_PWR_XLOW = 3 } hd2_power_t;
typedef enum { HD2_ENCFAM_OFF = 0, HD2_ENCFAM_NORMAL = 1, HD2_ENCFAM_ENHANCED = 2, HD2_ENCFAM_AES = 3 } hd2_encfam_t;
typedef enum { HD2_DMRMODE_SIMPLEX = 0x00, HD2_DMRMODE_REPEATER = 0x02, HD2_DMRMODE_DOUBLE = 0x08 } hd2_dmrmode_t;
typedef enum { HD2_BUSY_FORBID = 0, HD2_BUSY_IMPOLITE = 1, HD2_BUSY_POLITE_CC = 2, HD2_BUSY_POLITE_ALL = 3 } hd2_busylock_t;

#define HD2_CH_RXLIST_MAX  32u
#define HD2_CH_RXLIST_END  0xFFFFFFFFu  /* terminator sentinel */

typedef struct
{
    uint8_t  marker[4];       /* 0x00  0xFFFFFFFF for a populated slot     */
    char     name[10];        /* 0x04  ASCII, null/0xff-padded             */
    uint8_t  pad0[2];         /* 0x0E                                      */
    uint8_t  aux[4];          /* 0x10  DTMF idx (+0x10 bits6:0), PTT-ID/   */
                              /*       enc-variant (+0x11), GPS contact    */
                              /*       (+0x12); FM mode-aux mirror 00 20.. */
    uint8_t  rx_freq[4];      /* 0x14  BCD LE, x 10 Hz                     */
    uint8_t  tx_freq[4];      /* 0x18  BCD LE, x 10 Hz                     */
    uint32_t contact_id;      /* 0x1C  LE 24-bit DMR ID (TX contact)       */
    uint8_t  opt20;           /* 0x20  TxGPS/VOX/WorkAlone/GPS             */
    uint8_t  opt21;           /* 0x21  scan/talkaround/power/MODE/relay    */
    uint8_t  gps_timing;      /* 0x22  (seconds - 20)/10; 0=off            */
    uint8_t  tot;             /* 0x23  time-out timer, units of 15 s       */
    uint8_t  rx_tone[2];      /* 0x24  CTCSS/DCS (0xFFFF=none; FM)         */
    uint8_t  tx_tone[2];      /* 0x26  CTCSS/DCS (0xFFFF=none; FM)         */
    uint8_t  opt28;           /* 0x28  enc family/2nd-TX/key index         */
    uint8_t  opt29;           /* 0x29  promisc/TX-authority/bandwidth      */
    uint8_t  cc_ts_dmr;       /* 0x2A  color code + timeslot + DMR mode    */
    uint8_t  opt2b;           /* 0x2B  RxGPSInfo/busy-lock                 */
    uint8_t  kill_code;       /* 0x2C  1-based priority-contact idx        */
    uint8_t  pad1;            /* 0x2D                                      */
    uint8_t  wakeup_code;     /* 0x2E  1-based priority-contact idx        */
    uint8_t  pad2;            /* 0x2F                                      */
    uint32_t rx_list[HD2_CH_RXLIST_MAX]; /* 0x30..0xAF inline Rx List, LE */
}
__attribute__((packed)) hd2_vendor_channel_t; /* 0xB0 = 176 bytes */

/* ---- layout guards ------------------------------------------------- */
_Static_assert(sizeof(hd2_vendor_contact_t) == 72,  "contact must be 72 bytes");
_Static_assert(offsetof(hd2_vendor_contact_t, type)    == 0x04, "contact type off");
_Static_assert(offsetof(hd2_vendor_contact_t, name)    == 0x05, "contact name off");
_Static_assert(offsetof(hd2_vendor_contact_t, country) == 0x35, "contact country off");
_Static_assert(sizeof(hd2_vendor_channel_t) == 0xB0, "channel must be 176 bytes");
_Static_assert(offsetof(hd2_vendor_channel_t, rx_freq)    == 0x14, "ch rx_freq off");
_Static_assert(offsetof(hd2_vendor_channel_t, contact_id) == 0x1C, "ch contact off");
_Static_assert(offsetof(hd2_vendor_channel_t, opt21)      == 0x21, "ch opt21 off");
_Static_assert(offsetof(hd2_vendor_channel_t, cc_ts_dmr)  == 0x2A, "ch cc/ts off");
_Static_assert(offsetof(hd2_vendor_channel_t, rx_list)    == 0x30, "ch rxlist off");

/* ---- decode helpers ------------------------------------------------ */

/* 4-byte little-endian packed-BCD frequency field -> Hz.  The field stores
 * 8 BCD digits, value x 10 Hz (e.g. 43 00 00 00 ... per nibble order). */
static inline uint32_t hd2_bcd4_to_hz(const uint8_t f[4])
{
    uint32_t v = 0;
    for(int i = 3; i >= 0; --i)
        v = v * 100u + ((f[i] >> 4) * 10u) + (f[i] & 0x0Fu);
    return v * 10u;
}

/* Inverse of hd2_bcd4_to_hz: Hz -> 4-byte little-endian packed-BCD field
 * (8 BCD digits of the value / 10). */
static inline void hd2_hz_to_bcd4(uint32_t hz, uint8_t f[4])
{
    uint32_t v = hz / 10u;
    for(int i = 0; i < 4; ++i)
    {
        uint32_t pair = v % 100u;
        v /= 100u;
        f[i] = (uint8_t)(((pair / 10u) << 4) | (pair % 10u));
    }
}

static inline int hd2_channel_is_dmr(const hd2_vendor_channel_t *c)
{ return (c->opt21 & HD2_CH_O21_DMR_MODE) != 0; }

static inline int hd2_channel_is_wide(const hd2_vendor_channel_t *c)
{ return (c->opt29 & HD2_CH_O29_BW_WIDE) != 0; }

/* ================================================================== *
 *  Read-only accessors (hd2_cps_records.c)                            *
 * ================================================================== */
#ifdef __cplusplus
extern "C" {
#endif

/* Read contact / channel `index` (0-based, global) from the vendor flash.
 * Returns 0 on success, -1 on flash error or out-of-range.  A populated
 * channel slot has marker == 0xFFFFFFFF and a non-blank name; callers should
 * treat an all-0xFF / blank-name record as "empty" and skip it. */
int hd2_vendor_contact_read(uint16_t index, hd2_vendor_contact_t *out);
int hd2_vendor_channel_read(uint16_t index, hd2_vendor_channel_t *out);

/* True if the record looks populated (not an erased / blank slot). */
int hd2_vendor_channel_present(const hd2_vendor_channel_t *c);
int hd2_vendor_contact_present(const hd2_vendor_contact_t *c);

/* Write channel `index` back to the vendor flash (read-modify-write of the
 * 4 kB sector(s) the 176-byte record straddles).  Used by the vendor-backed
 * codeplug write path (cps_io_HD2.c); the contact records stay read-only.
 * Returns 0 on success, -1 on flash error. */
int hd2_vendor_channel_write(uint16_t index, const hd2_vendor_channel_t *in);

#ifdef __cplusplus
}
#endif

#endif /* HD2_CPS_RECORDS_H */
