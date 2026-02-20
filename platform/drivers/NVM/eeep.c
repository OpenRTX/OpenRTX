/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/nvmem_access.h"
#include <stdint.h>
#include <limits.h>
#include <errno.h>
#include "eeep.h"

// NOTE: cannot use an enum for these because enum type is "int"
#define EEEP_PAGE_ERASED      (0xFFFFFFFF)
#define EEEP_PAGE_VERIFIED    (0x00FFFFFF)
#define EEEP_PAGE_COPYING     (0x0000FFFF)
#define EEEP_PAGE_ACTIVE      (0x000000FF)
#define EEEP_PAGE_INACTIVE    (0x00000000)
#define EEEP_PAGE_HDR_SIZE    sizeof(uint32_t)

enum RecordStatus
{
    EEEP_RECORD_EMPTY   = 0xFF,
    EEEP_RECORD_INVALID = 0x55,
    EEEP_RECORD_VALID   = 0x05,
    EEEP_RECORD_ERASED  = 0x00
};

struct eeepRecord
{
    uint8_t  status;
    uint8_t  size;
    uint16_t virtAddr;
};

struct eeepEntry
{
    uint32_t physAddr;
    uint16_t virtAddr;
};

static uint32_t nextRecordAddress(const uint32_t addr, const struct eeepRecord *rec)
{
    uint32_t nextAddr = addr;
    switch(rec->status)
    {
        case EEEP_RECORD_EMPTY:
            break;

        case EEEP_RECORD_INVALID:
            if(rec->size != 0xFF)
                nextAddr += rec->size;
            break;

        case EEEP_RECORD_VALID:
        case EEEP_RECORD_ERASED:
            nextAddr += rec->size;
            break;
    }

    // Next record is at least one record header ahead of the current address.
    return nextAddr + sizeof(struct eeepRecord);
}

static int findRecord(struct eeepData *priv, uint32_t *memAddr, const uint16_t virtAddr)
{
    struct eeepRecord rec;
    uint32_t addr = priv->readAddr;
    *memAddr = 0xFFFFFFFF;

    while(addr < priv->writeAddr)
    {
        int ret = nvm_devRead(priv->nvm, addr, &rec, sizeof(struct eeepRecord));
        if(ret < 0)
            return ret;

        if((rec.status == EEEP_RECORD_VALID) && (rec.virtAddr == virtAddr))
            *memAddr = addr;

        addr = nextRecordAddress(addr, &rec);
    }

    if(*memAddr != 0xFFFFFFFF)
        return 0;

    return -1;
}

static int writeRecord(struct eeepData *priv, uint16_t virtAddr, const void *data,
                       size_t len)
{
    uint32_t dataAddr = priv->writeAddr + sizeof(struct eeepRecord);
    uint32_t headAddr = priv->writeAddr;
    struct eeepRecord rec =
    {
        .status   = EEEP_RECORD_INVALID,
        .size     = len,
        .virtAddr = virtAddr
    };

    // Write record header
    int ret = nvm_devWrite(priv->nvm, headAddr, &rec, sizeof(struct eeepRecord));
    if(ret < 0)
        return ret;

    // Write data. If the operation fails, it is assumed anyways that the entire
    // data block has been written. This to be sure that following writes do not
    // end up in a partially-written space.
    priv->writeAddr += sizeof(struct eeepRecord) + len;
    ret = nvm_devWrite(priv->nvm, dataAddr, data, len);
    if(ret < 0)
        return ret;

    // Finally, update the record header changing the state to "valid".
    rec.status = EEEP_RECORD_VALID;
    ret = nvm_devWrite(priv->nvm, headAddr, &rec, sizeof(struct eeepRecord));

    return ret;
}

static int swapBlock(struct eeepData *priv)
{
    // Round-robin page swap.
    // Note: when computing the next block address we have to take into account
    // that readAddr points at the first record after the page header.
    uint32_t currBlock = priv->readAddr - EEEP_PAGE_HDR_SIZE;
    uint32_t nextBlock = currBlock + priv->nvm->info->erase_size;
    if(nextBlock >= (priv->part->offset + priv->part->size))
        nextBlock = priv->part->offset;

    // Erase new page
    int ret = nvm_devErase(priv->nvm, nextBlock, priv->nvm->info->erase_size);
    if(ret < 0)
        return ret;

    // Search for all the active entries
    // TODO: use dynamic memory allocation
    struct eeepEntry entryList[8] = {0};
    uint32_t address = priv->readAddr;
    uint8_t numEntries = 0;

    while(address < priv->writeAddr)
    {
        struct eeepRecord rec;

        ret = nvm_devRead(priv->nvm, address, &rec, sizeof(struct eeepRecord));
        if(ret < 0)
            return ret;

        if(rec.status == EEEP_RECORD_VALID)
        {
            uint8_t pos;
            for(pos = 0; pos < numEntries; pos++)
            {
                // Found record in list, update its physical address
                if(entryList[pos].virtAddr == rec.virtAddr)
                {
                    entryList[pos].physAddr = address;
                    break;
                }
            }

            // Record not found, append it to the list
            if(pos == numEntries)
            {
                entryList[pos].virtAddr = rec.virtAddr;
                entryList[pos].physAddr = address;
                numEntries += 1;
            }
        }

        address = nextRecordAddress(address, &rec);
    }

    // Set new write address, mark the page as a page with an ogoing copy
    priv->writeAddr = nextBlock + sizeof(uint32_t);
    uint32_t tmp    = EEEP_PAGE_COPYING;
    ret = nvm_devWrite(priv->nvm, nextBlock, &tmp, sizeof(uint32_t));
    if(ret < 0)
        return ret;

    // Copy over the records to the new page,
    for(uint8_t i = 0; i < numEntries; i++)
    {
        struct eeepRecord rec;
        uint8_t data[256];
        uint32_t address = entryList[i].physAddr;

        ret = nvm_devRead(priv->nvm, address, &rec, sizeof(struct eeepRecord));
        if(ret < 0)
            return ret;

        address += sizeof(struct eeepRecord);
        ret = nvm_devRead(priv->nvm, address, data, rec.size);
        if(ret < 0)
            return ret;

        ret = writeRecord(priv, rec.virtAddr, data, rec.size);
        if(ret < 0)
            return ret;
    }

    // Finally, set the page as the new active page, invalidate the previous one
    // and update the reading address
    tmp = EEEP_PAGE_ACTIVE;
    ret = nvm_devWrite(priv->nvm, nextBlock, &tmp, sizeof(uint32_t));
    if(ret < 0)
        return ret;

    tmp = EEEP_PAGE_INACTIVE;
    ret = nvm_devWrite(priv->nvm, currBlock, &tmp, sizeof(uint32_t));
    if(ret < 0)
        return ret;

    priv->readAddr = nextBlock + EEEP_PAGE_HDR_SIZE;

    return 0;
}

