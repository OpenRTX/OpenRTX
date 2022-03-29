/***************************************************************************
 *   Copyright (C) 2020 - 2022 by Federico Amedeo Izzo IU2NUO,             *
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
    struct stat sb;

    if(stat(path, &sb) == 0)
    {
        if(S_ISDIR(sb.st_mode))
        {
            return 0;
        }

        printf("%s is not a directory\n", path);
        return -1;
    }
    else if(errno == ENOENT)
    {
        if(mkdir(path, 0700) == 0)
        {
            return 0;
        }

        printf("Cannot create %s. %s\n", path, strerror(errno));
        return -1;
    }
    else
    {
        printf("Cannot stat %s. %s", path, strerror(errno));
        return -1;
    }
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
    printf("Writing %s\n", path);

    FILE * file= fopen(path, "wb");
    if(file != NULL)
    {
        fwrite(data, size, 1, file);
        return fclose(file);
    }

    return -1;
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
    printf("Reading %s\n", path);

    FILE * file= fopen(path, "rb");
    if(file != NULL)
    {
        fread(data, size, 1, file);
        return fclose(file);
    }

    return -1;
}

void nvm_init()
{
    const char *name           = "XDG_STATE_HOME";
    const char *env_state_path = getenv(name);
    const char *openrtx        = "/OpenRTX";

    char memory_path[_NVM_MAX_PATHLEN];

    if(env_state_path)
    {
         if(_nvm_mkdir(env_state_path) != 0)
         {
             exit(1);
         }

         // we append /OpenRTX to env_state_path
         if(strlen(env_state_path) + strlen(openrtx) >= _NVM_MAX_PATHLEN)
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
           >= _NVM_MAX_PATHLEN)
            goto toolong;

        strcpy(memory_path, home);
        strcat(memory_path, local);
        if(_nvm_mkdir(memory_path) != 0)
        {
            exit(1);
        }

        strcat(memory_path, state);
        if(_nvm_mkdir(memory_path) != 0)
        {
            exit(1);
        }

        strcat(memory_path, openrtx);
    }

    if(_nvm_mkdir(memory_path) != 0)
    {
        exit(1);
    }

    // init memory_paths
    const char *file_settings  = "/settings.dat";
    const char *file_vfo       = "/vfo.dat";
    unsigned long base_len     = strlen(memory_path);

    for(enum path_idxs i = 0; i < P_LEN; i++)
    {
        const char *path;
        switch(i)
        {
            case P_SETTINGS:
                path = file_settings;
                break;
            case P_VFO:
                path = file_vfo;
                break;
            case P_LEN:
                continue;
        }

        size_t path_size = strlen(path) + base_len + 1;
        memory_paths[i] = malloc(path_size);

        strcpy(memory_paths[i], memory_path);
        strcat(memory_paths[i], path);
    }

    return;

toolong:
    printf("Expected path was too long\n");
    exit(1);
}

void nvm_terminate()
{
    for(enum path_idxs i = 0; i < P_LEN; i++)
    {
        free(memory_paths[i]);
    }
}

void nvm_loadHwInfo(hwInfo_t *info)
{
    /* Linux devices does not have any hardware info in the external flash. */
    (void) info;
}

int nvm_readVFOChannelData(channel_t *channel)
{
    return _cps_read(memory_paths[P_VFO], channel, sizeof(channel_t));
}

int nvm_readSettings(settings_t *settings)
{
    return _cps_read(memory_paths[P_SETTINGS], settings, sizeof(settings_t));
}

int nvm_writeSettings(const settings_t *settings)
{
    return _nvm_write(memory_paths[P_SETTINGS], settings, sizeof(settings_t));
}

int nvm_writeSettingsAndVfo(const settings_t *settings, const channel_t *vfo)
{
    if(nvm_writeSettings(settings) == 0)
    {
        return _nvm_write(memory_paths[P_VFO], vfo, sizeof(channel_t));
    }

    return -1;
}
