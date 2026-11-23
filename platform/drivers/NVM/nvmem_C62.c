/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <zephyr/storage/flash_map.h>
#include <zephyr/sys/printk.h>
#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "interfaces/nvmem.h"
#include "calibration/calib_C62.h"
#include "core/crc.h"
#include "core/nvmem_access.h"
#include "core/utils.h"
#include "flash_zephyr.h"

ZEPHYR_FLASH_DEVICE_DEFINE(eflash, flash_controller);

/* Partitions of the flash memory, as in platform/targets/c62/c62.dts */
enum {
    NVM_PART_STORAGE = 1, ///< OpenRTX settings and VFO.
    NVM_PART_CALIB = 2,   ///< Stock firmware's settings journal, never written.
};

static const struct nvmPartition memPartitions[] = {
    {
        .offset = FIXED_PARTITION_OFFSET(storage_partition),
        .size = FIXED_PARTITION_SIZE(storage_partition),
    },
    {
        .offset = FIXED_PARTITION_OFFSET(factory_cal),
        .size = FIXED_PARTITION_SIZE(factory_cal),
    },
};

static const struct nvmDescriptor nvMemory = {
    .name = "Flash",
    .dev = &eflash,
    .baseAddr = 0x00000000,
    .size = DT_REG_SIZE(DT_ALIAS(flash)),
    .nbPart = ARRAY_SIZE(memPartitions),
    .partitions = memPartitions,
};

const struct nvmTable nvmTab = {
    .areas = &nvMemory,
    .nbAreas = 1,
};

/*
 * Settings and VFO data each have two slots, in separate erase sectors of the
 * storage partition. A record is written to the slot not holding the current
 * copy, so a power cut during the erase or the write leaves the previous copy
 * in place: on the C62 the power knob is the normal way to switch off. Each
 * record carries a magic value, a sequence number and a check value to detect
 * erased or corrupted storage: the valid slot with the newest sequence number
 * is current.
 */
#define SECTOR_SIZE 0x1000
#define NVM_RECORDS 2 /* Settings and VFO, whose slots alternate */
#define NVM_SLOTS 2
#define SETTINGS_ADDR 0x0000
#define VFO_ADDR (SETTINGS_ADDR + SECTOR_SIZE)
#define NVM_MAGIC 0x4F525458U /* "ORTX" */
#define NVM_RECORD_MAX 256U
#define NVM_WRITE_ALIGN 4U

/**
 * Header of a record. The check value is the CRC16 of the payload XORed with
 * the sequence number, so it also covers the latter, and a record with
 * sequence number zero is the same as those written before the slots.
 */
struct nvmRecord {
    uint32_t magic;
    uint16_t check;
    uint16_t seq;
};

/**
 * Storage state of a record: the slot holding the current copy and its
 * sequence number.
 */
struct nvmSlots {
    uint32_t addr; ///< Offset of the first slot in the storage partition
    uint16_t seq;  ///< Sequence number of the current copy
    uint8_t slot;  ///< Slot of the current copy
    bool valid;    ///< A current copy exists
};

static struct nvmSlots settingsSlots = { .addr = SETTINGS_ADDR };
static struct nvmSlots vfoSlots = { .addr = VFO_ADDR };
static uint16_t settingsCrc;
static uint16_t vfoCrc;

/**
 * Read from the stock firmware's settings journal partition.
 *
 * @param ctx: unused.
 * @param off: offset in the partition.
 * @param buf: destination buffer.
 * @param len: number of bytes to read.
 * @return zero on success, a negative error code otherwise.
 */
static int journalRead(void *ctx, uint32_t off, void *buf, size_t len)
{
    (void)ctx;

    return nvm_read(0, NVM_PART_CALIB, off, buf, len);
}

/**
 * Get the offset of a record slot. The slots of the two records alternate, a
 * sector each: settings, VFO, settings, VFO.
 *
 * @param slots: record storage state.
 * @param slot: slot number.
 * @return slot offset inside the storage partition.
 */
static uint32_t slotAddr(const struct nvmSlots *slots, uint8_t slot)
{
    return slots->addr + (slot * NVM_RECORDS * SECTOR_SIZE);
}

/**
 * Read one slot of a record, validating magic and CRC.
 *
 * @param off: slot offset inside the storage partition.
 * @param buf: destination buffer of NVM_RECORD_MAX bytes: the payload is read
 * padded to the flash write block size.
 * @param len: payload size in bytes.
 * @param seq: sequence number of the record.
 * @return zero on success, a negative error code otherwise.
 */
static int readSlot(uint32_t off, uint8_t *buf, size_t len, uint16_t *seq)
{
    struct nvmRecord rec;
    size_t padded = (len + NVM_WRITE_ALIGN - 1U) & ~(NVM_WRITE_ALIGN - 1U);
    int ret;

    if (padded > NVM_RECORD_MAX)
        return -EINVAL;

    ret = nvm_read(0, NVM_PART_STORAGE, off, &rec, sizeof(rec));
    if (ret < 0)
        return ret;

    if (rec.magic != NVM_MAGIC)
        return -ENOENT;

    ret = nvm_read(0, NVM_PART_STORAGE, off + sizeof(rec), buf, padded);
    if (ret < 0)
        return ret;

    if (rec.check != (crc_ccitt(buf, len) ^ rec.seq))
        return -EIO;

    *seq = rec.seq;

    return 0;
}

