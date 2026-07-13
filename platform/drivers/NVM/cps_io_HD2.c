/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Vendor-backed OpenRTX codeplug (cps_io.h) for the Ailunce HD2: the UI reads
 * AND writes the radio's REAL (Ailunce/Dahua) vendor channel records on the
 * W25Q512 directly -- no separate OpenRTX-native ("RTXC") store and no import
 * step.  Channels translate on the fly between the 176-byte vendor record
 * (hd2_cps_records.h) and OpenRTX channel_t; edits read-modify-write the slot
 * back into the live factory region via hd2_vendor_channel_write().
 *
 * SLOT LAYOUT (factory codeplug): vendor slot 0 = VFO A default, slot 1 = VFO
 * B default, slots 2+ = real channels.  So OpenRTX channel index `pos` maps to
 * vendor slot `pos + 2`; the VFO slots are not exposed as channels.
 *
 * Scope: channel read + write + insert/delete are live (the translation is
 * validated byte-exact against cp_ai5qz_germany.bin via
 * scripts/cps_channel_roundtrip.py).  insert/delete shift the records in place
 * and also rewrite the vendor channel-presence bitmap (real-channel indexing,
 * dual copy) so the edit is visible to the vendor CPS/firmware too.  The
 * bitmap's physical flash base (HD2_BITMAP_BASE = 0x792000) is firmware-
 * confirmed and live-verified.  Priority contacts and zones are READ-only;
 * their writes return -1 -- a tracked follow-up.
 *
 * cps_create() reports ENOTSUP: we never author the vendor format from scratch
 * (that is the Ailunce CPS's job) and never erase the factory codeplug wholesale.
 *
 * Flash access goes through the shared flash_w25q_HD2 driver (HW SPI0); the
 * channel records use the hd2_vendor_channel_read/write accessors
 * (nvmem_vendor_records_HD2.c), contacts use hd2_vendor_contact_read, and zones
 * are read directly from their region base.
 */

#include "interfaces/cps_io.h"
#include "hd2_cps_records.h"
#include "flash_w25q_HD2.h"
#include "core/cps.h"
#include "rtx/rtx.h"            /* OPMODE_* + BW_* enums */
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <errno.h>

/* OpenRTX channel index -> vendor slot: skip the two VFO-default slots. */
#define HD2_VFO_SLOTS       2u
#define HD2_CHAN_MAX        3000u   /* UI iteration cap */

/* Zones (vendor "banks"): 145-byte (0x91) records, densely packed from the
 * region base (address-space.md / records.md).  No records-accessor helper, so
 * read them here. */
#define HD2_ZONE_BASE       0x00880000u
#define HD2_ZONE_STRIDE     0x91u
#define HD2_ZONE_MAX        256u
#define HD2_ZONE_CHMAX      64u

/* ================================================================== *
 *  channel_t  <->  hd2_vendor_channel_t  (mapped fields only)        *
 *  (was translate_channel / untranslate_channel in the import/export)*
 * ================================================================== */

/* vendor 2-byte sub-audio tone -> fmInfo tone (enable/index/type). */
static void decode_tone(const uint8_t t[2], uint8_t *en, uint8_t *idx, uint8_t *type)
{
    *en = 0; *idx = 0; *type = TONE_CTCSS;

    uint16_t raw = (uint16_t) t[0] | ((uint16_t) t[1] << 8);
    if(raw == 0xFFFFu) return;                       /* none */

    if((t[1] & 0x80u) != 0u)
    {
        uint16_t code = (uint16_t)((t[1] & 0x0Fu) * 64u
                                 + ((t[0] >> 4) & 0x0Fu) * 8u
                                 + ( t[0]       & 0x0Fu));
        for(uint8_t i = 0; i < DCS_CODE_NUM; ++i)
            if(dcs_code[i] == code)
            {
                *en = 1; *idx = i;
                *type = ((t[1] & 0xC0u) == 0xC0u) ? TONE_DCS_I : TONE_DCS_N;
                return;
            }
        return;
    }

    uint16_t freq = ((raw >> 12) & 0xFu) * 1000u + ((raw >> 8) & 0xFu) * 100u
                  + ((raw >>  4) & 0xFu) *   10u + ( raw       & 0xFu);
    for(uint8_t i = 0; i < CTCSS_FREQ_NUM; ++i)
        if(ctcss_tone[i] == freq) { *en = 1; *idx = i; return; }
}

/* fmInfo tone -> vendor 2-byte field (inverse of decode_tone). */
static void encode_tone(uint8_t en, uint8_t idx, uint8_t type, uint8_t out[2])
{
    if(!en) { out[0] = 0xFFu; out[1] = 0xFFu; return; }

    if(type == TONE_DCS_N || type == TONE_DCS_I)
    {
        uint16_t code = dcs_code[idx];
        out[0] = (uint8_t)((((code / 8u) % 8u) << 4) | (code % 8u));
        out[1] = (uint8_t)(((type == TONE_DCS_I) ? 0xC0u : 0x80u) | ((code / 64u) & 0x0Fu));
        return;
    }

    uint16_t f = ctcss_tone[idx];
    uint16_t raw = (uint16_t)((((f / 1000u) % 10u) << 12) | (((f / 100u) % 10u) << 8)
                            | (((f /   10u) % 10u) <<  4) |  ( f          % 10u));
    out[0] = (uint8_t)(raw & 0xFFu);
    out[1] = (uint8_t)(raw >> 8);
}

static void vendor_to_channel(const hd2_vendor_channel_t *v, channel_t *o)
{
    memset(o, 0, sizeof(*o));

    o->mode      = hd2_channel_is_dmr(v)  ? OPMODE_DMR : OPMODE_FM;
    o->bandwidth = hd2_channel_is_wide(v) ? BW_25 : BW_12_5;
    o->rx_only   = 0;

    uint8_t pw = (v->opt21 & HD2_CH_O21_POWER_MASK) >> HD2_CH_O21_POWER_SHIFT;
    switch(pw)
    {
        case HD2_PWR_HIGH: o->power = 5000; break;
        case HD2_PWR_MID:  o->power = 2500; break;
        case HD2_PWR_XLOW: o->power =  100; break;  /* UI tx_power_levels[0]=100 ("E"); 500 shows '?' */
        default:           o->power = 1000; break;
    }

    o->rx_frequency = hd2_bcd4_to_hz(v->rx_freq);
    o->tx_frequency = hd2_bcd4_to_hz(v->tx_freq);

    unsigned n = 0;
    for(; n < 10u && v->name[n] != 0 && (uint8_t) v->name[n] != 0xFFu; ++n)
        o->name[n] = v->name[n];
    o->name[n] = 0;

    if(o->mode == OPMODE_DMR)
    {
        uint8_t cc = (v->cc_ts_dmr & HD2_CH_O2A_CC_MASK) >> HD2_CH_O2A_CC_SHIFT;
        o->dmr.rxColorCode   = cc;
        o->dmr.txColorCode   = cc;
        o->dmr.dmr_timeslot  = (v->cc_ts_dmr & HD2_CH_O2A_TS2) ? 2 : 1;
        o->dmr.contact_index = 0;        /* contact-table map: follow-up */
    }
    else
    {
        uint8_t en, idx, type;
        decode_tone(v->rx_tone, &en, &idx, &type);
        o->fm.rxToneEn = en; o->fm.rxTone = idx; o->fm.rxToneType = type;
        decode_tone(v->tx_tone, &en, &idx, &type);
        o->fm.txToneEn = en; o->fm.txTone = idx; o->fm.txToneType = type;
    }
}

/* Overlay the OpenRTX-owned fields of `o` onto the (loaded) vendor record `v`,
 * leaving every other vendor byte intact (load-modify-save). */
static void channel_to_vendor(const channel_t *o, hd2_vendor_channel_t *v)
{
    if(o->mode == OPMODE_DMR) v->opt21 |=  HD2_CH_O21_DMR_MODE;
    else                      v->opt21 &= (uint8_t) ~HD2_CH_O21_DMR_MODE;

    if(o->bandwidth == BW_25) v->opt29 |=  HD2_CH_O29_BW_WIDE;
    else                      v->opt29 &= (uint8_t) ~HD2_CH_O29_BW_WIDE;

    uint8_t pw;
    if(o->power >= 5000u)      pw = HD2_PWR_HIGH;
    else if(o->power >= 2500u) pw = HD2_PWR_MID;
    else if(o->power <= 500u)  pw = HD2_PWR_XLOW;
    else                       pw = HD2_PWR_LOW;
    v->opt21 = (uint8_t)((v->opt21 & ~HD2_CH_O21_POWER_MASK)
                         | (pw << HD2_CH_O21_POWER_SHIFT));

    hd2_hz_to_bcd4(o->rx_frequency, v->rx_freq);
    hd2_hz_to_bcd4(o->tx_frequency, v->tx_freq);

    memset(v->name, 0, sizeof(v->name));
    for(unsigned i = 0; i < sizeof(v->name) && o->name[i] != '\0'; ++i)
        v->name[i] = o->name[i];

    if(o->mode == OPMODE_DMR)
    {
        uint8_t cc = o->dmr.txColorCode & 0x0Fu;
        v->cc_ts_dmr = (uint8_t)((v->cc_ts_dmr & ~HD2_CH_O2A_CC_MASK)
                                 | (cc << HD2_CH_O2A_CC_SHIFT));
        if(o->dmr.dmr_timeslot == 2) v->cc_ts_dmr |=  HD2_CH_O2A_TS2;
        else                         v->cc_ts_dmr &= (uint8_t) ~HD2_CH_O2A_TS2;
    }
    else
    {
        encode_tone(o->fm.rxToneEn, o->fm.rxTone, o->fm.rxToneType, v->rx_tone);
        encode_tone(o->fm.txToneEn, o->fm.txTone, o->fm.txToneType, v->tx_tone);
    }
}

/* ================================================================== *
 *  cps_io.h lifecycle                                                *
 * ================================================================== */

int cps_open(char *cps_name)
{
    (void) cps_name;
    return w25q_hd2_probe() ? 0 : -1;
}

void cps_close(void) { }

int cps_create(char *cps_name)
{
    /* The vendor codeplug is authored by the Ailunce CPS over USB; never lay
     * one down here (and never wipe the factory data). */
    (void) cps_name;
    errno = ENOTSUP;
    return -1;
}

/* ================================================================== *
 *  Channels (read + write, vendor slot = pos + 2)                    *
 * ================================================================== */

int cps_readChannel(channel_t *channel, uint16_t pos)
{
    if(pos >= HD2_CHAN_MAX) return -1;

    hd2_vendor_channel_t v;
    if(hd2_vendor_channel_read((uint16_t)(pos + HD2_VFO_SLOTS), &v) != 0) return -1;
    if(!hd2_vendor_channel_present(&v)) return -1;

    vendor_to_channel(&v, channel);
    return 0;
}

int cps_writeChannel(channel_t channel, uint16_t pos)
{
    if(pos >= HD2_CHAN_MAX) return -1;

    uint16_t slot = (uint16_t)(pos + HD2_VFO_SLOTS);

    /* Load-modify-save: preserve the slot's vendor-only fields. */
    hd2_vendor_channel_t v;
    if(hd2_vendor_channel_read(slot, &v) != 0) return -1;
    if(!hd2_vendor_channel_present(&v))
    {
        memset(&v, 0, sizeof(v));
        memset(v.marker, 0xFF, sizeof(v.marker));   /* 0xFFFFFFFF = populated */
        v.rx_list[0] = HD2_CH_RXLIST_END;
    }

    channel_to_vendor(&channel, &v);
    return hd2_vendor_channel_write(slot, &v);
}

/* ================================================================== *
 *  Priority contacts (read-only)                                     *
 * ================================================================== */

int cps_readContact(contact_t *contact, uint16_t pos)
{
    hd2_vendor_contact_t c;
    if(hd2_vendor_contact_read(pos, &c) != 0) return -1;
    if(!hd2_vendor_contact_present(&c)) return -1;

    memset(contact, 0, sizeof(*contact));
    contact->mode = OPMODE_DMR;

    unsigned n = 0;
    for(; n < 16u && c.name[n] != 0 && (uint8_t) c.name[n] != 0xFFu
          && n < sizeof(contact->name) - 1u; ++n)
        contact->name[n] = c.name[n];
    contact->name[n] = 0;

    contact->info.dmr.id = c.dmr_id;
    switch(c.type)
    {
        case HD2_CT_PRIVATE: contact->info.dmr.contactType = PRIVATE; break;
        case HD2_CT_ALL:     contact->info.dmr.contactType = ALL;     break;
        case HD2_CT_GROUP:
        default:             contact->info.dmr.contactType = GROUP;   break;
    }
    contact->info.dmr.rx_tone = 0;
    return 0;
}

/* ================================================================== *
 *  Zones / OpenRTX "banks" (read-only)                               *
 * ================================================================== */

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    if(pos >= HD2_ZONE_MAX || !w25q_hd2_probe()) return -1;

    uint8_t raw[HD2_ZONE_STRIDE];
    w25q_hd2_read(HD2_ZONE_BASE + (uint32_t) pos * HD2_ZONE_STRIDE, raw, sizeof(raw));

    uint8_t count = raw[0x00];
    if(count == 0xFFu || count == 0x00u || count > HD2_ZONE_CHMAX) return -1;

    bool blank = true;
    for(unsigned i = 0; i < 16u; ++i)
        if(raw[0x81 + i] != 0xFFu && raw[0x81 + i] != 0x00u) { blank = false; break; }
    if(blank) return -1;

    memset(b_header, 0, sizeof(*b_header));
    unsigned n = 0;
    for(; n < 16u && raw[0x81 + n] != 0 && raw[0x81 + n] != 0xFFu
          && n < sizeof(b_header->name) - 1u; ++n)
        b_header->name[n] = (char) raw[0x81 + n];
    b_header->name[n] = 0;
    b_header->ch_count = count;
    return 0;
}

