/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/cps_io.h"
#include <string.h>
#include <stdio.h>
#include <time.h>

#define CPS_CHUNK_SIZE 1024

static FILE *cps_file = NULL;
const char *default_author = "Codeplug author.";
const char *default_descr = "Codeplug description.";

/**
 * Internal: read and validate codeplug header
 *
 * @param header: pointer to the header struct to be populated
 * @return 0 on success, -1 on failure
 */
int _readHeader(cps_header_t *header)
{
    fseek(cps_file, 0L, SEEK_SET);
    fread(header, sizeof(cps_header_t), 1, cps_file);
    // Validate magic number
    if(header->magic != CPS_MAGIC)
        return -1;
    // Validate version number
    if(((header->version_number & 0xff00) >> 8) != CPS_VERSION_MAJOR ||
        (header->version_number & 0x00ff) > CPS_VERSION_MINOR)
        return -1;
    return 0;
}

/**
 * Internal: write codeplug header
 *
 * @param header: header struct to be written
 * @return 0 on success, -1 on failure
 */
int _writeHeader(cps_header_t header)
{
    fseek(cps_file, 0L, SEEK_SET);
    fwrite(&header, sizeof(cps_header_t), 1, cps_file);
    return 0;
}

/**
 * Internal: push down data at a given offset by a given amount
 *
 * @param offset: offset at which to start to push down data
 * @param amount: amount of free space to be created
 * @return 0 on success, -1 on failure
 */

int _pushDown(uint32_t offset, uint32_t amount)
{
    // Get end of file
    fseek(cps_file, 0, SEEK_END);
    long end = ftell(cps_file);
    // If offset equals end, just return
    if (offset == end)
        return 0;
    // Move data downwards in chunks of fixed size
    char buffer[CPS_CHUNK_SIZE] = { 0 };
    for(int i = 1; i <= ((end - offset) / CPS_CHUNK_SIZE); i++)
    {
        fseek(cps_file, end - i * CPS_CHUNK_SIZE, SEEK_SET);
        fread(buffer, CPS_CHUNK_SIZE, 1, cps_file);
        fseek(cps_file, end - i * CPS_CHUNK_SIZE + amount, SEEK_SET);
        fwrite(buffer, CPS_CHUNK_SIZE, 1, cps_file);
    }
    // Once initial offset is reached, move the last incomplete block
    fseek(cps_file, offset, SEEK_SET);
    fread(buffer, (end - offset) % CPS_CHUNK_SIZE, 1, cps_file);
    fseek(cps_file, offset + amount, SEEK_SET);
    fwrite(buffer, (end - offset) % CPS_CHUNK_SIZE, 1, cps_file);
    fseek(cps_file, offset, SEEK_SET);
    return 0;
}

/**
 * Internal: updates the contact numbering after a contact addition or removal
 *
 * @param pos: position at which the new contact was inserted or removed
 * @param add: if true a contact was inserted, otherwise it was removed
 * @return 0 on success, -1 on failure
 */
int _updateCtNumbering(uint16_t pos, bool add)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    for(int i = 0; i < header.ch_count; i++)
    {
        channel_t c = { 0 };
        cps_readChannel(&c, i);
        if (c.mode == OPMODE_M17 && c.m17.contact_index >= pos)
        {
            if (add)
                c.m17.contact_index++;
            else
                c.m17.contact_index--;
            cps_writeChannel(c, i);
        }
        if (c.mode == OPMODE_DMR && c.dmr.contact_index >= pos)
        {
            if (add)
                c.dmr.contact_index++;
            else
                c.dmr.contact_index--;
            cps_writeChannel(c, i);
        }
    }
    return 0;
}

/**
 * Internal: updates the channel numbering after a channel addition or removal
 *
 * @param pos: position at which the new channel was inserted or removed
 * @param add: if true a channel was inserted, otherwise it was removed
 * @return 0 on success, -1 on failure
 */
int _updateChNumbering(uint16_t pos, bool add)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    for(int i = 0; i < header.b_count; i++)
    {
        bankHdr_t b_header = { 0 };
        cps_readBankHeader(&b_header, i);
        for(int j = 0; j < b_header.ch_count; j++)
        {
            int32_t ch = cps_readBankData(i, j);
            if (ch >= pos)
            {
                if (add)
                    ch++;
                else
                    ch--;
                cps_writeBankData(ch, i, j);
            }
        }
    }
    return 0;
}

