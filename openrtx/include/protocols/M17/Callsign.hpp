/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
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

#ifndef M17_CALLSIGN_H
#define M17_CALLSIGN_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include "M17Datatypes.hpp"

namespace M17
{

/**
     * An M17 Callsign class
     * 
     * This class implements the M17 protocol spec for callsigns. It is responsible for encoding and decoding of callsigns. It also has the definitions for invalid and broadcast callsigns.
     */
class Callsign {
public:
    /**
     * Construct a Callsign class from an std::string containing the callsign text
     * 
     * @param callsign the ascii calsign (max 9 chars)
     */
    Callsign(std::string callsign);

    /**
     * Construct a Callsign class from a C string containing the callsign text
     * 
     * @param callsign NUL-terminated ascii calsign (max 9 chars)
     */
    Callsign(const char *callsign);

    /**
     * Construct a Callsign class from an encoded call_t base-40 callsign
     * 
     * @param encodedCall encoded callsign value
     */
    Callsign(const call_t encodedCall);

    /**
     * Answer if a callsign is empty
     * 
     * @return true if the first char of the callsign is a NUL
     */
    inline bool empty() const
    {
        return call[0] == '\0';
    }

    /**
     * Answer if two callsigns are equivalent; left side of equivalency should be local callsign and right side should be incoming in order for special broadcast, info, and echo callsigns to be considered equivalent.
     * This handles stripping slashes as well.
     * 
     * @param other the incoming callsign to compare against
     * @return true if callsigns are equivalent (or)
     */
    bool operator==(const Callsign &other) const;

    /**
     * Convenience operator for getting the callsign in encoded format
     * @return the base-40 encoded version of the callsign
     */
    operator call_t() const;

    /**
     * Convenience operator for getting the callsign as a std::string
     * @return the callsign in a std::string format
     */
    operator std::string() const;

    /**
     * Convenience operator for getting the callsign as a cstring
     * @return the callsign in a cstring format
     */
    operator const char *() const;

private:
    static constexpr size_t CALLSIGN_MAX_CHARS = 9;

    /**
    * Encode a callsign in base-40 format, starting with the right-most character.
    * The final value is wr(this->callitten out in "big-endian" form, with the most-significant
    * value first, leading to 0-padding of callsigns shorter than nine characters.
    *
    * @param callsign the callsign to encode.
    * @param encodedCall call_t data structure where to put the encoded data.
    * @param strict a flag (disabled by default) which indicates whether invalid
    * characters are allowed and assigned a value of 0 or not allowed, making the
    * function return an error.
    */
    void encode(const char *callsign, call_t &encodedCall,
                bool strict = false) const;

    /**
    * Decode a base-40 encoded callsign to its text representation. This decodes
    * a 6-byte big-endian value into a string of up to 9 characters.
    *
    * @param encodedCall base-40 encoded callsign.
    * @param out buffer for outputting the decoded call
    */
    void decode(const call_t &encodedCall, char *out) const;

    char call[M17::Callsign::CALLSIGN_MAX_CHARS + 1];
};
/**
 * Get the broadcast callsign "ALL".
 * 
 * @return Broadcast callsign
 */
Callsign &getBroadcastCallsign();
/**
 * Get the invalid callsign 0. This is useful internally to represent undefined, but is forbidden from being sent by the M17 specification.
 * 
 * @return Invalid callsign
 */
Callsign &getInvalidCallsign();
} // namespace M17

#endif // M17_CALLSIGN_H
