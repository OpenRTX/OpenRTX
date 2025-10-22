/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                                                         *
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

#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>
#include <algorithm>
#include <iostream>
#include "protocols/M17/Callsign.hpp"
#include "protocols/M17/M17Datatypes.hpp"

using namespace std;

int test_encode_ab1cd()
{
    const char callsign[] = "AB1CD";
    M17::call_t expected = { 0x00, 0x00, 0x00, 0x9f, 0xdd, 0x51 };

    M17::Callsign test = M17::Callsign(callsign);
    M17::call_t actual = test;
    if (equal(begin(actual), end(actual), begin(expected), end(expected))) {
        return 0;
    } else {
        return -1;
    }
}

int test_encode_empty()
{
    const char callsign[] = "";
    M17::call_t expected = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

    M17::Callsign test = M17::Callsign(callsign);
    M17::call_t actual = test;
    if (equal(begin(actual), end(actual), begin(expected), end(expected))) {
        return 0;
    } else {
        return -1;
    }
}

int test_decode_ab1cd()
{
    M17::call_t callsign = { 0x00, 0x00, 0x00, 0x9f, 0xdd, 0x51 };

    M17::Callsign test = M17::Callsign(callsign);
    const char expected[] = "AB1CD";
    const char *actual = test;
    if (strcmp(expected, actual) == 0) {
        return 0;
    } else {
        return -1;
    }
}

int main()
{
    if (test_encode_ab1cd()) {
        printf("Error in encoding callsign ab1cd!\n");
        return -1;
    }
    if (test_encode_empty()) {
        printf("Error in encoding empty callsign !\n");
        return -1;
    }
    if (test_decode_ab1cd()) {
        printf("Error in decoding callsign ab1cd!\n");
        return -1;
    }
    return 0;
}
