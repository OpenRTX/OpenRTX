/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/nvmem.h"
#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "emulated_flash.h"

int emulatedFlash_init(struct nvmEmulatedFlashDevice *dev, const char *fileName,
                       const size_t size)
{
    // struct nvmFileDevice *pDev = (struct nvmFileDevice *)(dev);

    // Test if file exists, if it doesn't create it.
    int flags = O_RDWR;
    int ret = access(fileName, F_OK);
    if (ret != 0)
        flags |= O_CREAT | O_EXCL;

    // Open file
    int fd = open(fileName, flags, S_IRUSR | S_IWUSR);
    if (fd < 0)
        return fd;

    // Truncate to size, pad with zeroes if extending.
    ftruncate(fd, size);

    dev->fd = fd;

    return 0;
}

int emulatedFlash_terminate(struct nvmEmulatedFlashDevice *dev)
{
    if (dev->fd < 0)
        return -EBADF;

    fsync(dev->fd);
    close(dev->fd);

    dev->fd = -1;

    return 0;
}

static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset,
                        void *data, size_t len)
{
    struct nvmEmulatedFlashDevice *pDev =
        (struct nvmEmulatedFlashDevice *)(dev);

    if (pDev->fd < 0)
        return -EBADF;

    lseek(pDev->fd, offset, SEEK_SET);
    return read(pDev->fd, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    struct nvmEmulatedFlashDevice *pDev =
        (struct nvmEmulatedFlashDevice *)(dev);

    if (pDev->fd < 0)
        return -EBADF;

    lseek(pDev->fd, offset, SEEK_SET);
    const uint8_t *input = (const uint8_t *)data;
    uint8_t *tmp = (uint8_t *)malloc(len);
    if (tmp == NULL)
        return -ENOMEM;

    int ret = read(pDev->fd, tmp, len);
    if (ret != len) {
        free(tmp);
        return -EIO;
    }

    for (size_t i = 0; i < len; i++)
        tmp[i] &= input[i];

    lseek(pDev->fd, offset, SEEK_SET);
    ret = write(pDev->fd, tmp, len);
    free(tmp);
    return ret;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t address,
                         size_t len)
{
    struct nvmEmulatedFlashDevice *pDev =
        (struct nvmEmulatedFlashDevice *)(dev);

    if (pDev->fd < 0)
        return -EBADF;

    lseek(pDev->fd, address, SEEK_SET);

    int ret = 0;
    const uint64_t ff = 0xFFFFFFFFFFFFFFFF;
    size_t to_write = len;
    while (to_write > 0) {
        size_t n = (to_write > 8) ? 8 : to_write;
        ret = write(pDev->fd, &ff, n);
        to_write -= ret;
        if (ret < 0)
            return ret;
    }
    return len;
}

const struct nvmOps emulated_flash_ops = {
    .read = nvm_api_read,
    .write = nvm_api_write,
    .erase = nvm_api_erase,
    .sync = NULL,
};

const struct nvmInfo emulated_flash_info = { .write_size = 1,
                                             .erase_size = 4096,
                                             .erase_cycles = INT_MAX,
                                             .device_info = NVM_FILE | NVM_WRITE
                                                          | NVM_ERASE };
