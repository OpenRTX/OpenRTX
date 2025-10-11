/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/nvmem.h"
#include <limits.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include "posix_file.h"

int posixFile_init(struct nvmFileDevice *dev, const char *fileName,
                   const size_t size)
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
    ftruncate(fd, size);

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

    lseek(pDev->fd, offset, SEEK_SET);
    return read(pDev->fd, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    struct nvmFileDevice *pDev = (struct nvmFileDevice *)(dev);

    if(pDev->fd < 0)
        return -EBADF;

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
