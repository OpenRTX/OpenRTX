/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "hwconfig.h"
#include <stdbool.h>
#include "core/crc.h"
#include <errno.h>
#include <string.h>
#include "core/nvmem_access.h"
#include "interfaces/nvmem.h"
#include "core/settings.h"
#include <stdio.h>

const uint32_t SETTINGS_MAGIC = 0x584E504F; // "OPNX"


/**
 * Returns a valid settings_store_t filled with default settings
 *
 * @param store a pointer to an allocated settings_store_t
 * @return 0 in case of success, negative error code otherwise
 */
int default_settings_store(settings_store_t *store)
{
    if (!store)
        return -EINVAL;

    store->MAGIC = SETTINGS_MAGIC;
    store->counter = 0;
    store->length = sizeof(settings_store_t);
    store->settings = default_settings;
    store->crc = crc_ccitt((void *)store, sizeof(settings_store_t) - 2);
    return 0;
}

/**
 * Update the device settings in a settings store, updating counter, length and
 * CRC accordingly.
 *
 * @param settings pointer to the up-to-date settings
 * @param store pointer to the store to update
 * @return 0 if successful, negative error code otherwise
 */
int update_settings_store(const settings_t *settings, settings_store_t *store)
{
    if (!settings || !store)
        return -EINVAL;

    store->counter++;
    store->length = sizeof(settings_store_t);
    store->settings = *settings;
    store->crc = crc_ccitt(store, sizeof(settings_store_t) - 2);
    return 0;
}

/**
 * Checks the integrity of a settings store
 *
 * @param store pointer to the settings_store to check
 * @return -1 if the store is valid but stale, 0 if the store is corrupted, 1 if the store is valid and current
 */
int check_store_integrity(const settings_store_t *store)
{
    if (store->MAGIC != SETTINGS_MAGIC)
        return 0;

    if (store->length > sizeof(settings_store_t))
        return 0;

    bool stale = (store->length != sizeof(settings_store_t));

    if (store->crc == crc_ccitt(store, store->length - 2))
        return stale ? -1 : 1;
    else
        return 0;
}

/**
 * Parse an NVM partition and look for a settings structure. This function
 * makes no guarantee that the store structure found is sound.
 *
 * @param dev NVM device to read from
 * @param part NVM partition to read from
 * @param offset pointer that will contain the partition offset to the beginning
 *               of the settings structure found. offset is invalid if the
 *               function does not return 0.
 * @param limit contains the maximum offset to explore within the partition
 * @return 0 if successful, negative error code otherwise
 */
int find_last_store(const int dev, const int part_nb, size_t *offset,
                    size_t limit)
{
    uint8_t buffer[6];
    uint32_t *magic = (uint32_t *)buffer;
    *offset = 0;
    size_t prev_offset = 0;

    // Go through the partition to find settings struct
    while (*offset < (limit - 6)) {
        int ret = nvm_read(dev, part_nb, *offset, buffer, 6);
        if (ret < 0)
            return ret;

        if (*magic != SETTINGS_MAGIC)
            break; // Check struct starts with magic word

        uint16_t len = *(uint16_t *)(buffer + 4);
        prev_offset = *offset;
        *offset += len;
    }

    /*
     * If offset == prev_offset == 0 and magic == 0xFFFFFFFF, the partition is empty
     * If offset > prev_offset and magic == 0xFFFFFFFF there is free space
     * If magic != 0xFFFFFFFF then the partition contains invalid data and needs to be cleaned
     */
    if ((*magic != 0xFFFFFFFF) && (*magic != SETTINGS_MAGIC))
        return -EILSEQ; // Partition contains invalid data
    else if (*offset == prev_offset)
        return -ENOENT; // Empty partition

    *offset = prev_offset;

    return 0;
}

/**
 * Read a store at a given offset from given NVM part. This function takes care of
 * possible shorter length from settings stored from a previous version. This
 * function does not check the integrity of the read store.
 *
 * @param dev NVM device to read from
 * @param part NVM partition to read from
 * @param offset offset to the beginning of the store to read
 * @param store pointer to the store to read to
 * @return 0 if successful, negative error code otherwise
 */
int read_store(const int dev, const int part, size_t offset,
               settings_store_t *store)
{
    // Read store header (magic, length, counter)
    int ret = nvm_read(dev, part, offset, store, 8);
    if (ret < 0)
        return ret;

    if (store->length == sizeof(settings_store_t)) {
        // read settings and CRC
        ret = nvm_read(dev, part, offset + 8, &(store->settings),
                       sizeof(settings_store_t) - 8);
        if (ret < 0)
            return ret;
    } else if (store->length < sizeof(settings_store_t)) {
        // Pre-init with default settings
        store->settings = default_settings;

        // Read settings
        ret = nvm_read(dev, part, offset + 8, &(store->settings),
                       store->length - 10);
        if (ret < 0)
            return ret;

        // Read CRC
        ret = nvm_read(dev, part, offset + store->length - 2, &(store->crc), 2);
        if (ret < 0)
            return ret;
    } else
        return -E2BIG; // Settings in NVM too large

    return 0;
}