/**
 * Read the current copy of a record: the valid slot with the newest sequence
 * number.
 *
 * @param slots: record storage state, updated with the slot found.
 * @param data: destination buffer for the record payload, left untouched if
 * there is no valid copy.
 * @param len: payload size in bytes.
 * @return zero on success, a negative error code otherwise.
 */
static int readRecord(struct nvmSlots *slots, void *data, size_t len)
{
    slots->valid = false;

    for (uint8_t slot = 0; slot < NVM_SLOTS; slot++) {
        uint8_t buf[NVM_RECORD_MAX];
        uint16_t seq;

        if (readSlot(slotAddr(slots, slot), buf, len, &seq) < 0)
            continue;

        // Sequence numbers wrap around: the newer one is ahead by less than
        // half the range
        if (slots->valid && ((int16_t)(seq - slots->seq) < 0))
            continue;

        memcpy(data, buf, len);
        slots->seq = seq;
        slots->slot = slot;
        slots->valid = true;
    }

    return slots->valid ? 0 : -ENOENT;
}

/**
 * Write a record to the slot not holding its current copy, which is erased
 * first. Payloads are padded to the flash write block size.
 *
 * @param slots: record storage state, updated once the write succeeds.
 * @param data: payload to be written.
 * @param len: payload size in bytes.
 * @return zero on success, a negative error code otherwise.
 */
static int writeRecord(struct nvmSlots *slots, const void *data, size_t len)
{
    const uint8_t slot = slots->valid ? ((slots->slot + 1) % NVM_SLOTS) : 0;
    const uint16_t seq = slots->valid ? (slots->seq + 1) : 0;
    const uint32_t off = slotAddr(slots, slot);
    struct nvmRecord rec = {
        .magic = NVM_MAGIC,
        .check = crc_ccitt(data, len) ^ seq,
        .seq = seq,
    };
    uint8_t buf[NVM_RECORD_MAX];
    size_t padded = (len + NVM_WRITE_ALIGN - 1U) & ~(NVM_WRITE_ALIGN - 1U);
    int ret;

    if (padded > sizeof(buf))
        return -EINVAL;

    memcpy(buf, data, len);
    memset(buf + len, 0xFF, padded - len);

    ret = nvm_erase(0, NVM_PART_STORAGE, off, SECTOR_SIZE);
    if (ret < 0)
        return ret;

    // The header goes last: without its magic value the slot is not valid
    ret = nvm_write(0, NVM_PART_STORAGE, off + sizeof(rec), buf, padded);
    if (ret < 0)
        return ret;

    ret = nvm_write(0, NVM_PART_STORAGE, off, &rec, sizeof(rec));
    if (ret < 0)
        return ret;

    slots->seq = seq;
    slots->slot = slot;
    slots->valid = true;

    return 0;
}

void nvm_init()
{
    zephirFlash_init(&eflash);
}

void nvm_terminate()
{
}

void nvm_readCalibData(void *buf)
{
    struct C62Calib *cal = (struct C62Calib *)buf;
    int ret = calibC62_read(journalRead, NULL,
                            FIXED_PARTITION_SIZE(factory_cal), cal);

    if (ret < 0) {
        calibC62_defaults(cal);
        printk("TX calibration: stock defaults, no valid factory data\n");
    } else {
        printk("TX calibration: factory data, %u VHF and %u UHF points\n",
               cal->vhf.count, cal->uhf.count);
    }
}

void nvm_readHwInfo(hwInfo_t *info)
{
    (void)info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    int ret = readRecord(&vfoSlots, channel, sizeof(channel_t));
    if (ret < 0)
        return ret;

    vfoCrc = crc_ccitt(channel, sizeof(channel_t));

    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    int ret = readRecord(&settingsSlots, settings, sizeof(settings_t));
    if (ret < 0)
        return ret;

    settingsCrc = crc_ccitt(settings, sizeof(settings_t));

    return 0;
}

int nvm_writeSettings(const settings_t *settings)
{
    uint16_t crc = crc_ccitt(settings, sizeof(settings_t));
    if (crc == settingsCrc)
        return 0;

    int ret = writeRecord(&settingsSlots, settings, sizeof(settings_t));
    if (ret < 0)
        return ret;

    settingsCrc = crc;

    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    // The VFO is written even if the settings could not be
    int settingsRet = nvm_writeSettings(settings);

    uint16_t crc = crc_ccitt(vfo, sizeof(channel_t));
    if (crc == vfoCrc)
        return settingsRet;

    int ret = writeRecord(&vfoSlots, vfo, sizeof(channel_t));
    if (ret < 0)
        return ret;

    vfoCrc = crc;

    return settingsRet;
}
