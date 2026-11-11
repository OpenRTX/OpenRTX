/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cstring>
#include "aprs_test_frame.h"

/* Last byte of an address, CRRSSSSL: the H bit, two reserved bits sent as
 * ones, the SSID and the last-address bit. */
static constexpr uint8_t ADDR_H = 0x80;
static constexpr uint8_t ADDR_RESERVED = 0x60;
static constexpr uint8_t ADDR_SSID_SHIFT = 1;
static constexpr uint8_t ADDR_SSID_MASK = 0x0f;
static constexpr uint8_t ADDR_LAST = 0x01;

/* Control and protocol identifier of an APRS frame: a UI frame carrying no
 * layer 3 protocol. */
static constexpr uint8_t AX25_CTRL_UI = 0x03;
static constexpr uint8_t AX25_PID_NO_L3 = 0xf0;

static constexpr size_t CALL_LEN = APRS_ADDR_LEN - 1;
static constexpr size_t INFO_OFFSET = (2 * APRS_ADDR_LEN) + APRS_CTRL_PID_LEN;

void aprsTest_putAddress(uint8_t *dst, const char *call, uint8_t ssid,
                         bool repeated, bool last)
{
    size_t i = 0;

    for (; (i < CALL_LEN) && (call[i] != '\0'); i++)
        dst[i] = (uint8_t)(call[i] << 1);
    for (; i < CALL_LEN; i++)
        dst[i] = (uint8_t)(' ' << 1);

    dst[CALL_LEN] = ADDR_RESERVED;
    dst[CALL_LEN] |= (uint8_t)((ssid & ADDR_SSID_MASK) << ADDR_SSID_SHIFT);
    if (repeated)
        dst[CALL_LEN] |= ADDR_H;
    if (last)
        dst[CALL_LEN] |= ADDR_LAST;
}

struct aprsTestFrame aprsTest_makeFrame(const char *dst, uint8_t dstSsid,
                                        const char *src, uint8_t srcSsid,
                                        const char *info)
{
    struct aprsTestFrame frame;
    const size_t infoLen = strlen(info);

    memset(&frame, 0, sizeof(frame));
    aprsTest_putAddress(&frame.data[0], dst, dstSsid, false, false);
    aprsTest_putAddress(&frame.data[APRS_ADDR_LEN], src, srcSsid, false, true);
    frame.data[2 * APRS_ADDR_LEN] = AX25_CTRL_UI;
    frame.data[(2 * APRS_ADDR_LEN) + 1] = AX25_PID_NO_L3;
    memcpy(&frame.data[INFO_OFFSET], info, infoLen);
    frame.len = INFO_OFFSET + infoLen;

    return frame;
}