/**
 * Gets the latest valid store from a partition along with the offset where to find it
 *
 * @param dev NVM device to read from
 * @param part NVM partition to read from
 * @param store pointer to the store to read to
 * @param offset pointer to the offset to the beginning of free space in the partition
 * @return 0 if the partition is corrupt, 1 if the partition is empty,
 *         2 if the store is valid but stale, 3 if the store is valid and current,
 *         negative error code otherwise
 */
int get_latest_valid_store(const int dev, const int part,
                           settings_store_t *store, size_t *offset)
{
    struct nvmPartition pInfo;
    int ret = nvm_getPart(dev, part, &pInfo);
    if (ret < 0)
        return ret;

    ssize_t end_lim = pInfo.size;
    *offset = 0;

    while (end_lim > 0) {
        size_t read_offset = 0;

        // Find beginning of latest store
        int ret = find_last_store(dev, part, &read_offset, end_lim);
        if (ret == -EILSEQ)
            return 0; // Corrupt partition
        else if (ret == -ENOENT)
            return 1; // Empty partition
        else if (ret < 0)
            return ret;

        ret = read_store(dev, part, read_offset, store);
        if(ret == -E2BIG)
        {
            end_lim -= store->length; // Skip this store
            continue;
        }
        else if (ret < 0)
            return ret;

        // Free space will be right after the first store found (last in partition)
        if (*offset == 0)
            *offset = read_offset + store->length;

        ret = check_store_integrity(store);
        if (ret == 1) // Valid
            return 3;
        else if (ret == 0) // Invalid
            end_lim = read_offset; // Limit the parsing to right before this store
        else if (ret == -1) // Stale
            return 2;
    }

    return 0; // No valid store, partition is considered corrupted
}

/**
 * Write a settings store to an NVM partition, erasing the partition if requested
 * or if there is not enough space left in the partition
 *
 * @param dev NVM device to write to.
 * @param part NVM partition to write to.
 * @param store pointer to the store to write.
 * @param offset where to write the store. Will be updated accordingly after
 *               writing. Can be smaller if the partition was erased.
 * @param erase if the partition needs to be erased before writing the store.

 * @return 0 if successful, negative error code otherwise
 */
int write_store(const int dev, const int part, const settings_store_t *store,
                size_t *offset, bool erase)
{
    struct nvmPartition pInfo;
    int ret = nvm_getPart(dev, part, &pInfo);
    if (ret < 0)
        return ret;

    // Check if we have enough space to write the store
    if ((*offset + sizeof(settings_store_t)) > pInfo.size)
        erase = true;

    if (erase) {
        ret = nvm_erase(dev, part, 0, pInfo.size);
        if (ret == -ENOTSUP) {
            // If device does not implement erase function (such as posix files)
            // then we manually write 0xFFFFFFFF everywhere
            const uint32_t ff = 0xFFFFFFFF;
            size_t i = 0;
            for (; i < pInfo.size - 4; i += 4) {
                ret = nvm_write(dev, part, i, &ff, 4);
                if (ret < 0)
                    return ret;
            }
            for (; i < pInfo.size; i++) {
                ret = nvm_write(dev, part, i, &ff, 1);
                if (ret < 0)
                    return ret;
            }
        } else if (ret < 0)
            return ret;

        *offset = 0;
    }

    ret = nvm_write(dev, part, *offset, (void *)store,
                    sizeof(settings_store_t));
    if (ret < 0)
        return ret;

    *offset += sizeof(settings_store_t);

    return 0;
}

/**
 * Prints the content of a settings store
 *
 * @param store store to print
 */
void print_store(settings_store_t *store)
{
    printf("Store at address 0x%08zX\r\n", (size_t)(store));
    printf("\tMAGIC=0x%08X\r\n", (unsigned int)store->MAGIC);
    printf("\tlength=%d\r\n", store->length);
    printf("\tcounter=%d\r\n", store->counter);
    printf("\tsettings=0x");
    uint8_t *tmp = (uint8_t *)&(store->settings);
    for (size_t i = 0; i < sizeof(settings_t); i++) {
        printf("%02X", tmp[i]);
    }
    printf("\r\n\tcrc=0x%04X\r\n", store->crc);
}

/**
 * Prints the binary content of a settings structure
 *
 * @param settings
 */
