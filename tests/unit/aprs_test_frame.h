/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_TEST_FRAME_H
#define APRS_TEST_FRAME_H

#include <cstddef>
#include <cstdint>
#include "protocols/APRS/constants.h"

/*
 * AX.25 frames built by hand for the APRS tests, so each test can pin one
 * property of the code receiving them.
 */

/**
 * A frame the way the decoder hands one back: raw bytes, without the FCS,
 * and a length.
 */
struct aprsTestFrame {
    uint8_t data[APRS_PACLEN]; ///< Frame bytes.
    size_t len;                ///< Frame length.
};

/**
 * Write one shifted-ASCII AX.25 address into a frame.
 *
 * @param dst: where the seven address bytes go.
 * @param call: callsign, up to six characters.
 * @param ssid: SSID, 0 to 15.
 * @param repeated: set the has-been-repeated bit.
 * @param last: set the last-address bit.
 */
void aprsTest_putAddress(uint8_t *dst, const char *call, uint8_t ssid,
                         bool repeated, bool last);

/**
 * Build a two-address (destination, source) UI frame.
 *
 * @param dst: destination callsign.
 * @param dstSsid: destination SSID.
 * @param src: source callsign.
 * @param srcSsid: source SSID.
 * @param info: info field, NUL-terminated.
 * @return the frame.
 */
struct aprsTestFrame aprsTest_makeFrame(const char *dst, uint8_t dstSsid,
                                        const char *src, uint8_t srcSsid,
                                        const char *info);

#endif /* APRS_TEST_FRAME_H */
