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
#include "drivers/NVM/posix_file.h"
#include "core/nvmem_access.h"
#include "interfaces/nvmem.h"

#define NVM_MAX_PATHLEN 256

POSIX_FILE_DEVICE_DEFINE(stateDevice)

const struct nvmPartition statePartitions[] =
{
    {
        .offset = 0x0000,   // First partition, radio state
        .size   = 512
    },
    {
        .offset = 0x0200,   // Second partition, settings
        .size   = 512
    }
};

const struct nvmDescriptor stateNvm =
{
    .name       = "Device state NVM area",
    .dev        = (const struct nvmDevice *) &stateDevice,
    .baseAddr   = 0x00000000,
    .size       = 1024,
    .nbPart     = sizeof(statePartitions) / sizeof(struct nvmPartition),
    .partitions = statePartitions
};

const struct nvmTable nvmTab = {
    .areas = &stateNvm,
    .nbAreas = 1,
};

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

    strcat(memory_path, "state.bin");

    int ret = posixFile_init(&stateDevice, memory_path, 1024);
    if(ret < 0)
        printf("Opening of state file failed with status %d\n", ret);

    return;

toolong:
    printf("Expected path was too long\n");
    exit(1);
}

void nvm_terminate()
{
    posixFile_terminate(&stateDevice);
}

const struct nvmDescriptor *nvm_getDesc(const size_t index)
{
    if(index > 0)
        return NULL;

    return &stateNvm;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    /* Linux devices does not have any hardware info in the external flash. */
    (void) info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    int ret = nvm_read(0, 0, 0, channel, sizeof(channel_t));
    if(ret < 0)
        return ret;

    // TODO: implement a more serious integrity check
    for(size_t i = 0; i < sizeof(channel_t); i++)
    {
        const uint8_t *p = (const uint8_t *) channel;
        if(p[i] != 0x00)
            return 0;
    }

    return -1;
}

int nvm_readSettings(settings_t *settings)
{
    int ret = nvm_read(0, 1, 0, settings, sizeof(settings_t));
    if(ret < 0)
        return ret;

    // TODO: implement a more serious integrity check
    for(size_t i = 0; i < sizeof(settings_t); i++)
    {
        const uint8_t *p = (const uint8_t *) settings;
        if(p[i] != 0x00)
            return 0;
    }

    return -1;
}

int nvm_writeSettings(const settings_t *settings)
{
    return nvm_write(0, 1, 0, settings, sizeof(settings_t));
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    int ret = nvm_write(0, 1, 0, settings, sizeof(settings_t));
    if(ret < 0)
        return ret;

    return nvm_write(0, 0, 0, vfo, sizeof(channel_t));
}
