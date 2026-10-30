/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "calibration/calib_C62.h"
#include "core/utils.h"
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

/*
 * The settings journal of the stock firmware is little-endian, like the C62
 * and the hosts running the unit tests, so its structures are read directly.
 */

#define ENTRY_MARKER 0x5A
#define ENTRY_LEN_10 3036
#define ENTRY_LEN_20 3400
#define RECORD_OFFSET 1104
#define MAX_TX_DEVIATION 4095
#define DEFAULT_TX_DEV_NARROW 1250
#define DEFAULT_TX_DEV_WIDE 1240

/**
 * Header of a journal entry: a marker, the 8-bit sum of the rest of the entry
 * and the total entry length, header included. C2 entries hold 10 calibration
 * records, C62 entries 20.
 */
struct C62EntryHdr {
    uint8_t marker;
    uint8_t sum;
    uint16_t len;
};

/**
 * Calibration record of a journal entry: the frequency, the duty cycles of the
 * low and high power settings and of two more power selectors, two TX and RX
 * levels, the TX deviations for 12.5 and 25 kHz channels and two squelch
 * thresholds. Only the frequency, power and deviations are used here.
 */
struct C62CalRecord {
    uint32_t freq;
    uint16_t loPwr;
    uint16_t hiPwr;
    uint16_t otherPwr[2];
    uint32_t levels[2];
    uint32_t txDevNarrow;
    uint32_t txDevWide;
    uint32_t squelch[2];
};

_Static_assert(sizeof(struct C62EntryHdr) == 4, "C62 entry header size");
_Static_assert(sizeof(struct C62CalRecord) == 36, "C62 record size");
_Static_assert(offsetof(struct C62CalRecord, txDevNarrow) == 20,
               "C62 record 12.5 kHz deviation offset");
_Static_assert(offsetof(struct C62CalRecord, txDevWide) == 24,
               "C62 record 25 kHz deviation offset");

/**
 * Stock default calibration, as the C62 firmware ships it. Its TX deviations
 * are the same at every point.
 */
static const struct C62CalPoint defaultCalib[] = {
    { .freq = 136125000, .loPwr = 33, .hiPwr = 67 },
    { .freq = 146125000, .loPwr = 30, .hiPwr = 57 },
    { .freq = 156125000, .loPwr = 27, .hiPwr = 64 },
    { .freq = 166125000, .loPwr = 33, .hiPwr = 73 },
    { .freq = 173975000, .loPwr = 40, .hiPwr = 74 },
    { .freq = 400125000, .loPwr = 33, .hiPwr = 75 },
    { .freq = 420125000, .loPwr = 32, .hiPwr = 74 },
    { .freq = 440125000, .loPwr = 31, .hiPwr = 74 },
    { .freq = 460125000, .loPwr = 30, .hiPwr = 76 },
    { .freq = 479975000, .loPwr = 31, .hiPwr = 75 },
};

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
 * @param point: calibration point.
 */
static void addPoint(struct C62BandCalib *band, const struct C62CalPoint *point)
{
    if (band->count >= C62_CAL_POINTS)
        return;

    uint8_t i = band->count;
    while ((i > 0) && (band->points[i - 1].freq > point->freq)) {
        band->points[i] = band->points[i - 1];
        i--;
    }

    band->points[i] = *point;
    band->count++;
}

/**
 * Limit a TX deviation to the 12-bit REG_40 field, as the stock firmware does.
 *
 * @param dev: TX deviation from a calibration record.
 * @return TX deviation, at most MAX_TX_DEVIATION.
 */
static uint16_t limitDeviation(uint32_t dev)
{
    return (dev > MAX_TX_DEVIATION) ? MAX_TX_DEVIATION : (uint16_t)dev;
}