/**
 * Internal: get bank data offset
 *
 * @param pos: position of the bank to be read
 * @return the offset in the file where the bank data is stored, -1 if error
 */
long _getBankDataOffset(uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.b_count + 1)
        return -1;
    if (pos == header.b_count)
    {
        // No bank is present, no offset to read
        if (header.b_count == 0)
            return ftell(cps_file) +
                   header.ct_count * sizeof(contact_t) +
                   header.ch_count * sizeof(channel_t) +
                   sizeof(uint32_t);
        // Read last bank offset
        fseek(cps_file,
              header.ct_count * sizeof(contact_t) +
              header.ch_count * sizeof(channel_t) +
              (header.b_count - 1) * sizeof(uint32_t),
              SEEK_CUR);
        uint32_t offset = 0;
        fread(&offset, sizeof(uint32_t), 1, cps_file);
        long bdata_pos = ftell(cps_file);
        bankHdr_t last_bank = { 0 };
        cps_readBankHeader(&last_bank, header.b_count - 1);
        return bdata_pos + offset + sizeof(bankHdr_t) + last_bank.ch_count * sizeof(uint32_t);
    }
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    return ftell(cps_file) +
           (header.b_count - pos - 1) * sizeof(uint32_t) +
           offset;
}

int cps_open(char *cps_name)
{
    if (!cps_name)
        cps_name = "default.rtxc";
    cps_file = fopen(cps_name, "r+");
    if (!cps_file)
        return -1;
    return 0;
}

void cps_close()
{
    fclose(cps_file);
}

int cps_create(char *cps_name)
{
    // Clear or create cps file
    FILE *new_cps = NULL;
    if (!cps_name)
        cps_name = "default.rtxc";
    new_cps = fopen(cps_name, "w");
    if (!new_cps)
        return -1;
    // Write new header
    cps_header_t header = { 0 };
    header.magic = CPS_MAGIC;
    header.version_number = CPS_VERSION_MAJOR << 8 | CPS_VERSION_MINOR;
    strncpy(header.author, default_author, 17);
    strncpy(header.descr, default_descr, 23);
    // TODO: Implement unix timestamp in Miosix
    header.timestamp = time(NULL);
    header.ct_count = 0;
    header.ch_count = 0;
    header.b_count = 0;
    fwrite(&header, sizeof(cps_header_t), 1, new_cps);
    fclose(new_cps);
    return 0;
}

int cps_readContact(contact_t *contact, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.ct_count)
        return -1;
    fseek(cps_file, pos * sizeof(contact_t), SEEK_CUR);
    fread(contact, sizeof(contact_t), 1, cps_file);
    return 0;
}

int cps_readChannel(channel_t *channel, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.ch_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          pos * sizeof(channel_t),
          SEEK_CUR);
    fread(channel, sizeof(channel_t), 1, cps_file);
    return 0;
}

int cps_readBankHeader(bankHdr_t *b_header, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.b_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    fseek(cps_file, (header.b_count - pos - 1) * sizeof(uint32_t) + offset, SEEK_CUR);
    fread(b_header, sizeof(bankHdr_t), 1, cps_file);
    return 0;
}

int cps_readBankData(uint16_t bank_pos, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (bank_pos >= header.b_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          bank_pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    fseek(cps_file, (header.b_count - bank_pos - 1) * sizeof(uint32_t) + offset, SEEK_CUR);
    bankHdr_t b_header = { 0 };
    fread(&b_header, sizeof(bankHdr_t), 1, cps_file);
    if (pos >= b_header.ch_count)
        return -1;
    fseek(cps_file, pos * sizeof(uint32_t), SEEK_CUR);
    uint32_t ch_index = 0;
    fread(&ch_index, sizeof(uint32_t), 1, cps_file);
    return ch_index;
}

int cps_writeContact(contact_t contact, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.ct_count)
        return -1;
    fseek(cps_file, pos * sizeof(contact_t), SEEK_CUR);
    fwrite(&contact, sizeof(contact_t), 1, cps_file);
    return 0;
}

int cps_writeChannel(channel_t channel, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.ch_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          pos * sizeof(channel_t),
          SEEK_CUR);
    fwrite(&channel, sizeof(channel_t), 1, cps_file);
    return 0;
}

int cps_writeBankHeader(bankHdr_t b_header, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.b_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    fseek(cps_file, header.b_count - pos * sizeof(uint32_t) + offset, SEEK_CUR);
    fwrite(&b_header, sizeof(bankHdr_t), 1, cps_file);
    return 0;
}

