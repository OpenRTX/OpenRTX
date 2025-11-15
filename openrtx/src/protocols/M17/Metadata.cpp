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

#include "protocols/M17/Metadata.hpp"

using namespace M17;

Metadata *MetadataFactory::create(M17LinkSetupFrame &lsf, void *storage)
{
    streamType_t streamType = lsf.getType();

    M17::MetadataType type =
        M17::MetadataFactory::getTypeFromStream(streamType);
    if (!storage)
        return nullptr;

    switch (type) {
        case MetadataType::EXTENDED_CALLSIGN:
            return new (storage) ExtendedCallsignMetadata(lsf.metadata());
        case MetadataType::TEXT:
            return new (storage) TextBlockMetadata(lsf.metadata());
        default:
            return nullptr;
    }
}

MetadataType MetadataFactory::getTypeFromStream(const streamType_t &streamType)
{
    if (streamType.fields.encType != M17_ENCRYPTION_NONE) {
        return MetadataType::NONE;
    }

    switch (streamType.fields.encSubType) {
        case M17_META_EXTD_CALLSIGN:
            return MetadataType::EXTENDED_CALLSIGN;
        case M17_META_TEXT:
            return MetadataType::TEXT;
        case M17_META_GNSS:
            return MetadataType::GNSS;
        default:
            return MetadataType::NONE;
    }
}
