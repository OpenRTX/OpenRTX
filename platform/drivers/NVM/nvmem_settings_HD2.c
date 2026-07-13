/*
 * SPDX-FileCopyrightText: Copyright 2026 HD2 Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Persistent OpenRTX settings + last-VFO storage for the Ailunce HD2
 * (Dahua HR_C7000, CK803S), on the external Winbond W25Q512 SPI-NOR
 * flash (drivers/SPI/spi_hd2.c, HW SPI0 by default).
 *
 * Implements the settings/VFO half of interfaces/nvmem.h for the
 * threaded `openrtx_hd2` app; the codeplug half (cps_io) lives in
 * nvmem_HD2.c.
 *
 * ==================================================================
 *  Flash sector choice -- HD2_SETT_SECTOR = 0x00FFF000 (4 kB)
 * ------------------------------------------------------------------
 *  The chip is a 64 MB W25Q512 (JEDEC ef 40 20, live-verified) shared
 *  with the VENDOR firmware's codeplug + data.  Vendor flash map,
 *  reverse-engineered from the v2.1.3 decomp (src/firmware/) and
 *  vendor/pylunce/codeplug.py (wire block-address x block-size):
 *
 *    0x0000000-0x0000400  codeplug header / addr book
 *    0x0100000-0x0100380  vendor VFO config
 *    0x0148000-0x014B680  vendor settings
 *    0x06DDD00-...        contact presence bitmap (low banks)
 *    0x06E1000-0x0774000  contacts + channels ("channels" region)
 *    0x077E000-0x0781000  table_1dfx / quick-text SMS
 *    0x0793D00-0x0794400  scan lists + group lists
 *    0x07AF000-0x0894100  CPS wire-protocol write window
 *    0x0814000            contact bank 0
 *    0x0876000-0x08C0000  table_21dx (SMS in/out, call logs, bank 4)
 *    0x0F8DD00-<0x0F9D200 contact presence bitmap, high bank.  HARD
 *                         upper bound: index < 0x7A120 (500000) is
 *                         enforced in record_presence_bit_set
 *                         (v213 @ 0x03055f90), so the bitmap cannot
 *                         reach past 0x0F8DD00 + 500000/8 = 0x0F9D124.
 *    0x1000000-0x1000400  table_4000 / contact bank 2 (big DMR DB)
 *    0x10A0000-0x10A0C00  table_428x
 *    0x3A33000, 0x3A46000 high contact bank + bitmap (extent unknown)
 *
 *  0x0FFF000 is the topmost 4 kB sector of the 3-byte-addressable
 *  first 16 MB.  It sits ABOVE the bounded presence bitmap (ends
 *  <= 0x0F9D200) and immediately BELOW the next vendor base at
 *  0x1000000; sector erase is exact (4 kB), so adjacency is safe.  A
 *  literal scan of the entire 5896-function decomp for constants in
 *  0x8C0000..0xFFFFFF turned up only the 0xF8DD00 bitmap, so this
 *  sector is free as far as the vendor RE goes.  The true top of the
 *  chip (0x3FFF000) was rejected: the 0x3A46000 contact bank's extent
 *  is unknown and may grow upward.
 *
 * ==================================================================
 *  Robustness / "never clobber the vendor data" rules
 * ------------------------------------------------------------------
 *  1. JEDEC gate: every operation (read or write) first requires a
 *     JEDEC-ID read returning ef 40 20.  This proves both SPI data
 *     directions work at firmware speed before any address is ever
 *     clocked out -- a half-broken bus could otherwise misclock an
 *     erase address into vendor territory.  If the gate fails we
 *     behave exactly like the old stubs (boot with defaults, skip
 *     the save).
 *  2. Mode-independent opcodes: the W25Q512 powers up in 3-byte
 *     address mode, but the vendor firmware switches it to 4-byte
 *     (opcode 0xB7) at boot, and a warm reflash can leave it there.
 *     Legacy 0x03/0x02/0x20 commands change meaning with the mode --
 *     in the wrong mode a page program would land at a pseudo-random
 *     address.  We exclusively use the dedicated 4-byte-address
 *     instructions (0x13 read / 0x12 program / 0x21 erase), which
 *     take 4 address bytes REGARDLESS of the chip's current mode.
 *     (They exist on every >=256 Mbit Winbond part; the JEDEC gate
 *     guarantees we are on one.)
 *  3. Read-back verify after program; on mismatch the cached state is
 *     invalidated and -1 returned, so the next boot falls back to
 *     defaults instead of trusting a torn write (the CRC would catch
 *     it anyway).
 *
 * ==================================================================
 *  GPIOA concurrency
 * ------------------------------------------------------------------
 *  The flash bus (PTA18/20/21/22) shares the GPIOA data register with
 *  the AT1846S / RDA5802E bit-bang I2C (PTA7/PTA8, i2c_csky.c), which
 *  does plain non-atomic read-modify-writes from the rtx thread.  A
 *  flash transaction interleaved with an I2C RMW would lose updates on
 *  GPIOA_DR.  Therefore every SPI *transaction* here (CS low ... CS
 *  high) runs with IRQs disabled (hd2_irq_save/restore, same primitive
 *  as the gpio_atomic_* helpers): on this single-core part that makes
 *  the whole transaction atomic against both threads and IRQs.  Locked
 *  windows are bounded (largest: one 256-byte page program, a few ms at
 *  bit-bang speed); the multi-ms erase WAIT polls the status register
 *  in short locked transactions with IRQs enabled in between.  Boot
 *  reads run before create_threads(), so they are uncontended anyway;
 *  the save runs in state_terminate() on the knob-off shutdown path.
 *
 * ==================================================================
 *  Storage format (one self-contained block at sector start)
 * ------------------------------------------------------------------
 *  [ u32 magic "H2SV" | u8 version | u8 pad | u16 size |
 *    settings_t | channel_t vfo | u16 crc_ccitt(settings..vfo) ]
 *
 *  An erased (all-0xFF) or never-written/garbage sector fails the
 *  magic/size/CRC check and reads return -1, so state_init() falls
 *  back to compiled-in defaults exactly as with the old stubs.  The
 *  `size` field doubles as a layout fingerprint: any change to
 *  settings_t / channel_t that alters their packed size invalidates
 *  old blocks automatically (bump HD2_SETT_VERSION for same-size
 *  layout changes).
 *
 *  Save policy: state_terminate() (clean shutdown) is the only HD2
 *  call site of nvm_writeSettingsAndVfo(); writes are skipped when the
 *  content CRC is unchanged, so the sector wears only when the user
 *  actually changed something (NOR endurance 100k cycles).
 */

