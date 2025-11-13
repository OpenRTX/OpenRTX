/***************************************************************************
 *   Copyright (C)        2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef M17_METADATA_H
#define M17_METADATA_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <cstdint>
#include <cstddef>
#include "M17Datatypes.hpp"
#include "rtx/rtx.h"
#include "M17LinkSetupFrame.hpp"
#include "M17Callsign.hpp"
#include <cstring>

namespace M17
{
enum class MetadataType : uint8_t {
    NONE = 0,
    EXTENDED_CALLSIGN = M17_META_EXTD_CALLSIGN,
    TEXT = M17_META_TEXT,
    GNSS = M17_META_GNSS
};

/**
 * @brief A base class for the common concepts of metadata within single frames.
 */
class Metadata
{
public:
    Metadata(MetadataType type, const meta_t &meta)
        : type(type)
        , meta(meta)
    {
    }
    virtual ~Metadata() = default;

    MetadataType getType() const
    {
        return type;
    }
    const meta_t &getMeta() const
    {
        return meta;
    }

protected:
    MetadataType type;
    meta_t meta;
};

/**
 * @brief A class representing the text metadata contained within a single LSF.
 */
class TextBlockMetadata : public Metadata
{
public:
    TextBlockMetadata(const meta_t &rawMeta)
        : Metadata(MetadataType::TEXT, rawMeta)
    {
    }
};

/**
 * @brief A class representing the extended callsign metadata.
 */
class ExtendedCallsignMetadata : public Metadata
{
public:
    ExtendedCallsignMetadata(const meta_t &rawMeta)
        : Metadata(MetadataType::EXTENDED_CALLSIGN, rawMeta)
    {
    }

    std::string getCall1() const
    {
        return decode_callsign(meta.extended_call_sign.call1);
    }

    std::string getCall2() const
    {
        return decode_callsign(meta.extended_call_sign.call2);
    }
};

/**
 * @brief Handle the determination and creation of metadata, as well as facilitate the storage of metadata without needing to use the heap.
 */
class MetadataFactory
{
public:
    /**
     * @brief From a given M17 frame's LSF, parse the metadata portion and store the frame's metadata in the appropriate class
     * @param lsf the LSF to process
     * @param storage a pointer to the memory location to store the created class; use getMaxStorageSize() and getStorageAlignment() to determine the appriate storage to allocate
     * @returns a pointer to the created metadata; nullptr if creation failed or metadata type is not supported
     */
    static Metadata *create(M17LinkSetupFrame &lsf, void *storage);

    /**
     * @brief From a given M17 frame LSF Stream Type, determine the appropriate metadata type contained
     * @param streamType the streamType to process
     * @returns the metadata type (or NONE if unsupported or encrypted)
     */
    static MetadataType getTypeFromStream(const streamType_t &streamType);

    /**
     * @brief Returns the max storage size needed for the Metadata superclass
     * @return the size required in bytes for the largest possible Metadata object
     */
    static constexpr size_t getMaxStorageSize()
    {
        return std::max(sizeof(ExtendedCallsignMetadata),
                        sizeof(TextBlockMetadata));
    }
};

}
#endif // M17_METADATA_H