/***************************************************************************
 *   Copyright (C) 2023 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#include "interfaces/nvmem.h"
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "posix_file.h"

int posixFile_init(struct nvmFileDevice *dev, const char *fileName)
{
    // struct nvmFileDevice *pDev = (struct nvmFileDevice *)(dev);

    // Test if file exists, if it doesn't create it.
    int flags = O_RDWR;
    int ret   = access(fileName, F_OK);
    if(ret != 0)
        flags |= O_CREAT | O_EXCL;

    // Open file
    int fd = open(fileName, flags, S_IRUSR | S_IWUSR);
    if(fd < 0)
        return fd;

    // Truncate to size, pad with zeroes if extending.
    ftruncate(fd, dev->size);

    dev->fd = fd;

    return 0;
}

int posixFile_terminate(struct nvmFileDevice *dev)
{
    // struct nvmFileDevice *pDev = (struct nvmFileDevice *)(dev);

    if(dev->fd < 0)
        return -EBADF;

    fsync(dev->fd);
    close(dev->fd);

    dev->fd = -1;

    return 0;
}


static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset,
                        void *data, size_t len)
{
    struct nvmFileDevice *pDev = (struct nvmFileDevice *)(dev);

    if(pDev->fd < 0)
        return -EBADF;

    if((offset + len) >= pDev->size)
        return -EINVAL;

    lseek(pDev->fd, offset, SEEK_SET);
    return read(pDev->fd, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    struct nvmFileDevice *pDev = (struct nvmFileDevice *)(dev);

    if(pDev->fd < 0)
        return -EBADF;

    if((offset + len) > pDev->size)
        return -EINVAL;

    lseek(pDev->fd, offset, SEEK_SET);
    return write(pDev->fd, data, len);
}

const struct nvmOps posix_file_ops =
{
    .read   = nvm_api_read,
    .write  = nvm_api_write,
    .erase  = NULL,
    .sync   = NULL,
};

const struct nvmInfo posix_file_info =
{
    .write_size   = 1,
    .erase_size   = 1,
    .erase_cycles = INT_MAX,
    .device_info  = NVM_FILE | NVM_WRITE | NVM_BITWRITE
};
