/***************************************************************************
 *   Copyright (C) 2020 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <interfaces/nvmem.h>
#include <sys/stat.h>
#include <sys/errno.h>

#define _NVM_MAX_PATHLEN 256

// path indexes for memory_paths
enum path_idxs
{
    P_SETTINGS = 0,
    P_VFO,
    P_LEN
};

// absolute paths to files storing chunk of memory
char *memory_paths[P_LEN];

// Simulate CPS with 16 channels, 16 zones, 16 contacts
const uint32_t maxNumZones    = 16;
const uint32_t maxNumChannels = 16;
const uint32_t maxNumContacts = 16;
const freq_t dummy_base_freq = 145500000;

/**
 * Creates a directory if it does not exist.
 *
 * \param path: the directory path
 * \return 0 on success, -1 on failure
 */
int _nvm_mkdir(const char *path)
{
    // TODO;
    return 0;
}

/**
 * Write binary information into a file
 *
 * \param path: the file to write
 * \param data: the struct to write
 * \param size: the size of the data to write
 * \return 0 on success, -1 on failure
 */
int _nvm_write(const char *path, const void *data, size_t size)
{
    // TODO;
    return 0;
}

/**
 * Read binary information into a file
 *
 * \param path: the file to read
 * \param data: the struct to populate with the file content
 * \param size: the size of the data to read
 * \return 0 on success, -1 on failure
 */
int _cps_read(const char *path, void *data, size_t size)
{
    // TODO
    return 0;
}

void nvm_init()
{
    // TODO
    ;
}

void nvm_terminate()
{
    // TODO
    ;
}

void nvm_readHwInfo(hwInfo_t *info)
{
    // TODO
    ;
}

int nvm_readVfoChannelData(channel_t *channel)
{
    // TODO
    return 0;
}

int nvm_readSettings(settings_t *settings)
{
    // TODO
    return 0;
}

int nvm_writeSettings(const settings_t *settings)
{
    // TODO
    return 0;
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    // TODO
    return 0;
}
