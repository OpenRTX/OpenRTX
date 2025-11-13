/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <new>
#include <cstddef>
#include <cstring>
#include "protocols/M17/MetaText.hpp"

using namespace M17;

MetaText::MetaText()
{
    memset(text, 0, sizeof(text));
    blockMask = 0;
    nextBlockIndex = 0;
}

MetaText::~MetaText()
{
}

void MetaText::reset()
{
    memset(text, 0, sizeof(text));
    blockMask = 0;
    resetBlockPaging();
}

uint8_t MetaText::getBlockIndex(const meta_t &meta)
{
    uint8_t mask = meta.raw_data[0] & 0x0f;
    return mask ? __builtin_ctz(mask) : 0;
}

uint8_t MetaText::setBlockIndex(meta_t &meta, uint8_t index)
{
    // set control byte lower nibble to block id
    // 0001 = blk1, 0010 = blk2, 0100 = blk3, 1000 = blk4
    meta.text.block_index = (1 << index);
    return meta.text.block_index;
}

uint8_t MetaText::getTotalBlocks(const meta_t &meta)
{
    uint8_t mask = (meta.raw_data[0] & 0xf0) >> 4;
    // mask is 0x1 => 1, 0x3 => 2, 0x7 => 3, 0xF => 4
    return __builtin_popcount(mask);
}

uint8_t MetaText::setTotalBlocks(meta_t &meta, uint8_t totalBlocks)
{
    // set control byte upper nibble for number of text blocks
    // 0001 = 1 blk. 0011 = 2 blks, 0111 = 3 blks, 1111 = 4 blks
    meta.text.total_blocks = (0x0f >> (4 - totalBlocks));
    return meta.text.total_blocks;
}

/**
 * Set what the MetaText text should be
 */
void MetaText::setText(char* srcText) 
{
    if (!srcText) {
        text[0] = '\0';
        return;
    }
    strncpy(text, srcText, META_TEXT_MAX_LEN);
    text[META_TEXT_MAX_LEN] = '\0'; // Always null-terminate
    // Optionally, null-terminate earlier if srcText is shorter
    size_t len = strnlen(srcText, META_TEXT_MAX_LEN);
    text[len] = '\0';
}

meta_t MetaText::getBlock(meta_t &meta, uint8_t index)
{
    meta = {};
    setBlockIndex(meta, index);
    uint8_t totalBlocks = (strlen(text) + META_TEXT_BLOCK_LEN - 1) / META_TEXT_BLOCK_LEN;
    setTotalBlocks(meta, totalBlocks);
    
    // Fill text portion with spaces (raw_data[1] through raw_data[13])
    // Do NOT overwrite raw_data[0] which contains the control byte
    memset(meta.raw_data + 1, ' ', META_TEXT_BLOCK_LEN);

    size_t textLen = strlen(text);
    size_t offset = index * META_TEXT_BLOCK_LEN;
    size_t charsToCopy = (offset < textLen)
        ? std::min(textLen - offset, (size_t)META_TEXT_BLOCK_LEN)
        : 0;

    if (charsToCopy > 0)
        memcpy(meta.raw_data + 1, text + offset, charsToCopy);

    return meta;
}

bool MetaText::addBlock(const meta_t &meta)
{
    uint8_t idx = getBlockIndex(meta);
    if (idx >= META_TEXT_MAX_BLOCKS)
        return false;

    // Set bit for this block and copy data directly to buffer
    blockMask |= (1 << idx);
    memcpy(text + (idx * META_TEXT_BLOCK_LEN), meta.raw_data + 1,
            META_TEXT_BLOCK_LEN);
    return true;
}

const char *MetaText::getText()
{
    // According to M17 spec: OR the control bytes together, and once
    // the most significant and least significant four bits are the same,
    // a complete message has been received.
    if (blockMask == 0) {
        text[0] = '\0';
        return text;
    }

    // Find the length based on complete blocks
    size_t len = 0;
    for (uint8_t i = 0; i < META_TEXT_MAX_BLOCKS; i++) {
        if (blockMask & (1 << i)) {
            len = (i + 1) * META_TEXT_BLOCK_LEN;
        } else {
            // Gap found - only return text up to last complete block
            break;
        }
    }

    // Trim trailing spaces
    while (len > 0 && text[len - 1] == ' ') {
        len--;
    }
    text[len] = '\0';
    return text;
}

void MetaText::getNextBlock(meta_t &meta) {
    uint8_t totalBlocks = (strlen(text) + META_TEXT_BLOCK_LEN - 1) / META_TEXT_BLOCK_LEN;
    // printf("Send meta block %d / %d", totalBlocks, nextBlockIndex);

    // This resulted in "1 / 0";
    getBlock(meta, nextBlockIndex);
    // getBlock(meta, 0);
    nextBlockIndex = (nextBlockIndex + 1) % totalBlocks;
    return;
}

void MetaText::resetBlockPaging() {
    nextBlockIndex = 0;
}