#include "interfaces/nvmem.h"
#include "interfaces/delays.h"
#include "drivers/SPI/spi_hd2.h"
#include "drivers/NVM/flash_w25q_HD2.h"
#include "core/crc.h"
#include "hd2_regs.h"
#include <stdbool.h>
#include <string.h>

/* ------------------------------------------------------------------ *
 *  Flash geometry / layout                                            *
 * ------------------------------------------------------------------ */
#define HD2_SETT_SECTOR     0x00FFF000u  /* see header comment       */
#define HD2_SECT_SIZE       4096u
#define HD2_PAGE_SIZE       256u

#define HD2_SETT_MAGIC      0x56533248u  /* "H2SV" little-endian     */
#define HD2_SETT_VERSION    1u

typedef struct
{
    uint32_t   magic;
    uint8_t    version;
    uint8_t    _pad;
    uint16_t   size;        /* sizeof(hd2SettBlock_t): layout fingerprint */
    settings_t settings;
    channel_t  vfo;
    uint16_t   crc;         /* crc_ccitt over settings..vfo */
}
__attribute__((packed)) hd2SettBlock_t;

/* Cached copy of the flash block + tri-state validity. */
enum cacheState { CACHE_UNKNOWN = 0, CACHE_VALID, CACHE_INVALID };

static hd2SettBlock_t blockCache;
static enum cacheState cacheState = CACHE_UNKNOWN;


/* ================================================================== *
 *  Low-level W25Q access: shared flash_w25q_HD2 driver               *
 * ================================================================== */


/* ================================================================== *
 *  Block cache                                                        *
 * ================================================================== */