int cps_readBankData(uint16_t bank_pos, uint16_t pos)
{
    if(bank_pos >= HD2_ZONE_MAX || pos >= HD2_ZONE_CHMAX || !w25q_hd2_probe())
        return -1;

    uint8_t raw[HD2_ZONE_STRIDE];
    w25q_hd2_read(HD2_ZONE_BASE + (uint32_t) bank_pos * HD2_ZONE_STRIDE, raw, sizeof(raw));

    uint8_t count = raw[0x00];
    if(count == 0xFFu || pos >= count || count > HD2_ZONE_CHMAX) return -1;

    uint32_t i = 0x01u + (uint32_t) pos * 2u;       /* 2-byte LE channel index */
    return (int)((uint32_t) raw[i] | ((uint32_t) raw[i + 1] << 8));
}

/* ================================================================== *
 *  Channel insert / delete (records + presence bitmap)               *
 *                                                                    *
 *  Shift the contiguous channel records in place, re-terminate the   *
 *  list with a blank slot, then rewrite the vendor channel-presence   *
 *  bitmap so the edit is also visible to the vendor CPS / firmware    *
 *  (which count channels via the bitmap, not the per-record markers   *
 *  OpenRTX reads).                                                    *
 *                                                                    *
 *  NOTE: in-place edits are not power-fail atomic (each shift is a    *
 *  sector erase+program); a tear mid-shift can leave a torn list.     *
 * ================================================================== */

