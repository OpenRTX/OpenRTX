/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <limits.h>
#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <fcntl.h>
#include <string.h>
#include <stdlib.h>
#include "interfaces/nvmem.h"
#include "core/nvmem_device.h"
#include "emulated_flash.h"

static int nvm_api_read(const struct nvmDevice *dev, uint32_t offset,
                        void *data, size_t len)
{
    return nvm_devRead(dev->priv, offset, data, len);
}

static int nvm_api_write(const struct nvmDevice *dev, uint32_t offset,
                         const void *data, size_t len)
{
    const uint8_t *input = (const uint8_t *)data;
    uint8_t *tmp;
    int ret;

    tmp = malloc(len);
    if (!tmp)
        return -ENOMEM;

    if (nvm_devRead(dev->priv, offset, tmp, len)) {
        free(tmp);
        return -EIO;
    }

    for (size_t i = 0; i < len; i++)
        tmp[i] &= input[i];

    ret = nvm_devWrite(dev->priv, offset, tmp, len);
    free(tmp);

    return ret;
}

static int nvm_api_erase(const struct nvmDevice *dev, uint32_t address,
                         size_t len)
{
    const uint64_t ff = 0xFFFFFFFFFFFFFFFF;
    size_t to_write = len;

    while (to_write > 0) {
        size_t n = (to_write > 8) ? 8 : to_write;
        int ret = nvm_devWrite(dev->priv, address, &ff, n);
        if (ret < 0)
            return ret;

        to_write -= n;
        address += n;
    }

    return 0;
}

int emulatedFlash_init(struct nvmDevice *dev, const char *fileName,
                       const size_t size)
{
    int ret = posixFile_init((struct nvmFileDevice *)dev->priv, fileName, size);
    if (ret == 1)
        return nvm_api_erase(dev, 0, size);

    return ret;
}

int emulatedFlash_terminate(struct nvmDevice *dev)
{
    return posixFile_terminate((struct nvmFileDevice *)dev->priv);
}

const struct nvmOps emulated_flash_ops = {
    .read = nvm_api_read,
    .write = nvm_api_write,
    .erase = nvm_api_erase,
    .sync = NULL,
};

const struct nvmInfo emulated_flash_info = {
    .write_size = 1,
    .erase_size = 4096,
    .erase_cycles = INT_MAX,
    .device_info = NVM_FILE | NVM_WRITE | NVM_ERASE,
};
