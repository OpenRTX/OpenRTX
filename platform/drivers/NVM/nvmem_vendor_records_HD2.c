/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Read-only accessor for the vendor factory channel/contact records
 * (hd2_cps_records.h) on the W25Q512 over HW SPI0.
 *
 * Records live in the vendor "channels" family-0x31 region.  Physical W25Q
 * byte offset = CPS block_index * 0x400 (validated in nvmem_HD2.c):
 *   priority contacts  block 0x1B84 -> 0x006E1000, 72 B stride, 14 / block
 *   channel slots      block 0x1BCC -> 0x006F3000, 176 B stride, contiguous
 * Contacts pack 14-per-0x400-block from the block base (record N in block
 * N/14, slot N%14).  Channels do NOT: they form one contiguous 176-byte
 * array starting at the region's first-block +0x80, straddling block
 * boundaries (record N at region+0x80+N*176).  See channelAddr().
 *
 * Channels read AND write; contacts stay read-only.  Both back the vendor-
 * backed OpenRTX codeplug (cps_io_HD2.c): the reader surfaces the factory
 * channels to the UI, the channel writer rewrites a slot in place in the live
 * factory region when the UI edits a channel.  Flash primitives mirror the
 * sibling NVM drivers (TODO: factor a shared flash_w25q_HD2 driver).
 */

#include "hd2_cps_records.h"
#include "flash_w25q_HD2.h"
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* Vendor region geometry (see header + nvmem_HD2.c). */
#define VND_BLOCK_SIZE     0x400u
#define VND_CONTACT_BASE   0x006E1000u
#define VND_CONTACT_PERBLK 14u
#define VND_CHAN_BASE      0x006F3000u
#define VND_CHAN_SLOT0     0x80u        /* first channel slot's in-block offset */

/* ---- block/slot address arithmetic -------------------------------- */

static uint32_t contactAddr(uint16_t index)
{
    uint32_t block = index / VND_CONTACT_PERBLK;
    uint32_t slot  = index % VND_CONTACT_PERBLK;
    return VND_CONTACT_BASE + block * VND_BLOCK_SIZE + slot * sizeof(hd2_vendor_contact_t);
}

static uint32_t channelAddr(uint16_t index)
{
    /* Unlike contacts (14 packed per 0x400 block), channel records are stored
     * CONTIGUOUSLY: a 176-byte stride starting at the region's first-block
     * +0x80 offset, running straight across the 0x400 block boundaries -- a
     * single record can straddle two blocks.  Verified against the factory
     * codeplug and the vendor CPS parser: channel 5 lands at region+0x80+5*0xB0
     * (block 0's tail, spilling into block 1), NOT block1+0x80.  The earlier
     * per-block model read a gap for channel 5 and made the importer stop
     * after the first block. */
    return VND_CHAN_BASE + VND_CHAN_SLOT0 +
           (uint32_t) index * sizeof(hd2_vendor_channel_t);
}

/* ---- public API ---------------------------------------------------- */

int hd2_vendor_contact_present(const hd2_vendor_contact_t *c)
{
    /* Erased flash reads 0xFF; a real contact has a sane type and a name
     * that isn't all-0xFF/all-0x00. */
    if(c->type != HD2_CT_GROUP && c->type != HD2_CT_PRIVATE && c->type != HD2_CT_ALL)
        return 0;
    if((uint8_t)c->name[0] == 0xFFu || c->name[0] == 0x00) return 0;
    return 1;
}

int hd2_vendor_channel_present(const hd2_vendor_channel_t *c)
{
    /* Key off the RX frequency, NOT the name -- valid channels can be unnamed
     * (e.g. cp_ai5qz channel 0 = 145.325 MHz with a blank name).  An erased
     * slot has rx_freq all-0xFF; an unused/zeroed slot all-0x00. */
    int all_ff = 1, all_00 = 1;
    for(int i = 0; i < 4; ++i)
    {
        if(c->rx_freq[i] != 0xFFu) all_ff = 0;
        if(c->rx_freq[i] != 0x00u) all_00 = 0;
    }
    return !(all_ff || all_00);
}

int hd2_vendor_contact_read(uint16_t index, hd2_vendor_contact_t *out)
{
    if(!w25q_hd2_probe()) return -1;
    w25q_hd2_read(contactAddr(index), out, sizeof(*out));
    return 0;
}

int hd2_vendor_channel_read(uint16_t index, hd2_vendor_channel_t *out)
{
    if(!w25q_hd2_probe()) return -1;
    w25q_hd2_read(channelAddr(index), out, sizeof(*out));
    return 0;
}

int hd2_vendor_channel_write(uint16_t index, const hd2_vendor_channel_t *in)
{
    if(!w25q_hd2_probe()) return -1;

    const uint32_t addr = channelAddr(index);
    const uint32_t end  = addr + sizeof(*in);
    const uint8_t *src  = (const uint8_t *) in;

    /* A 176-byte record straddles 1-2 of the 0x400-byte... no -- the W25Q erase
     * granularity is a 4 kB SECTOR, so rewrite each sector the record touches:
     * read it, patch the record's bytes in place, erase, program back.  Single-
     * threaded under the export path; a 4 kB scratch keeps it off the stack. */
    static uint8_t sec[W25Q_HD2_SECTOR_SIZE];
    for(uint32_t base = addr & ~(W25Q_HD2_SECTOR_SIZE - 1u);
        base < end; base += W25Q_HD2_SECTOR_SIZE)
    {
        w25q_hd2_read(base, sec, sizeof(sec));

        uint32_t lo = (addr > base) ? addr : base;
        uint32_t hi = (end < base + W25Q_HD2_SECTOR_SIZE) ? end
                                                          : base + W25Q_HD2_SECTOR_SIZE;
        memcpy(sec + (lo - base), src + (lo - addr), hi - lo);

        if(w25q_hd2_eraseSector(base) < 0)        return -1;
        if(w25q_hd2_program(base, sec, sizeof(sec)) < 0) return -1;
    }
    return 0;
}