/*
 * Channel-presence bitmap (vendor "vfo_config"): TWO identical 375-byte copies,
 * bit CLEAR = populated, bit 0 of a byte = lowest channel of that group of 8.
 * Indexing is real-channel (bit i == OpenRTX channel i == vendor slot i+2); the
 * two VFO-default slots are not represented -- matches the factory codeplug.
 * Both copies must stay identical (vendor anti-tamper).
 *
 * Physical flash base (firmware-confirmed + live-verified): the b1=0x0F CPS
 * regions map as physical = radio_addr + 0x790000 (a flat base, NOT the b1=0x31
 * block*0x400 rule), so vfo_config radio 0x2000 -> 0x792000.  Confirmed in the
 * vendor fw (the bm2 constant 0x792177 appears in the anti-tamper, per-channel
 * and boot-count paths) and by a live CPS read of radio 0x2000 returning the
 * 38-channel bitmap (00 00 00 00 c0 ff...).  bm2 follows bm1 at +0x177 (=375).
 * The settings block shares this 4 kB sector (radio 0x2900 -> 0x792900); the
 * RMW below reads-modifies-writes the whole sector, so it is preserved.
 */
#define HD2_BITMAP_BASE     0x00792000u
#define HD2_BITMAP_BYTES    375u            /* one copy = 3000 channel bits */
#define HD2_BITMAP_BM2_OFF  HD2_BITMAP_BYTES /* parity copy follows bm1 */