int calibC62_read(int (*read)(void *ctx, uint32_t off, void *buf, size_t len),
                  void *ctx, uint32_t size, struct C62Calib *cal)
{
    struct C62EntryHdr hdr;
    uint32_t off = 0;
    bool found = false;

    uint8_t *entry = malloc(ENTRY_LEN_20);
    if (entry == NULL)
        return -ENOMEM;

    // Walk the journal up to the first erased or malformed header, parsing
    // each entry with a valid checksum: the newest one is kept
    while (off + sizeof(hdr) <= size) {
        if (read(ctx, off, &hdr, sizeof(hdr)) != 0)
            break;

        if ((hdr.marker != ENTRY_MARKER)
            || ((hdr.len != ENTRY_LEN_10) && (hdr.len != ENTRY_LEN_20))
            || (off + hdr.len > size))
            break;

        if (read(ctx, off, entry, hdr.len) != 0)
            break;

        off += hdr.len;

        uint8_t sum = 0;
        for (uint16_t i = sizeof(hdr); i < hdr.len; i++)
            sum += entry[i];

        if (sum != hdr.sum)
            continue;

        const struct C62CalRecord *rec =
            (const struct C62CalRecord *)&entry[RECORD_OFFSET];
        uint8_t records = (hdr.len == ENTRY_LEN_10) ? 10 : 20;

        memset(cal, 0, sizeof(*cal));
        for (uint8_t i = 0; i < records; i++) {
            struct C62BandCalib *band = bandOf(cal, rec[i].freq);
            if ((band == NULL) || (rec[i].loPwr > 100) || (rec[i].hiPwr > 100))
                continue;

            struct C62CalPoint point = {
                .freq = rec[i].freq,
                .loPwr = rec[i].loPwr,
                .hiPwr = rec[i].hiPwr,
                .txDevNarrow = limitDeviation(rec[i].txDevNarrow),
                .txDevWide = limitDeviation(rec[i].txDevWide),
            };
            addPoint(band, &point);
        }

        found = true;
    }

    free(entry);

    if (!found || (cal->vhf.count == 0) || (cal->uhf.count == 0))
        return -ENOENT;

    return 0;
}

void calibC62_defaults(struct C62Calib *cal)
{
    memset(cal, 0, sizeof(*cal));

    for (size_t i = 0; i < ARRAY_SIZE(defaultCalib); i++) {
        struct C62CalPoint point = defaultCalib[i];
        point.txDevNarrow = DEFAULT_TX_DEV_NARROW;
        point.txDevWide = DEFAULT_TX_DEV_WIDE;
        addPoint(bandOf(cal, point.freq), &point);
    }
}

/**
 * Interpolate a calibration value linearly between two frequencies.
 *
 * @param freq: frequency, in Hz, between f0 and f1.
 * @param f0: lower frequency, in Hz.
 * @param v0: value at f0.
 * @param f1: upper frequency, in Hz.
 * @param v1: value at f1.
 * @return value at freq.
 */
static uint16_t interpValue(freq_t freq, freq_t f0, uint16_t v0, freq_t f1,
                            uint16_t v1)
{
    int64_t num = (int64_t)(freq - f0) * ((int32_t)v1 - (int32_t)v0);

    return (uint16_t)(v0 + (num / (int64_t)(f1 - f0)));
}

struct C62CalPoint calibC62_interp(const struct C62BandCalib *band, freq_t freq)
{
    uint8_t last = band->count - 1;
    uint8_t i = 0;

    // First point at or above the frequency, or the last one
    while ((i < last) && (band->points[i].freq < freq))
        i++;

    const struct C62CalPoint *p1 = &band->points[i];
    struct C62CalPoint point = *p1;
    point.freq = freq;

    if ((i > 0) && (freq < p1->freq)) {
        const struct C62CalPoint *p0 = &band->points[i - 1];
        point.loPwr = interpValue(freq, p0->freq, p0->loPwr, p1->freq,
                                  p1->loPwr);
        point.hiPwr = interpValue(freq, p0->freq, p0->hiPwr, p1->freq,
                                  p1->hiPwr);
        point.txDevNarrow = interpValue(freq, p0->freq, p0->txDevNarrow,
                                        p1->freq, p1->txDevNarrow);
        point.txDevWide = interpValue(freq, p0->freq, p0->txDevWide, p1->freq,
                                      p1->txDevWide);
    }

    return point;
}