static int eeep_read(const struct nvmDevice *dev, uint32_t offset, void *data,
                     size_t len)
{
    struct eeepData *priv = (struct eeepData *) dev->priv;
    struct eeepRecord rec;
    uint32_t memAddr;

    if((offset >= 0xFFFF) || (len >= 255))
        return -EINVAL;

    int ret = findRecord(priv, &memAddr, offset);
    if(ret < 0)
        return ret;

    // Found, read infoblock
    ret = nvm_devRead(priv->nvm, memAddr, &rec, sizeof(struct eeepRecord));
    if(ret < 0)
        return ret;

    // Adjust size and read data
    if(rec.size < len)
        len = rec.size;

    memAddr += sizeof(struct eeepRecord);
    ret = nvm_devRead(priv->nvm, memAddr, data, rec.size);

    return ret;
}

static int eeep_write(const struct nvmDevice *dev, uint32_t offset,
                      const void *data, size_t len)
{
    struct eeepData *priv = (struct eeepData *) dev->priv;
    int ret;

    if((offset >= 0xFFFF) || (len >= 255))
        return -EINVAL;

    uint32_t usedSpace = (priv->writeAddr - priv->readAddr) + EEEP_PAGE_HDR_SIZE;
    uint32_t freeSpace = priv->nvm->info->erase_size - usedSpace;
    uint32_t entrySize = sizeof(struct eeepRecord) + len;

    if(entrySize > freeSpace)
    {
        ret = swapBlock(priv);
        if(ret < 0)
            return ret;
    }

    return writeRecord(priv, offset, data, len);
}

int eeep_init(const struct nvmDevice *dev, const uint32_t nvm,
              const uint32_t part)
{
    const struct nvmDescriptor *desc = nvm_getDesc(nvm);
    if(desc == NULL)
        return -EINVAL;

    if(part >= desc->nbPart)
        return -EINVAL;

    uint32_t partAddr = desc->partitions[part].offset;
    uint32_t partSize = desc->partitions[part].size;
    uint32_t pageSize = desc->dev->info->erase_size;

    struct eeepData *priv = (struct eeepData *) dev->priv;
    priv->nvm = desc->dev;
    priv->part = &desc->partitions[part];
    priv->readAddr = 0xFFFFFFFF;

    // Search for an active page, set the read address to the first record
    // immediately after the page header
    for(uint32_t i = 0; i < partSize; i += pageSize)
    {
        uint32_t pageAddr = partAddr + i;
        uint32_t tmp;

        nvm_devRead(priv->nvm, pageAddr, &tmp, sizeof(uint32_t));
        if(tmp == EEEP_PAGE_ACTIVE)
        {
            priv->readAddr = pageAddr + EEEP_PAGE_HDR_SIZE;
            break;
        }
    }

    // If no active page found, erase all the memory and set the first page as
    // active page.
    if(priv->readAddr == 0xFFFFFFFF)
    {
        uint32_t tmp = EEEP_PAGE_ACTIVE;

        nvm_devErase(priv->nvm, partAddr, partSize);
        nvm_devWrite(priv->nvm, partAddr, &tmp, sizeof(uint32_t));
        priv->readAddr = partAddr + EEEP_PAGE_HDR_SIZE;
        priv->writeAddr = priv->readAddr;
    }
    else
    {
        uint32_t addr = priv->readAddr;
        uint32_t end = priv->readAddr + pageSize - EEEP_PAGE_HDR_SIZE;
        priv->writeAddr = end;

        while(addr < end)
        {
            struct eeepRecord rec;
            uint32_t *tmp = (uint32_t *) &rec;

            nvm_devRead(desc->dev, addr, &rec, sizeof(struct eeepRecord));
            if(*tmp == 0xFFFFFFFF)
            {
                priv->writeAddr = addr;
                break;
            }

            addr = nextRecordAddress(addr, &rec);
        }
    }

    return 0;
}

int eeep_terminate(const struct nvmDevice* dev)
{
    (void) dev;

    return 0;
}

const struct nvmOps eeep_ops =
{
    .read   = eeep_read,
    .write  = eeep_write,
    .erase  = NULL,
    .sync   = NULL
};

const struct nvmInfo eeep_info =
{
    .write_size   = 1,
    .erase_size   = 1,
    .erase_cycles = INT_MAX,
    .device_info  = NVM_EEEPROM | NVM_WRITE
};
