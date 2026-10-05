/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Compile-time layout check of cps.h packed structs against the sizes
 * and offsets measured from upstream/master (OpenRTX v0.4.5). If any
 * assert fails, stop; do not guess a fix.
 */

#include "core/cps.h"
#include <stddef.h>
#include <assert.h>

#define SAME_SIZE(T, N) \
    static_assert(sizeof(T) == (N), "sizeof(" #T ") != upstream " #N)
#define SAME_OFF(T, F, N) \
    static_assert(offsetof(T, F) == (N), #T "." #F " != upstream " #N)

SAME_SIZE(fmInfo_t, 2);
SAME_SIZE(dmrInfo_t, 4);
SAME_SIZE(dmrContact_t, 5);
SAME_SIZE(m17Info_t, 5);
SAME_SIZE(m17Contact_t, 6);
SAME_SIZE(geo_t, 9);
SAME_OFF(geo_t, ch_lat_int, 0);
SAME_OFF(geo_t, ch_lat_dec, 1);
SAME_OFF(geo_t, ch_lon_int, 3);
SAME_OFF(geo_t, ch_lon_dec, 5);
SAME_OFF(geo_t, ch_altitude, 7);
SAME_SIZE(channel_t, 94);
SAME_OFF(channel_t, mode, 0);
SAME_OFF(channel_t, power, 2);
SAME_OFF(channel_t, rx_frequency, 6);
SAME_OFF(channel_t, tx_frequency, 10);
SAME_OFF(channel_t, scanList_index, 14);
SAME_OFF(channel_t, groupList_index, 15);
SAME_OFF(channel_t, name, 16);
SAME_OFF(channel_t, descr, 48);
SAME_OFF(channel_t, ch_location, 80);
SAME_OFF(channel_t, fm, 89);
SAME_OFF(channel_t, dmr, 89);
SAME_OFF(channel_t, m17, 89);
SAME_SIZE(contact_t, 39);
SAME_OFF(contact_t, name, 0);
SAME_OFF(contact_t, mode, 32);
SAME_OFF(contact_t, info, 33);
SAME_SIZE(bankHdr_t, 34);
SAME_OFF(bankHdr_t, name, 0);
SAME_OFF(bankHdr_t, ch_count, 32);
SAME_SIZE(cps_header_t, 88);
SAME_OFF(cps_header_t, magic, 0);
SAME_OFF(cps_header_t, version_number, 8);
SAME_OFF(cps_header_t, author, 10);
SAME_OFF(cps_header_t, descr, 42);
SAME_OFF(cps_header_t, timestamp, 74);
SAME_OFF(cps_header_t, ct_count, 82);
SAME_OFF(cps_header_t, ch_count, 84);
SAME_OFF(cps_header_t, b_count, 86);
SAME_OFF(dmrContact_t, id, 0);
SAME_OFF(m17Contact_t, address, 0);
SAME_OFF(dmrInfo_t, dmr_timeslot, 1);
SAME_OFF(dmrInfo_t, contact_index, 2);
SAME_OFF(m17Info_t, gps_mode, 2);
SAME_OFF(m17Info_t, contact_index, 3);

/* Fork-only: fits in the existing union (m17Info_t is 5B). */
SAME_SIZE(horseInfo_t, 4);
SAME_OFF(channel_t, horse, 89);

int main(void)
{
    return 0;
}
