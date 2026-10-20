/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "drivers/NVM/calib_C62.h"
#include <string.h>

/*
 * Settings journal of the stock firmware: entries start with a marker, the
 * 8-bit sum of the payload and the little-endian total length, header
 * included. C2 entries hold 10 calibration records, C62 entries 20.
 */
#define ENTRY_MARKER 0x5A
#define ENTRY_HDR_SIZE 4
#define ENTRY_LEN_10 0x0BDC
#define ENTRY_LEN_20 0x0D48

/*
 * Calibration records start at this offset from the beginning of an entry.
 * Each one holds the frequency in Hz (LE32) followed by four duty cycles in
 * percent (LE16); the first two are the low and high power settings.
 */
#define RECORD_OFFSET 0x450
#define RECORD_SIZE 0x24

/**
 * Stock default calibration, as the C62 firmware ships it.
 */
static const struct {
    freq_t freq;
    uint8_t low;
    uint8_t high;
} defaultCalib[] = {
    { 136125000, 33, 67 }, { 146125000, 30, 57 }, { 156125000, 27, 64 },
    { 166125000, 33, 73 }, { 173975000, 40, 74 }, { 400125000, 33, 75 },
    { 420125000, 32, 74 }, { 440125000, 31, 74 }, { 460125000, 30, 76 },
    { 479975000, 31, 75 },
};

/**
 * Read a little-endian 16-bit value.
 *
 * @param p: pointer to the value.
 * @return the value.
 */
static uint16_t read16(const uint8_t *p)
{
    return (uint16_t)(p[0] | (p[1] << 8));
}

/**
 * Read a little-endian 32-bit value.
 *
 * @param p: pointer to the value.
 * @return the value.
 */
static uint32_t read32(const uint8_t *p)
{
    return (uint32_t)p[0] | ((uint32_t)p[1] << 8) | ((uint32_t)p[2] << 16)
         | ((uint32_t)p[3] << 24);
}

/**
 * Check the payload sum of a journal entry.
 *
 * @param read: journal read function.
 * @param ctx: context passed to read.
 * @param off: offset of the entry.
 * @param len: total length of the entry.
 * @param sum: payload sum from the entry header.
 * @return true if the entry can be read and its payload sum matches.
 */
static bool entryValid(int (*read)(void *, uint32_t, void *, size_t), void *ctx,
                       uint32_t off, uint16_t len, uint8_t sum)
{
    uint8_t buf[64];
    uint8_t acc = 0;
    uint32_t pos = off + ENTRY_HDR_SIZE;
    uint32_t end = off + len;

    while (pos < end) {
        size_t chunk = end - pos;
        if (chunk > sizeof(buf))
            chunk = sizeof(buf);

        if (read(ctx, pos, buf, chunk) != 0)
            return false;

        for (size_t i = 0; i < chunk; i++)
            acc += buf[i];

        pos += chunk;
    }

    return acc == sum;
}

/**
 * Select the band a calibration frequency belongs to.
 *
 * @param cal: calibration data.
 * @param freq: frequency, in Hz.
 * @return the band, or NULL if the frequency is outside both bands.
 */
static struct C62BandCalib *bandOf(struct C62Calib *cal, freq_t freq)
{
    if ((freq >= 136000000) && (freq <= 174000000))
        return &cal->vhf;

    if ((freq >= 400000000) && (freq <= 480000000))
        return &cal->uhf;

    return NULL;
}

/**
 * Insert a calibration point, keeping the band sorted by frequency. Points
 * beyond the band capacity are dropped.
 *
 * @param band: band calibration.
 * @param freq: frequency, in Hz.
 * @param low: duty cycle of the low power setting, in percent.
 * @param high: duty cycle of the high power setting, in percent.
 */
static void addPoint(struct C62BandCalib *band, freq_t freq, uint8_t low,
                     uint8_t high)
{
    if (band->points >= C62_CAL_POINTS)
        return;

    uint8_t i = band->points;
    while ((i > 0) && (band->freq[i - 1] > freq)) {
        band->freq[i] = band->freq[i - 1];
        band->lowPwr[i] = band->lowPwr[i - 1];
        band->highPwr[i] = band->highPwr[i - 1];
        i--;
    }

    band->freq[i] = freq;
    band->lowPwr[i] = low;
    band->highPwr[i] = high;
    band->points++;
}

int calibC62_read(int (*read)(void *ctx, uint32_t off, void *buf, size_t len),
                  void *ctx, uint32_t size, struct C62Calib *cal)
{
    uint8_t hdr[ENTRY_HDR_SIZE];
    uint32_t off = 0;
    uint32_t newest = 0;
    uint16_t newestLen = 0;

    // Walk the journal up to the first erased or malformed header
    while (off + ENTRY_HDR_SIZE <= size) {
        if (read(ctx, off, hdr, sizeof(hdr)) != 0)
            break;

        uint16_t len = read16(&hdr[2]);
        if ((hdr[0] != ENTRY_MARKER)
            || ((len != ENTRY_LEN_10) && (len != ENTRY_LEN_20))
            || (off + len > size))
            break;

        if (entryValid(read, ctx, off, len, hdr[1])) {
            newest = off;
            newestLen = len;
        }

        off += len;
    }

    if (newestLen == 0)
        return -1;

    memset(cal, 0, sizeof(*cal));

    uint8_t records = (newestLen == ENTRY_LEN_10) ? 10 : 20;
    for (uint8_t i = 0; i < records; i++) {
        uint8_t rec[RECORD_SIZE];
        uint32_t pos = newest + RECORD_OFFSET + (i * RECORD_SIZE);

        if (read(ctx, pos, rec, sizeof(rec)) != 0)
            return -1;

        freq_t freq = read32(&rec[0]);
        uint16_t low = read16(&rec[4]);
        uint16_t high = read16(&rec[6]);
        struct C62BandCalib *band = bandOf(cal, freq);

        if ((band == NULL) || (low > 100) || (high > 100))
            continue;

        addPoint(band, freq, (uint8_t)low, (uint8_t)high);
    }

    if ((cal->vhf.points == 0) || (cal->uhf.points == 0))
        return -1;

    cal->fromFlash = true;
    return 0;
}

void calibC62_defaults(struct C62Calib *cal)
{
    memset(cal, 0, sizeof(*cal));

    for (size_t i = 0; i < sizeof(defaultCalib) / sizeof(defaultCalib[0]);
         i++) {
        addPoint(bandOf(cal, defaultCalib[i].freq), defaultCalib[i].freq,
                 defaultCalib[i].low, defaultCalib[i].high);
    }
}
