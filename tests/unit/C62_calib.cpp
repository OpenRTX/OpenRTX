/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cstdint>
#include <cstring>
#include <vector>
#include "drivers/NVM/calib_C62.h"

using namespace std;

namespace
{

/**
 * One calibration record: frequency and the low/high power duty cycles.
 */
struct Record {
    uint32_t freq;
    uint16_t low;
    uint16_t high;
};

/**
 * The ten records of the stock default table, in the order the stock
 * firmware stores them.
 */
const vector<Record> stockRecords = {
    { 400125000, 33, 75 }, { 420125000, 32, 74 }, { 440125000, 31, 74 },
    { 460125000, 30, 76 }, { 479975000, 31, 75 }, { 136125000, 33, 67 },
    { 146125000, 30, 57 }, { 156125000, 27, 64 }, { 166125000, 33, 73 },
    { 173975000, 40, 74 },
};

/**
 * Write a journal entry with the given records.
 *
 * @param journal: journal image.
 * @param off: offset of the entry.
 * @param len: total entry length, 0x0BDC or 0x0D48.
 * @param records: calibration records, from slot zero.
 * @param badSum: write a wrong payload sum.
 * @return offset right after the entry.
 */
uint32_t writeEntry(vector<uint8_t> &journal, uint32_t off, uint16_t len,
                    const vector<Record> &records, bool badSum = false)
{
    memset(&journal[off], 0, len);

    for (size_t i = 0; i < records.size(); i++) {
        uint8_t *rec = &journal[off + 0x450 + (i * 0x24)];
        for (int b = 0; b < 4; b++)
            rec[b] = (records[i].freq >> (8 * b)) & 0xFF;
        rec[4] = records[i].low & 0xFF;
        rec[5] = records[i].low >> 8;
        rec[6] = records[i].high & 0xFF;
        rec[7] = records[i].high >> 8;
    }

    uint8_t sum = 0;
    for (uint32_t i = off + 4; i < off + len; i++)
        sum += journal[i];

    journal[off] = 0x5A;
    journal[off + 1] = badSum ? sum + 1 : sum;
    journal[off + 2] = len & 0xFF;
    journal[off + 3] = len >> 8;

    return off + len;
}

/**
 * Journal read function over an in-memory image.
 *
 * @param ctx: pointer to the journal image.
 * @param off: offset to read.
 * @param buf: destination buffer.
 * @param len: number of bytes to read.
 * @return zero on success, -1 when reading past the end.
 */
int readJournal(void *ctx, uint32_t off, void *buf, size_t len)
{
    auto *journal = static_cast<vector<uint8_t> *>(ctx);
    if (off + len > journal->size())
        return -1;

    memcpy(buf, journal->data() + off, len);
    return 0;
}

/**
 * Parse a journal image.
 *
 * @param journal: journal image.
 * @param cal: destination for the calibration data.
 * @return value returned by calibC62_read().
 */
int parse(vector<uint8_t> &journal, C62Calib &cal)
{
    return calibC62_read(readJournal, &journal, journal.size(), &cal);
}

} // namespace

TEST_CASE("C62 calibration from the newest valid journal entry",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<Record> older = stockRecords;
    older[6].low = 11;

    uint32_t off = writeEntry(journal, 0, 0x0BDC, older);
    writeEntry(journal, off, 0x0BDC, stockRecords);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.fromFlash);
    REQUIRE(cal.vhf.points == 5);
    REQUIRE(cal.uhf.points == 5);

    // Sorted by frequency within each band
    REQUIRE(cal.vhf.freq[0] == 136125000);
    REQUIRE(cal.vhf.freq[4] == 173975000);
    REQUIRE(cal.uhf.freq[0] == 400125000);

    // From the newer entry
    REQUIRE(cal.vhf.lowPwr[1] == 30);
    REQUIRE(cal.vhf.highPwr[1] == 57);
}

TEST_CASE("C62 calibration skips an entry with a bad checksum",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<Record> newer = stockRecords;
    newer[6].low = 11;

    uint32_t off = writeEntry(journal, 0, 0x0BDC, stockRecords);
    writeEntry(journal, off, 0x0BDC, newer, true);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.lowPwr[1] == 30);
}

TEST_CASE("C62 calibration from a 20-slot entry", "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    writeEntry(journal, 0, 0x0D48, stockRecords);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.points == 5);
    REQUIRE(cal.uhf.points == 5);
    REQUIRE(cal.uhf.highPwr[3] == 76);
}

TEST_CASE("C62 calibration skips records outside the bands or above 100%",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<Record> records = stockRecords;
    records.push_back({ 300000000, 30, 60 });
    records[0].high = 150;

    writeEntry(journal, 0, 0x0D48, records);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.points == 5);
    REQUIRE(cal.uhf.points == 4);
    REQUIRE(cal.uhf.freq[0] == 420125000);
}

TEST_CASE("C62 calibration fails on an erased journal", "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == -1);
}

TEST_CASE("C62 calibration fails without points in both bands",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<Record> vhfOnly(stockRecords.begin() + 5, stockRecords.end());
    writeEntry(journal, 0, 0x0BDC, vhfOnly);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == -1);
}

TEST_CASE("C62 default calibration", "[c62][calibration]")
{
    C62Calib cal;
    calibC62_defaults(&cal);

    REQUIRE_FALSE(cal.fromFlash);
    REQUIRE(cal.vhf.points == 5);
    REQUIRE(cal.uhf.points == 5);
    REQUIRE(cal.vhf.freq[0] == 136125000);
    REQUIRE(cal.vhf.lowPwr[0] == 33);
    REQUIRE(cal.vhf.highPwr[0] == 67);
    REQUIRE(cal.uhf.freq[4] == 479975000);
    REQUIRE(cal.uhf.lowPwr[4] == 31);
    REQUIRE(cal.uhf.highPwr[4] == 75);
}
