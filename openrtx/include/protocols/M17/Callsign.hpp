/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef CALLSIGN_H
#define CALLSIGN_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include "M17Datatypes.hpp"

namespace M17
{

/**
 * Class representing an M17 callsign object.
 */
class Callsign
{
public:
    /**
     * Default constructor.
     * By default, an uninitialized callsign is set to invalid.
     */
    Callsign();

    /**
     * Construct a callsign object from an std::string
     * The callsign can have up to 9 characters
     *
     * @param callsign: callsign string
     */
    Callsign(const std::string callsign);

    /**
     * Construct a callsign object from a NULL-terminated string
     * The callsign can have up to 9 characters
     *
     * @param callsign: callsign string
     */
    Callsign(const char *callsign);

    /**
     * Construct a callsign object from a base-40 encoded callsign
     *
     * @param encodedCall: encoded callsign value
     */
    Callsign(const call_t &encodedCall);

    /**
     * Test if callsign is empty.
     * A callsign is considered empty when its first character is NULL.
     *
     * @return true if the callsign is empty
     */
    inline bool isEmpty() const
    {
        return call[0] == '\0';
    }

    /**
     * Test if callsign is a special one.
     *
     * @return true if the callsign is either ALL, INFO or ECHO
     */
    bool isSpecial() const;

    /**
     * Type-conversion operator to retrieve the callsign in encoded format
     *
     * @return the base-40 encoded version of the callsign
     */
    operator call_t() const;

    /**
     * Type-conversion operator to retrieve the callsign as a std::string
     *
     * @return a std::string containing the callsign
     */
    operator std::string() const;

    /**
     * Type-conversion operator to retrieve the callsign as a NULL-terminated
     * string
     *
     * @return the callsign as a NULL-terminated string
     */
    operator const char *() const;

    /**
     * Comparison operator.
     *
     * @param other the incoming callsign to compare against
     * @return true if callsigns are equivalent
     */
    bool operator==(const Callsign &other) const;

private:
    static constexpr size_t MAX_CALLSIGN_CHARS = 9;
    static constexpr size_t MAX_CALLSIGN_LEN = MAX_CALLSIGN_CHARS + 1;
    char call[MAX_CALLSIGN_LEN];
};

} // namespace M17

#endif // CALLSIGN_H
