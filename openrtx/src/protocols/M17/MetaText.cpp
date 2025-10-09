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
#include "protocols/M17/M17Golay.hpp"
#include "protocols/M17/MetaText.hpp"
#include "protocols/M17/M17Interleaver.hpp"
#include "protocols/M17/M17Decorrelator.hpp"
#include "protocols/M17/M17CodePuncturing.hpp"
#include "protocols/M17/M17Constants.hpp"
#include "protocols/M17/M17Utils.hpp"
#include <algorithm>
#include <cstdio>

using namespace M17;

MetaText::MetaText()
{
}

MetaText::~MetaText()
{
}

void MetaText::reset()
{
    blocks = {};
    metaTextBuffer[0] = '\0';
}

bool MetaText::addBlock(const meta_t &meta)
{
    uint8_t blockIndex = getBlockIndex(meta);

    if (blockIndex < MAX_BLOCKS) {
        blocks[blockIndex] = meta;
        return true;
    }
    return false;
}
const char *MetaText::get()
{
    memset(metaTextBuffer, 0, sizeof(metaTextBuffer));

    // Stitch the blocks together
    for (size_t i = 0; i < blocks.size(); i++) {
        const auto &meta = blocks[i];
        if (meta.raw_data[0] == 0)
            break;

        uint8_t totalBlocks = getTotalBlocks(meta);
        // TODO: should we use totalBlocks for validation that we've received all blocks?
        memcpy(metaTextBuffer + (i * BLOCK_LENGTH), meta.raw_data + 1,
               BLOCK_LENGTH);
    }

    // Set the string terminator after the last non-space character
    for (size_t i = MAX_TEXT_LENGTH; i > 0; i--) {
        if (metaTextBuffer[i - 1] != ' ') {
            metaTextBuffer[i] = '\0';
            break;
        }
    }
    return metaTextBuffer;
}