/* Number of populated channels (contiguous from vendor slot 2). */
static uint16_t channel_count(void)
{
    uint16_t n = 0;
    hd2_vendor_channel_t v;
    while(n < HD2_CHAN_MAX)
    {
        if(hd2_vendor_channel_read((uint16_t)(n + HD2_VFO_SLOTS), &v) != 0) break;
        if(!hd2_vendor_channel_present(&v)) break;
        n++;
    }
    return n;
}

/* Rewrite both bitmap copies for a contiguous list of `count` channels
 * (bits 0..count-1 clear = populated, rest set).  One 4 kB-sector RMW that
 * preserves the rest of the region. */
static int bitmap_write(uint16_t count)
{
    if(!w25q_hd2_probe()) return -1;

    uint8_t bm[HD2_BITMAP_BYTES];
    for(unsigned i = 0; i < HD2_BITMAP_BYTES; ++i)
    {
        unsigned base = i * 8u;
        if(base + 8u <= count)  bm[i] = 0x00u;                       /* all populated */
        else if(base >= count)  bm[i] = 0xFFu;                       /* all empty */
        else                    bm[i] = (uint8_t)(0xFFu << (count - base)); /* split */
    }

    static uint8_t sec[W25Q_HD2_SECTOR_SIZE];
    uint32_t sbase = HD2_BITMAP_BASE & ~(W25Q_HD2_SECTOR_SIZE - 1u);
    uint32_t off   = HD2_BITMAP_BASE - sbase;
    w25q_hd2_read(sbase, sec, sizeof(sec));
    memcpy(sec + off,                       bm, HD2_BITMAP_BYTES);   /* bm1 */
    memcpy(sec + off + HD2_BITMAP_BM2_OFF,  bm, HD2_BITMAP_BYTES);   /* bm2 */
    if(w25q_hd2_eraseSector(sbase) < 0)               return -1;
    if(w25q_hd2_program(sbase, sec, sizeof(sec)) < 0) return -1;
    return 0;
}