void print_settings(settings_t *settings)
{
    printf("Settings at address 0x%08zX\r\n", (size_t)(settings));
    printf("\tcontent=0x");
    uint8_t *tmp = (uint8_t *)(settings);
    for (size_t i = 0; i < sizeof(settings_t); i++) {
        printf("%02X", tmp[i]);
    }
    printf("\r\n");
}

/**
 * Populates the latest store of a settings_storage_t struct with the latest
 * non-corrupted version available across both partitions.
 *
 * @param s storage to populate
 * @return 0 in case of success, negative error code otherwise
 */
int populate_latest_store(settings_storage_t *s)
{
    // One of each per partition
    settings_store_t store_A, store_B;
    size_t offset_A, offset_B;
    bool store_A_stale = false, store_B_stale = false;

    int ret = get_latest_valid_store(s->dev, s->part_A, &store_A, &offset_A);
    if (ret < 0)
        return ret;
    else if (ret == 0)
        s->part_A_status = PART_CORRUPTED;
    else if (ret == 1) {
        s->part_A_offset = 0;
        s->part_A_status = PART_EMPTY;
    } else if (ret == 2) {// Latest store in part A is stale
        s->part_A_offset = offset_A;
        s->part_A_status = PART_CLEAN; // Partition is clean
        store_A_stale = true;
    } else if (ret == 3) {  // Latest store in part A is valid
        s->part_A_offset = offset_A;
        s->part_A_status = PART_CLEAN;
    }

    ret = get_latest_valid_store(s->dev, s->part_B, &store_B, &offset_B);
    if (ret < 0)
        return ret;
    else if (ret == 0)
        s->part_B_status = PART_CORRUPTED;
    else if (ret == 1) {
        s->part_B_offset = 0;
        s->part_B_status = PART_EMPTY;
    } else if (ret == 2) {  // Latest store in part B is stale
        s->part_B_offset = offset_B;
        s->part_B_status = PART_CLEAN; // Partition is clean
        store_B_stale = true;
    } else if (ret == 3) {  // Latest store in part B is valid
        s->part_B_offset = offset_B;
        s->part_B_status = PART_CLEAN;
    }

    // Do something with the results
    if (s->part_A_status == PART_CLEAN && s->part_B_status == PART_CLEAN) {
        if (store_A.counter >= store_B.counter) {
            s->latest_store = store_A;
            s->write_needed = store_A_stale;
        } else {
            s->latest_store = store_B;
            s->write_needed = store_B_stale;
        }
        s->initialized = true;
    } else if (s->part_A_status == PART_CLEAN && s->part_B_status != PART_CLEAN) {
        s->latest_store = store_A;
        s->write_needed = store_A_stale;
        s->initialized = true;
    } else if (s->part_A_status != PART_CLEAN && s->part_B_status == PART_CLEAN) {
        s->latest_store = store_B;
        s->initialized = true;
        s->write_needed = store_B_stale;
    } else {
        // Init store with default settings
        ret = default_settings_store(&(s->latest_store));
        if (ret < 0)
            return ret;
        s->initialized = true;
        s->write_needed = true;
    }

    return 0;
}

int settings_storage_init(settings_storage_t *s, const int nvm_dev,
                          const int part_A, const int part_B)
{
    s->dev = nvm_dev;
    s->part_A = part_A;
    s->part_B = part_B;
    s->initialized = false;
    return default_settings_store(&(s->latest_store));
}

int settings_storage_load(settings_storage_t *s, settings_t *settings)
{
    // Check if we already read the settings
    if (!s->initialized) {
        int ret = populate_latest_store(s);
        if (ret < 0)
            return ret;
    }
    *settings = s->latest_store.settings;
    return 0;
}

int settings_storage_save(settings_storage_t *s, const settings_t *settings)
{
    int settings_comparison = memcmp(&(s->latest_store.settings), settings,
                                     sizeof(settings_t));

    if ((settings_comparison != 0) || (s->write_needed == true)) {
        // Update settings. If settings were not changed but write_needed is true
        // this will increment counter and update crc
        int ret = update_settings_store(settings, &(s->latest_store));
        if (ret < 0)
            return ret;
        s->write_needed = true;
    }

    if (s->write_needed) {
        if (s->latest_store.counter % 2) // Write to part B
        {
            int ret = write_store(s->dev, s->part_B, &(s->latest_store),
                                  &(s->part_B_offset),
                                  (s->part_B_status == PART_CORRUPTED));
            if (ret < 0)
                return ret;
            s->part_B_status = PART_CLEAN; // Partition is now clean
        } else                        // Write to part A
        {
            int ret = write_store(s->dev, s->part_A, &(s->latest_store),
                                  &(s->part_A_offset),
                                  (s->part_A_status == PART_CORRUPTED));
            if (ret < 0)
                return ret;
            s->part_A_status = PART_CLEAN; // Partition is now clean
        }
    }

    s->write_needed = false;
    return 0;
}
