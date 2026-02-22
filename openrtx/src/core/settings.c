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

#define SETTINGS_MAGIC 0x584E504F // "OPNX"

enum storeIndex {
    STORE_A = 0,
    STORE_B = 1,

    STORES_NUM
};

enum storeStatus {
    STORE_INVALID = 0,
    STORE_STALE = 1,
    STORE_VALID = 2,
};

/**
 * Structure defining the header of the settings store
 */
struct storeHeader {
    uint32_t magic;   ///< Must be 0x584E504F ("OPNX")
    uint16_t length;  ///< Total length of the structure (incl. header)
    uint16_t counter; ///< Free-running counter
    uint16_t crc;     ///< Checksum of the structure (crc field set to 0)
} __attribute__((packed));

/**
 * Structure defining the binary layout of the settings in NVM memory
 */
struct settingsStore {
    struct storeHeader header;
    struct settings settings;
} __attribute__((packed));

struct settingsArea {
    uint32_t dev;
    uint32_t part;
};

struct settingsStorage {
    struct settingsArea areas[STORES_NUM];
    struct settingsStore store;
    bool initialized;
    bool writeNeeded;
};

static struct settingsStorage s;

#ifdef SETTINGS_DBG
static void printStore(struct settingsStore *store)
{
    printf("Store at address 0x%08X\r\n", (size_t)(store));
    printf("\tHeader:");
    printf("\t\tmagic=0x%08X\r\n", (unsigned int)store->header.magic);
    printf("\t\tlength=%d\r\n", store->header.length);
    printf("\t\tcounter=%d\r\n", store->header.counter);
    printf("\t\tcrc=0x%04X\r\n", store->header.crc);
    printf("\tsettings=0x");

    uint8_t *tmp = (uint8_t *)&(store->settings);
    for (size_t i = 0; i < store->header.length; i++)
        printf("%02X", tmp[i]);

    printf("\r\n");
}

static void printSettings(struct settings *settings)
{
    printf("Settings at address 0x%08X\r\n", (size_t)(settings));
    printf("\tcontent=0x");

    uint8_t *tmp = (uint8_t *)(settings);
    for (size_t i = 0; i < sizeof(struct settings); i++)
        printf("%02X", tmp[i]);

    printf("\r\n");
}
#endif

static uint16_t settingsCrc16(const struct settingsStore *store)
{
    struct settingsStore tmp;
    memcpy(&tmp, store, sizeof(struct settingsStore));

    tmp.header.crc = 0;
    return crc_ccitt(&tmp, tmp.header.length);
}

static void initStore(struct settingsStore *store)
{
    store->header.magic = SETTINGS_MAGIC;
    store->header.counter = 0;
    store->settings = default_settings;
    store->header.length = sizeof(struct settings);
    store->header.crc = settingsCrc16(store);
}

static uint16_t updateStore(struct settingsStore *store,
                        const struct settings *settings)
{
    store->header.counter++;
    memcpy(&store->settings, settings, sizeof(struct settings));
    store->header.length = sizeof(struct settings);
    store->header.crc = settingsCrc16(store);

    return store->header.counter;
}

static int readAndCheckStore(const uint32_t dev, const uint32_t part,
                             struct settingsStore *store)
{
    int ret = nvm_read(dev, part, 0, store, sizeof(struct settingsStore));
    if (ret)
        return STORE_INVALID;

    if (store->header.magic != SETTINGS_MAGIC)
        return STORE_INVALID;

    if (store->header.length > sizeof(struct settings))
        return STORE_INVALID;

    if (store->header.crc != settingsCrc16(store))
        return STORE_INVALID;

    if (store->header.length < sizeof(struct settings)) {
        size_t diff = sizeof(struct settings) - store->header.length;
        uint8_t *dst = (uint8_t *)&store->settings + store->header.length;
        uint8_t *src = (uint8_t *)&default_settings + store->header.length;

        memcpy(dst, src, diff);
        return STORE_STALE;
    }

    return STORE_VALID;
}

static int loadSettings(struct settingsStorage *s)
{
    struct settingsStore store[STORES_NUM];
    int status[STORES_NUM] = {STORE_INVALID};
    int selected = STORE_A;

    for (int i = 0; i < STORES_NUM; i++) {
        if (s->areas[i].dev == NVM_DEVICE_NONE)
            continue;

        status[i] = readAndCheckStore(s->areas[i].dev, s->areas[i].part,
                                      &store[i]);
    }

    // No usable store
    if ((status[STORE_A] == STORE_INVALID) && (status[STORE_B] == STORE_INVALID))
        return -1;

    // Select B if better than A based on VALID > STALE > INVALID
    if (status[STORE_B] > status[STORE_A])
        selected = STORE_B;

    // Both valid, select B if more recent
    if((status[STORE_B] == status[STORE_A]) && 
       (store[STORE_B].header.counter > store[STORE_A].header.counter))
        selected = STORE_B;

    s->store = store[selected];
    s->writeNeeded = (status[selected] == STORE_STALE);

    return 0;
}

int settings_init(const uint32_t devA, const uint32_t partA,
                  const uint32_t devB, const uint32_t partB)
{
    s.areas[STORE_A].dev = devA;
    s.areas[STORE_A].part = partA;
    s.areas[STORE_B].dev = devB;
    s.areas[STORE_B].part = partB;
    s.initialized = false;
    initStore(&s.store);

    return 0;
}

int settings_load(struct settings *settings)
{
    int ret = 0;

    if (!s.initialized) {
        ret = loadSettings(&s);
        s.initialized = true;
    }

    memcpy(settings, &s.store.settings, sizeof(struct settings));

    return ret;
}

int settings_save(const struct settings *settings)
{
    int diff, ret = 0;

    diff = memcmp(&s.store.settings, settings, sizeof(struct settings));
    if ((diff != 0) || (s.writeNeeded == true)) {
        uint16_t area;
        int ret;

        area = updateStore(&s.store, settings) % STORES_NUM;
        if (s.areas[area].dev == NVM_DEVICE_NONE)
            area = (area + 1) % STORES_NUM;

        ret = nvm_write(s.areas[area].dev, s.areas[area].part, 0, &s.store,
                        sizeof(struct settingsStore));
    }

    s.writeNeeded = false;

    return ret;
}
