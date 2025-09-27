/***************************************************************************
 *   Copyright (C) 2020 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN,                     *
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

#include "protocols/FM/DTMF.hpp"

namespace DTMF
{
static constexpr DtmfKeyEntry dtmfKeyMap[] = {
    { '1', 697, 1209 }, { '2', 697, 1336 }, { '3', 697, 1477 },
    { 'A', 697, 1633 }, { '4', 770, 1209 }, { '5', 770, 1336 },
    { '6', 770, 1477 }, { 'B', 770, 1633 }, { '7', 852, 1209 },
    { '8', 852, 1336 }, { '9', 852, 1477 }, { 'C', 852, 1633 },
    { '*', 941, 1209 }, { '0', 941, 1336 }, { '#', 941, 1477 },
    { 'D', 941, 1633 }
};

DtmfKeyEntry lookupDtmfFreq(char symbol)
{
    for (const auto &entry : dtmfKeyMap)
        if (entry.symbol == symbol)
            return entry;
    return { '\0', 0, 0 }; // Not found
}

} // namespace DTMF