int cps_insertChannel(channel_t channel, uint16_t pos)
{
    uint16_t n = channel_count();
    if(n >= HD2_CHAN_MAX) return -1;
    if(pos > n) pos = n;                         /* clamp to append */

    /* Shift records [pos, n) up by one slot, from the top down. */
    hd2_vendor_channel_t v;
    for(int i = (int) n - 1; i >= (int) pos; --i)
    {
        if(hd2_vendor_channel_read((uint16_t)(i + HD2_VFO_SLOTS), &v) != 0)
            return -1;
        if(hd2_vendor_channel_write((uint16_t)(i + 1 + HD2_VFO_SLOTS), &v) != 0)
            return -1;
    }

    /* Write the new channel as a clean populated record at `pos`. */
    hd2_vendor_channel_t nv;
    memset(&nv, 0, sizeof(nv));
    memset(nv.marker, 0xFF, sizeof(nv.marker));   /* 0xFFFFFFFF = populated */
    nv.rx_list[0] = HD2_CH_RXLIST_END;
    channel_to_vendor(&channel, &nv);
    if(hd2_vendor_channel_write((uint16_t)(pos + HD2_VFO_SLOTS), &nv) != 0)
        return -1;

    return bitmap_write((uint16_t)(n + 1));       /* presence bitmap upkeep */
}

int cps_deleteChannel(channel_t channel, uint16_t pos)
{
    (void) channel;
    uint16_t n = channel_count();
    if(pos >= n) return -1;

    /* Shift records (pos, n) down by one slot. */
    hd2_vendor_channel_t v;
    for(uint16_t i = pos; (uint16_t)(i + 1) < n; ++i)
    {
        if(hd2_vendor_channel_read((uint16_t)(i + 1 + HD2_VFO_SLOTS), &v) != 0)
            return -1;
        if(hd2_vendor_channel_write((uint16_t)(i + HD2_VFO_SLOTS), &v) != 0)
            return -1;
    }

    /* Blank the now-vacated last slot to terminate the contiguous list. */
    hd2_vendor_channel_t blank;
    memset(&blank, 0, sizeof(blank));
    if(hd2_vendor_channel_write((uint16_t)((n - 1) + HD2_VFO_SLOTS), &blank) != 0)
        return -1;

    return bitmap_write((uint16_t)(n - 1));       /* presence bitmap upkeep */
}

int cps_writeContact(contact_t contact, uint16_t pos)
{ (void) contact; (void) pos; return -1; }

int cps_insertContact(contact_t contact, uint16_t pos)
{ (void) contact; (void) pos; return -1; }

int cps_deleteContact(uint16_t pos)
{ (void) pos; return -1; }

int cps_writeBankHeader(bankHdr_t b_header, uint16_t pos)
{ (void) b_header; (void) pos; return -1; }

int cps_insertBankHeader(bankHdr_t b_header, uint16_t pos)
{ (void) b_header; (void) pos; return -1; }

int cps_deleteBankHeader(uint16_t pos)
{ (void) pos; return -1; }

int cps_writeBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{ (void) ch; (void) bank_pos; (void) pos; return -1; }

int cps_insertBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{ (void) ch; (void) bank_pos; (void) pos; return -1; }

int cps_deleteBankData(uint16_t bank_pos, uint16_t pos)
{ (void) bank_pos; (void) pos; return -1; }