int cps_writeBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.b_count + 1)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          bank_pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    fseek(cps_file, (header.b_count - bank_pos - 1) * sizeof(uint32_t) + offset, SEEK_CUR);
    bankHdr_t b_header = { 0 };
    fread(&b_header, sizeof(bankHdr_t), 1, cps_file);
    if (pos >= b_header.ch_count)
        return -1;
    fseek(cps_file, pos * sizeof(uint32_t), SEEK_CUR);
    fwrite(&ch, sizeof(uint32_t), 1, cps_file);
    return 0;
}

int cps_insertContact(contact_t contact, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.ct_count + 1)
        return -1;
    long ct_pos = ftell(cps_file) + pos * sizeof(contact_t);
    _pushDown(ct_pos, sizeof(contact_t));
    fwrite(&contact, sizeof(contact_t), 1, cps_file);
    header.ct_count++;
    _writeHeader(header);
    if (_updateCtNumbering(pos, true))
        return -1;
    return 0;
}

int cps_insertChannel(channel_t channel, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.ch_count + 1)
        return -1;
    long ch_pos = ftell(cps_file) +
                  header.ct_count * sizeof(contact_t) +
                  pos * sizeof(channel_t);
    _pushDown(ch_pos, sizeof(channel_t));
    fwrite(&channel, sizeof(channel_t), 1, cps_file);
    header.ch_count++;
    _writeHeader(header);
    _updateChNumbering(pos, true);
    return 0;
}

int cps_insertBankHeader(bankHdr_t b_header, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (pos >= header.b_count + 1)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          pos * sizeof(uint32_t),
          SEEK_CUR);
    long b_offset_pos = ftell(cps_file);
    uint32_t b_offset = _getBankDataOffset(pos) - _getBankDataOffset(0);
    // Read position of the new offset
    _pushDown(b_offset_pos, sizeof(uint32_t));
    fwrite(&b_offset, sizeof(uint32_t), 1, cps_file);
    // Update all the offsets following the moved bank
    for(int i = 0; i < header.b_count - pos; i++)
    {
        long p = ftell(cps_file);
        uint32_t o = 0;
        fread(&o, sizeof(uint32_t), 1, cps_file);
        fseek(cps_file, p, SEEK_SET);
        o += sizeof(bankHdr_t);
        fwrite(&o, sizeof(uint32_t), 1, cps_file);
    }
    header.b_count++;
    _writeHeader(header);
    _pushDown(_getBankDataOffset(pos), sizeof(bankHdr_t));
    fwrite(&b_header, sizeof(bankHdr_t), 1, cps_file);
    return 0;
}

int cps_insertBankData(uint32_t ch, uint16_t bank_pos, uint16_t pos)
{
    cps_header_t header = { 0 };
    if (_readHeader(&header))
        return -1;
    if (bank_pos >= header.b_count)
        return -1;
    fseek(cps_file,
          header.ct_count * sizeof(contact_t) +
          header.ch_count * sizeof(channel_t) +
          bank_pos * sizeof(uint32_t),
          SEEK_CUR);
    uint32_t offset = 0;
    fread(&offset, sizeof(uint32_t), 1, cps_file);
    // Update all the offsets following the moved bank
    for(int i = 0; i < header.b_count - bank_pos - 1; i++)
    {
        long p = ftell(cps_file);
        uint32_t o = 0;
        fread(&o, sizeof(uint32_t), 1, cps_file);
        fseek(cps_file, p, SEEK_SET);
        o += sizeof(uint32_t);
        fwrite(&o, sizeof(uint32_t), 1, cps_file);
    }
    // Update bank header
    fseek(cps_file, offset, SEEK_CUR);
    bankHdr_t b_header = { 0 };
    long h_pos = ftell(cps_file);
    fread(&b_header, sizeof(bankHdr_t), 1, cps_file);
    if (pos >= b_header.ch_count + 1)
        return -1;
    b_header.ch_count++;
    fseek(cps_file, h_pos, SEEK_SET);
    fwrite(&b_header, sizeof(bankHdr_t), 1, cps_file);
    fseek(cps_file, pos * sizeof(uint32_t), SEEK_CUR);
    long p = ftell(cps_file);
    _pushDown(p, sizeof(uint32_t));
    fwrite(&ch, sizeof(uint32_t), 1, cps_file);
    return 0;
}
