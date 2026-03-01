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
#include "core/settings.h"
#include "core/vfo.h"
#include "interfaces/nvmem.h"

#define NVM_MAX_PATHLEN 256

POSIX_FILE_DEVICE_DEFINE(stateDevice)

const struct nvmPartition statePartitions[] = {
    NVM_PART_DEFINE(0x0000, 512), // First partition, radio state
    NVM_PART_DEFINE(0x0200, 256), // Second partition, settings (Part A)
    NVM_PART_DEFINE(0x0300, 256), // Second partition, settings (Part B)
};

const struct nvmDescriptor stateNvm = {
    .name = "Device state NVM area",
    .dev = (const struct nvmDevice *)&stateDevice,
    .baseAddr = 0x00000000,
    .size = 1024,
    .nbPart = sizeof(statePartitions) / sizeof(struct nvmPartition),
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

    if (stat(path, &sb) == 0) {
        if (S_ISDIR(sb.st_mode))
            return 0;

        printf("%s is not a directory\n", path);
    } else if (errno == ENOENT) {
        if (mkdir(path, 0700) == 0)
            return 0;

        printf("Cannot create %s. %s\n", path, strerror(errno));
    } else {
        printf("Cannot stat %s. %s", path, strerror(errno));
    }

    return -1;
}

void nvm_init()
{
    const char *env_state_path = getenv("XDG_STATE_HOME");
    const char *openrtx = "/OpenRTX/";

    char memory_path[NVM_MAX_PATHLEN];

    if (env_state_path) {
        if (create_dir(env_state_path) != 0)
            exit(1);

        // we append /OpenRTX to env_state_path
        if (strlen(env_state_path) + strlen(openrtx) >= NVM_MAX_PATHLEN)
            goto toolong;

        strcpy(memory_path, env_state_path);
        strcat(memory_path, openrtx);
    } else {
        // XDG_STATE_HOME should default to ~/.local/state
        // we build the path directory by directory making sure each one exists

        const char *home = getenv("HOME");
        const char *local = "/.local";
        const char *state = "/state";

        if (strlen(home) + strlen(local) + strlen(state) + strlen(openrtx)
            >= NVM_MAX_PATHLEN)
            goto toolong;

        strcpy(memory_path, home);
        strcat(memory_path, local);
        if (create_dir(memory_path) != 0)
            exit(1);

        strcat(memory_path, state);
        if (create_dir(memory_path) != 0)
            exit(1);

        strcat(memory_path, openrtx);
    }

    if (create_dir(memory_path) != 0)
        exit(1);

    strcat(memory_path, "state.bin");

    int ret = posixFile_init(&stateDevice, memory_path, 1024);
    if (ret < 0)
        printf("Opening of state file failed with status %d\n", ret);

    settings_init(0, 2, 0, 3);  // Device 0, partitions 2 and 3
    vfo_init(0, 1); // Device 0, partition 1

    return;

toolong:
    printf("Expected path was too long\n");
    exit(1);
}

void nvm_terminate()
{
    posixFile_terminate(&stateDevice);
}

void nvm_readHwInfo(hwInfo_t *info)
{
    /* Linux devices does not have any hardware info in the external flash. */
    (void)info;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    return -1; // Deprecated API, to be removed
}

int nvm_readSettings(settings_t *settings)
{
    return -1; // Deprecated API, to be removed
}

int nvm_writeSettings(const settings_t *settings)
{
    return -1; // Deprecated API, to be removed
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    return -1; // Deprecated API, to be removed
}