static uint16_t blockCrc(const hd2SettBlock_t *block)
{
    return crc_ccitt(&block->settings,
                     sizeof(settings_t) + sizeof(channel_t));
}

/*
 * Load + validate the settings block from flash into the cache.  Runs
 * the full probe-read-validate chain only once; subsequent calls hit
 * the cache (boot calls readSettings and readVfoChannelData back to
 * back, this keeps the second one free).
 */
static void loadCache(void)
{
    if(cacheState != CACHE_UNKNOWN)
        return;

    cacheState = CACHE_INVALID;

    if(w25q_hd2_probe() == false)
        return;

    w25q_hd2_read(HD2_SETT_SECTOR, &blockCache, sizeof(blockCache));

    if(blockCache.magic   != HD2_SETT_MAGIC)            return;
    if(blockCache.version != HD2_SETT_VERSION)          return;
    if(blockCache.size    != sizeof(hd2SettBlock_t))    return;
    if(blockCache.crc     != blockCrc(&blockCache))     return;

    cacheState = CACHE_VALID;
}

/* Erase + program + verify one complete block; updates the cache. */
static int storeBlock(const hd2SettBlock_t *block)
{
    if(w25q_hd2_probe() == false)
        return -1;

    if(w25q_hd2_eraseSector(HD2_SETT_SECTOR) < 0)
        return -1;

    if(w25q_hd2_program(HD2_SETT_SECTOR, block, sizeof(*block)) < 0)
        return -1;

    /* Read-back verify: never trust a torn/failed program. */
    hd2SettBlock_t check;
    w25q_hd2_read(HD2_SETT_SECTOR, &check, sizeof(check));
    if(memcmp(&check, block, sizeof(check)) != 0)
    {
        cacheState = CACHE_INVALID;
        return -1;
    }

    blockCache = *block;
    cacheState = CACHE_VALID;
    return 0;
}


/* ================================================================== *
 *  interfaces/nvmem.h -- settings + VFO entry points                  *
 * ================================================================== */

int nvm_readSettings(settings_t *settings)
{
    loadCache();
    if(cacheState != CACHE_VALID)
        return -1;

    *settings = blockCache.settings;
    return 0;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    loadCache();
    if(cacheState != CACHE_VALID)
        return -1;

    /* mode == 0 marks a block saved without VFO data (settings-only
     * write path); let the core fall back to its default channel. */
    if(blockCache.vfo.mode == 0)
        return -1;

    *channel = blockCache.vfo;
    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    hd2SettBlock_t block;

    memset(&block, 0x00, sizeof(block));
    block.magic    = HD2_SETT_MAGIC;
    block.version  = HD2_SETT_VERSION;
    block.size     = sizeof(hd2SettBlock_t);
    block.settings = *settings;
    block.vfo      = *vfo;
    block.crc      = blockCrc(&block);

    /* Nothing changed since the last load/save: skip the flash wear
     * (CRC first as a cheap reject, memcmp to rule out collisions). */
    if((cacheState == CACHE_VALID) && (block.crc == blockCache.crc) &&
       (memcmp(&block.settings, &blockCache.settings,
               sizeof(settings_t) + sizeof(channel_t)) == 0))
        return 0;

    return storeBlock(&block);
}

int nvm_writeSettings(const settings_t *settings)
{
    /* Not called on the HD2 (only Module17 uses it), implemented for
     * contract completeness: keep the stored VFO if we have one. */
    hd2SettBlock_t block;

    memset(&block, 0x00, sizeof(block));
    block.magic    = HD2_SETT_MAGIC;
    block.version  = HD2_SETT_VERSION;
    block.size     = sizeof(hd2SettBlock_t);
    block.settings = *settings;
    if(cacheState == CACHE_VALID)
        block.vfo = blockCache.vfo;
    block.crc      = blockCrc(&block);

    if((cacheState == CACHE_VALID) && (block.crc == blockCache.crc) &&
       (memcmp(&block.settings, &blockCache.settings,
               sizeof(settings_t) + sizeof(channel_t)) == 0))
        return 0;

    return storeBlock(&block);
}
