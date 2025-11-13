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
    reset();
}

MetaText::~MetaText()
{
}

void MetaText::reset()
{
    memset(text, 0, sizeof(text));
    blockMask = 0;
    expectedBlockMask = 0;
    resetBlockPaging();
}

uint8_t MetaText::getBlockIndex(const meta_t &meta)
{
    uint8_t mask = meta.raw_data[0] & 0x0f;
    return mask ? __builtin_ctz(mask) : 0;
}

uint8_t MetaText::getTotalBlocks(const meta_t &meta)
{
    uint8_t mask = (meta.raw_data[0] & 0xf0) >> 4;
    // mask is 0x1 => 1, 0x3 => 2, 0x7 => 3, 0xF => 4
    return __builtin_popcount(mask);
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
    memset(text, 0, META_TEXT_MAX_LEN);
    strncpy(text, srcText, META_TEXT_MAX_LEN);
}

void MetaText::getBlock(meta_t &meta, uint8_t index)
{
    memset(&meta, 0, sizeof(meta));

    size_t textLen = strlen(text);
    uint8_t totalBlocks = (textLen + META_TEXT_BLOCK_LEN - 1) / META_TEXT_BLOCK_LEN;

    // Set control byte: lower nibble = block index, upper nibble = total blocks mask
    uint8_t lowerNibble = (1 << index);
    uint8_t upperNibble = (0x0f >> (4 - totalBlocks)) << 4;
    meta.raw_data[0] = upperNibble | lowerNibble;

    // Fill text portion with spaces (raw_data[1] through raw_data[13])
    memset(meta.raw_data + 1, ' ', META_TEXT_BLOCK_LEN);

    size_t offset = index * META_TEXT_BLOCK_LEN;
    size_t charsToCopy = (offset < textLen)
        ? std::min(textLen - offset, (size_t)META_TEXT_BLOCK_LEN)
        : 0;

    if (charsToCopy > 0)
        memcpy(meta.raw_data + 1, text + offset, charsToCopy);
}

bool MetaText::addBlock(const meta_t &meta)
{
    uint8_t idx = getBlockIndex(meta);
    if (idx >= META_TEXT_MAX_BLOCKS)
        return false;

    // Store expected block mask from upper nibble of control byte
    uint8_t totalBlocks = getTotalBlocks(meta);
    expectedBlockMask = (1 << totalBlocks) - 1;

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
    if (blockMask == 0 || blockMask != expectedBlockMask) {
        return nullptr;
    }

    // All expected blocks received, compute total length
    size_t len = __builtin_popcount(blockMask) * META_TEXT_BLOCK_LEN;

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