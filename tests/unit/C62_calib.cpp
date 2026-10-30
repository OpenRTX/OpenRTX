/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <vector>
#include "calibration/calib_C62.h"

using namespace std;

namespace
{

/**
 * The ten points of the stock default table, in the order the stock firmware
 * stores them.
 */
const vector<C62CalPoint> stockPoints = {
    { 400125000, 33, 75 }, { 420125000, 32, 74 }, { 440125000, 31, 74 },
    { 460125000, 30, 76 }, { 479975000, 31, 75 }, { 136125000, 33, 67 },
    { 146125000, 30, 57 }, { 156125000, 27, 64 }, { 166125000, 33, 73 },
    { 173975000, 40, 74 },
};

/**
 * Write a journal entry with the given calibration points.
 *
 * @param journal: journal image.
 * @param off: offset of the entry.
 * @param len: total entry length, 0x0BDC or 0x0D48.
 * @param points: calibration points, one per record from slot zero.
 * @param badSum: write a wrong payload sum.
 * @return offset right after the entry.
 */
uint32_t writeEntry(vector<uint8_t> &journal, uint32_t off, uint16_t len,
                    const vector<C62CalPoint> &points, bool badSum = false)
{
    memset(&journal[off], 0, len);

    for (size_t i = 0; i < points.size(); i++)
        memcpy(&journal[off + 0x450 + (i * 0x24)], &points[i],
               sizeof(points[i]));

    uint8_t sum = 0;
    for (uint32_t i = off + 4; i < off + len; i++)
        sum += journal[i];

    journal[off] = 0x5A;
    journal[off + 1] = badSum ? sum + 1 : sum;
    memcpy(&journal[off + 2], &len, sizeof(len));

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
    vector<C62CalPoint> older = stockPoints;
    older[6].loPwr = 11;

    uint32_t off = writeEntry(journal, 0, 0x0BDC, older);
    writeEntry(journal, off, 0x0BDC, stockPoints);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.count == 5);
    REQUIRE(cal.uhf.count == 5);

    // Sorted by frequency within each band
    REQUIRE(cal.vhf.points[0].freq == 136125000);
    REQUIRE(cal.vhf.points[4].freq == 173975000);
    REQUIRE(cal.uhf.points[0].freq == 400125000);

    // From the newer entry
    REQUIRE(cal.vhf.points[1].loPwr == 30);
    REQUIRE(cal.vhf.points[1].hiPwr == 57);
}

TEST_CASE("C62 calibration skips an entry with a bad checksum",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<C62CalPoint> newer = stockPoints;
    newer[6].loPwr = 11;

    uint32_t off = writeEntry(journal, 0, 0x0BDC, stockPoints);
    writeEntry(journal, off, 0x0BDC, newer, true);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.points[1].loPwr == 30);
}

TEST_CASE("C62 calibration from a 20-slot entry", "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    writeEntry(journal, 0, 0x0D48, stockPoints);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.count == 5);
    REQUIRE(cal.uhf.count == 5);
    REQUIRE(cal.uhf.points[3].hiPwr == 76);
}

TEST_CASE("C62 calibration skips records outside the bands or above 100%",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<C62CalPoint> points = stockPoints;
    points.push_back({ 300000000, 30, 60 });
    points[0].hiPwr = 150;

    writeEntry(journal, 0, 0x0D48, points);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == 0);
    REQUIRE(cal.vhf.count == 5);
    REQUIRE(cal.uhf.count == 4);
    REQUIRE(cal.uhf.points[0].freq == 420125000);
}

TEST_CASE("C62 calibration fails on an erased journal", "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == -ENOENT);
}

TEST_CASE("C62 calibration fails without points in both bands",
          "[c62][calibration]")
{
    vector<uint8_t> journal(0x10000, 0xFF);
    vector<C62CalPoint> vhfOnly(stockPoints.begin() + 5, stockPoints.end());
    writeEntry(journal, 0, 0x0BDC, vhfOnly);

    C62Calib cal;
    REQUIRE(parse(journal, cal) == -ENOENT);
}

TEST_CASE("C62 default calibration", "[c62][calibration]")
{
    C62Calib cal;
    calibC62_defaults(&cal);

    REQUIRE(cal.vhf.count == 5);
    REQUIRE(cal.uhf.count == 5);
    REQUIRE(cal.vhf.points[0].freq == 136125000);
    REQUIRE(cal.vhf.points[0].loPwr == 33);
    REQUIRE(cal.vhf.points[0].hiPwr == 67);
    REQUIRE(cal.uhf.points[4].freq == 479975000);
    REQUIRE(cal.uhf.points[4].loPwr == 31);
    REQUIRE(cal.uhf.points[4].hiPwr == 75);
}

TEST_CASE("C62 calibration interpolated between points", "[c62][calibration]")
{
    C62Calib cal;
    calibC62_defaults(&cal);

    // 440.125 MHz 31/74 and 460.125 MHz 30/76: rounded toward the lower point
    C62CalPoint uhf = calibC62_interp(&cal.uhf, 450125000);
    REQUIRE(uhf.freq == 450125000);
    REQUIRE(uhf.loPwr == 31);
    REQUIRE(uhf.hiPwr == 75);

    // 136.125 MHz 33/67 and 146.125 MHz 30/57
    C62CalPoint vhf = calibC62_interp(&cal.vhf, 141125000);
    REQUIRE(vhf.loPwr == 32);
    REQUIRE(vhf.hiPwr == 62);
}

TEST_CASE("C62 calibration at and beyond the calibration points",
          "[c62][calibration]")
{
    C62Calib cal;
    calibC62_defaults(&cal);

    C62CalPoint at = calibC62_interp(&cal.uhf, 440125000);
    REQUIRE(at.loPwr == 31);
    REQUIRE(at.hiPwr == 74);

    C62CalPoint below = calibC62_interp(&cal.uhf, 400000000);
    REQUIRE(below.freq == 400000000);
    REQUIRE(below.loPwr == 33);
    REQUIRE(below.hiPwr == 75);

    C62CalPoint above = calibC62_interp(&cal.uhf, 480000000);
    REQUIRE(above.loPwr == 31);
    REQUIRE(above.hiPwr == 75);
}
