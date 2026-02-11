/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstring>

extern "C" {
#include "interfaces/cps_io.h"
}

TEST_CASE("CPS initialization and read-back", "[cps]")
{
    int err = cps_create("/tmp/test1.rtxc");
    REQUIRE(err == 0);
    err = cps_open("/tmp/test1.rtxc");
    REQUIRE(err == 0);
}

TEST_CASE("CPS contact insertion and ordering", "[cps]")
{
    cps_create("/tmp/test2.rtxc");
    cps_open("/tmp/test2.rtxc");

    contact_t c1 = { "Test contact 1", 0, { { 0 } } };
    contact_t c2 = { "Test contact 2", 0, { { 0 } } };
    contact_t c3 = { "Test contact 3", 0, { { 0 } } };
    contact_t c4 = { "Test contact 4", 0, { { 0 } } };
    cps_insertContact(c1, 0);
    cps_insertContact(c2, 0);
    cps_insertContact(c3, 0);
    cps_insertContact(c4, 1);

    contact_t c = { 0 };
    cps_readContact(&c, 0);
    REQUIRE(strncmp(c3.name, c.name, 32L) == 0);
    cps_readContact(&c, 1);
    REQUIRE(strncmp(c4.name, c.name, 32L) == 0);
    cps_readContact(&c, 2);
    REQUIRE(strncmp(c2.name, c.name, 32L) == 0);
    cps_readContact(&c, 3);
    REQUIRE(strncmp(c1.name, c.name, 32L) == 0);

    cps_close();
}

TEST_CASE("CPS channel insertion and ordering", "[cps]")
{
    cps_create("/tmp/test3.rtxc");
    cps_open("/tmp/test3.rtxc");

    channel_t c1 = { 0,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 1",
                     "", { 0 }, { { 0 } } };
    channel_t c2 = { 0,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 2",
                     "", { 0 }, { { 0 } } };
    channel_t c3 = { 0,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 3",
                     "", { 0 }, { { 0 } } };
    channel_t c4 = { 0,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 4",
                     "", { 0 }, { { 0 } } };
    cps_insertChannel(c1, 0);
    cps_insertChannel(c2, 0);
    cps_insertChannel(c3, 0);
    cps_insertChannel(c4, 1);

    channel_t c = { 0 };
    cps_readChannel(&c, 0);
    REQUIRE(strncmp(c3.name, c.name, 32L) == 0);
    cps_readChannel(&c, 1);
    REQUIRE(strncmp(c4.name, c.name, 32L) == 0);
    cps_readChannel(&c, 2);
    REQUIRE(strncmp(c2.name, c.name, 32L) == 0);
    cps_readChannel(&c, 3);
    REQUIRE(strncmp(c1.name, c.name, 32L) == 0);

    cps_close();
}

TEST_CASE("CPS contact index fix on insertion", "[cps][!mayfail]")
{
    cps_create("/tmp/test4.rtxc");
    cps_open("/tmp/test4.rtxc");

    contact_t ct1 = { "Test contact 1", 0, { { 0 } } };
    contact_t ct2 = { "Test contact 2", 0, { { 0 } } };
    channel_t ch1 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 1",
                      "", { 0 }, { { 0 } } };
    cps_insertContact(ct1, 0);
    cps_insertChannel(ch1, 0);
    cps_insertContact(ct2, 0);

    channel_t c = { 0 };
    cps_readChannel(&c, 0);
    REQUIRE(c.m17.contact_index == 1);

    cps_close();
}

TEST_CASE("CPS complex codeplug creation", "[cps]")
{
    cps_create("/tmp/test5.rtxc");
    cps_open("/tmp/test5.rtxc");

    contact_t ct1 = { "Test contact 1", 0, { { 0 } } };
    contact_t ct2 = { "Test contact 2", 0, { { 0 } } };
    channel_t ch1 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 1",
                      "", { 0 }, { { 0 } } };
    channel_t ch2 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 2",
                      "", { 0 }, { { 0 } } };
    channel_t ch3 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 3",
                      "", { 0 }, { { 0 } } };
    channel_t ch4 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 4",
                      "", { 0 }, { { 0 } } };
    channel_t ch5 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 5",
                      "", { 0 }, { { 0 } } };
    bankHdr_t b1 = { "Test Bank 1", 0 };
    bankHdr_t b2 = { "Test Bank 2", 0 };
    cps_insertContact(ct2, 0);
    cps_insertContact(ct1, 0);
    cps_insertChannel(ch1, 0);
    cps_insertChannel(ch2, 1);
    cps_insertChannel(ch3, 2);
    cps_insertChannel(ch4, 3);
    cps_insertChannel(ch5, 4);
    cps_insertBankHeader(b2, 0);
    cps_insertBankHeader(b1, 0);
    cps_insertBankData(0, 0, 0);
    cps_insertBankData(1, 0, 1);
    cps_insertBankData(2, 1, 0);
    cps_insertBankData(3, 1, 1);
    cps_insertBankData(4, 1, 2);

    cps_close();
}

TEST_CASE("CPS out-of-order codeplug creation", "[cps]")
{
    cps_create("/tmp/test6.rtxc");
    cps_open("/tmp/test6.rtxc");

    contact_t ct1 = { "Test contact 1", 0, { { 0 } } };
    contact_t ct2 = { "Test contact 2", 0, { { 0 } } };
    channel_t ch1 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 1",
                      "", { 0 }, { { 0 } } };
    channel_t ch2 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 2",
                      "", { 0 }, { { 0 } } };
    channel_t ch3 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 3",
                      "", { 0 }, { { 0 } } };
    channel_t ch4 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 4",
                      "", { 0 }, { { 0 } } };
    channel_t ch5 = { 2,  0,     0,        0, 0, 0, 0, 0, 0, "Test channel 5",
                      "", { 0 }, { { 0 } } };
    bankHdr_t b1 = { "Test Bank 1", 0 };
    bankHdr_t b2 = { "Test Bank 2", 0 };
    cps_insertContact(ct1, 0);
    cps_insertContact(ct2, 1);
    cps_insertBankHeader(b1, 0);
    cps_insertBankHeader(b2, 1);
    cps_insertChannel(ch5, 0);
    cps_insertBankData(0, 1, 0);
    cps_insertChannel(ch4, 0);
    cps_insertBankData(0, 1, 0);
    cps_insertChannel(ch3, 0);
    cps_insertBankData(0, 1, 0);
    cps_insertChannel(ch2, 0);
    cps_insertBankData(0, 0, 0);
    cps_insertChannel(ch1, 0);
    cps_insertBankData(0, 0, 0);

    cps_close();
}
