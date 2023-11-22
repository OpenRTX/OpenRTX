/***************************************************************************
 *   Copyright (C) 2023 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
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

#include <interfaces/nvmem.h>
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include "posix_file.h"

static const struct nvmParams posix_file_params =
{
    .write_size   = 1,
    .erase_size   = 1,
    .erase_cycles = INT_MAX,
    .type         = NVM_FILE
};


int posixFile_init(const struct nvmDevice *dev, const char *fileName)
{
    const struct posixFileCfg *cfg = (const struct posixFileCfg *)(dev->config);
    const char *name = cfg->fileName;

    // Override filename from config, if a new one is provided.
    if(fileName != NULL)
        name = fileName;

    // Test if file exists, if it doesn't create it.
    int flags = O_RDWR;
    int ret   = access(name, F_OK);
    if(ret != 0)
        flags |= O_CREAT | O_EXCL;

    // Open file
    int fd = open(name, flags, S_IRUSR | S_IWUSR);
    if(fd < 0)
        return fd;

    // Truncate to size, pad with zeroes if extending.
    ftruncate(fd, cfg->fileSize);

    *(int *)(dev->priv) = fd;

    return 0;
}

int posixFile_terminate(const struct nvmDevice *dev)
{
    int fd = *(int *)(dev->priv);
    if(fd < 0)
        return -EBADF;

    fsync(fd);
    close(fd);

    *(int *)(dev->priv) = -1;

    return 0;
}


static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset,
                        void *data, size_t len)
{
    const struct posixFileCfg *cfg = (const struct posixFileCfg *)(dev->config);
    const int fd = *(int *)(dev->priv);

    if(fd < 0)
        return -EBADF;

    if((offset + len) >= cfg->fileSize)
        return -EINVAL;

    lseek(fd, offset, SEEK_SET);
    return read(fd, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    const struct posixFileCfg *cfg = (const struct posixFileCfg *)(dev->config);
    const int fd = *(int *)(dev->priv);

    if(fd < 0)
        return -EBADF;

    if((offset + len) >= cfg->fileSize)
        return -EINVAL;

    lseek(fd, offset, SEEK_SET);
    return write(fd, data, len);
}

static const struct nvmParams *nvm_api_params(const struct nvmDevice *dev)
{
    (void) dev;

    return &posix_file_params;
}

const struct nvmApi posix_file_api =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = NULL,
    .sync   = NULL,
    .params = nvm_api_params
};
