/***************************************************************************
 *   Copyright (C) 2022 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef METATEXT_H
#define METATEXT_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <string>
#include <array>
#include "M17LinkSetupFrame.hpp"
#include "M17Viterbi.hpp"
#include "M17StreamFrame.hpp"

namespace M17
{

/**
 * M17 frame decoder.
 */
class MetaText
{
public:
    /**
     * Constructor.
     */
    MetaText();

    /**
     * Constructor.
     */
    MetaText(const char *text);

    /**
     * Destructor.
     */
    ~MetaText();

    /**
     * Add a new block.
     */
    bool addBlock(const meta_t &meta);

    /**
     * Get a specific block. If the block number is out of range, return
     * the first block.
     */
    const meta_t &getBlock(const uint8_t num) const
    {
        if (num >= MAX_BLOCKS)
            return blocks[0];
        return blocks[num];
    };

    /**
     * Return the assembled meta text, or an empty string if not all blocks
     * have been received yet.
     */
    const char *get();

    void reset();

private:
    /**
     * Convert the M17 block index field to a normal integer
     */
    inline uint8_t getBlockIndex(const meta_t &meta)
    {
        uint8_t blockIndexMask = (meta.raw_data[0] & 0x0f);
        if (blockIndexMask == 0x1)
            return 0;
        else if (blockIndexMask == 0x2)
            return 1;
        else if (blockIndexMask == 0x4)
            return 2;
        else if (blockIndexMask == 0x8)
            return 3;
        else
            return 0;
    }

    /**
     * Convert the M17 total blocks field to a normal integer
     */
    inline uint8_t getTotalBlocks(const meta_t &meta)
    {
        uint8_t requiredBlocksMask = (meta.raw_data[0] & 0xf0) >> 4;
        if (requiredBlocksMask == 0x1)
            return 1;
        else if (requiredBlocksMask == 0x3)
            return 2;
        else if (requiredBlocksMask == 0x7)
            return 3;
        else if (requiredBlocksMask == 0xF)
            return 4;
        else
            return 0;
    }

    constexpr static uint8_t MAX_BLOCKS = 4;
    constexpr static uint8_t BLOCK_LENGTH = 13;
    constexpr static uint8_t MAX_TEXT_LENGTH = MAX_BLOCKS * BLOCK_LENGTH;
    std::array<meta_t, MAX_BLOCKS> blocks;
    char metaTextBuffer[MAX_TEXT_LENGTH + 1];
};

} // namespace M17

#endif // METATEXT_H
