/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

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
