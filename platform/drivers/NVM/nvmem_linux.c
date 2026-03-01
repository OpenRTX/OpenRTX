/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/errno.h>
#include "core/utils.h"
#include "drivers/NVM/posix_file.h"
#include "interfaces/nvmem.h"
#include "core/settings.h"
#include "drivers/NVM/emulated_flash.h"
#include "drivers/NVM/eeep.h"
#include "core/cps.h"
#include "core/vfo.h"

#define NVM_MAX_PATHLEN 256

POSIX_FILE_DEVICE_DEFINE(stateDevice)
EMULATED_FLASH_DEVICE_DEFINE(emulatedFlash)
EEEP_DEVICE_DEFINE(eeepDevice)

const struct nvmPartition statePartitions[] =
{
    {
        .offset = 0x0000,   // Second partition, settings (Part A)
        .size   = 1024
    },
    {
        .offset = 1024,   // Third partition, settings (Part B)
        .size   = 1024,
    }
};

const struct nvmDescriptor storage[] =
{
    {
        .name       = "Device state NVM area",
        .dev        = (const struct nvmDevice *) &stateDevice,
        .baseAddr   = 0x00000000,
        .size       = 2048,
        .nbPart     = ARRAY_SIZE(statePartitions),
        .partitions = statePartitions
    },
    {
        .name       = "Emulated flash area",
        .dev        = (const struct nvmDevice *) &emulatedFlash,
        .baseAddr   = 0x00000000,
        .size       = 8192,
        .nbPart     = 0,
        .partitions = NULL
    },
{
        .name       = "Emulated EEPROM area",
        .dev        = (const struct nvmDevice *) &eeepDevice,
        .baseAddr   = 0x00000000,
        .size       = 0xFFFF, // Virtual address is 16 bits
        .nbPart     = 0,
        .partitions = NULL
    }
};



const struct nvmTable nvmTab = {
    .areas = storage,
    .nbAreas = ARRAY_SIZE(storage),
};

struct settings_storage settings_storage;
struct vfo_storage vfo_storage;

/**
 * Creates a directory if it does not exist.
 *
 * \param path: the directory path
 * \return 0 on success, -1 on failure
 */
static int create_dir(const char *path)
{
    struct stat sb;

    if(stat(path, &sb) == 0)
    {
        if(S_ISDIR(sb.st_mode))
            return 0;

        printf("%s is not a directory\n", path);
    }
    else if(errno == ENOENT)
    {
        if(mkdir(path, 0700) == 0)
            return 0;

        printf("Cannot create %s. %s\n", path, strerror(errno));
    }
    else
    {
        printf("Cannot stat %s. %s", path, strerror(errno));
    }

    return -1;
}


void nvm_init()
{
    const char *env_state_path = getenv("XDG_STATE_HOME");
    const char *openrtx        = "/OpenRTX/";

    char memory_path[NVM_MAX_PATHLEN];
    char emulated_flash_path[NVM_MAX_PATHLEN];

    if(env_state_path)
    {
         if(create_dir(env_state_path) != 0)
             exit(1);

         // we append /OpenRTX to env_state_path
         if(strlen(env_state_path) + strlen(openrtx) >= NVM_MAX_PATHLEN)
             goto toolong;

         strcpy(memory_path, env_state_path);
         strcat(memory_path, openrtx);
    }
    else
    {
        // XDG_STATE_HOME should default to ~/.local/state
        // we build the path directory by directory making sure each one exists

        const char *home = getenv("HOME");
        const char *local = "/.local";
        const char *state = "/state";

        if(strlen(home) + strlen(local) + strlen(state) + strlen(openrtx)
           >= NVM_MAX_PATHLEN)
            goto toolong;

        strcpy(memory_path, home);
        strcat(memory_path, local);
        if(create_dir(memory_path) != 0)
            exit(1);

        strcat(memory_path, state);
        if(create_dir(memory_path) != 0)
            exit(1);

        strcat(memory_path, openrtx);
    }

    if(create_dir(memory_path) != 0)
        exit(1);

    strncpy(emulated_flash_path, memory_path, NVM_MAX_PATHLEN-1);

    strcat(memory_path, "settings.bin");
    strcat(emulated_flash_path, "flash.bin");

    int ret = posixFile_init(&stateDevice, memory_path, 2048);
    if(ret < 0)
    {
        printf("Opening of settings file failed with status %d\n", ret);
        return;
    }

    ret = settings_initStorage(&settings_storage, 0, 1, 2);
    if(ret < 0)
    {
        printf("Intialization of settings storage failed with status %d\n", ret);
        return;
    }

    ret = emulatedFlash_init(&emulatedFlash, emulated_flash_path, 8192);
    if(ret < 0)
    {
        printf("Opening of flash file failed with status %d\n", ret);
        return;
    }

    ret = eeep_init(&eeepDevice, 1, 0);
    if(ret < 0)
    {
        printf("Initialization of emulated eeeprom failed with status %d\n", ret);
        return;
    }

    ret = vfo_initStorage(&vfo_storage, 2, 0, 16, CPS_VERSION_NUMBER);


    return;

toolong:
    printf("Expected path was too long\n");
    exit(1);
}

void nvm_terminate()
{
    emulatedFlash_terminate(&emulatedFlash);
    posixFile_terminate(&stateDevice);
}

void nvm_readHwInfo(hwInfo_t *info)
{
    /* Linux devices does not have any hardware info in the external flash. */
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    return vfo_load(&vfo_storage, channel);
}

int nvm_readSettings(settings_t *settings)
{
    return settings_load(&settings_storage, settings);
}

int nvm_writeSettings(const settings_t *settings)
{
    return settings_save(&settings_storage, settings);
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    int ret = nvm_writeSettings(settings);
    if(ret < 0)
        return ret;

    return vfo_save(&vfo_storage, vfo);
}
