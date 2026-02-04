/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <catch2/catch_test_macros.hpp>

#include <cstdio>
#include <cstring>

#include "core/utils.h"
#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/packet.h"

static constexpr const char *TEST_FILE = APRS_ASSET_DIR "/aprs.raw";
static constexpr const char *TEST_PACKETS[] = {
    "N2BP-7>APRS::N2BP-7   :Testing 1", "N2BP-7>APRS::N2BP-7   :Testing 2",
    "N2BP-7>APRS::N2BP-7   :Testing 3", "N2BP-7>APRS::N2BP-7   :Testing 4",
    "N2BP-7>APRS::N2BP-7   :Testing 5", "N2BP-7>APRS::N2BP-7   :Testing 6",
    "N2BP-7>APRS::N2BP-7   :Testing 7", "N2BP-7>APRS::N2BP-7   :Testing 8",
    "N2BP-7>APRS::N2BP-7   :Testing 9", "N2BP-7>APRS::N2BP-7   :Testing 10",
};

/**
 * Format a packet as a TNC2 monitor line, "SRC>DEST,PATH:info".
 */
static void tnc2FromPkt(const struct aprsPacket *pkt, char *out, size_t len)
{
    char src[APRS_ADDR_STR_LEN];
    char dest[APRS_ADDR_STR_LEN];
    char path[APRS_MAX_ADDRESSES * (APRS_ADDR_STR_LEN + 1)];

    aprsAddr_toStr(&pkt->addresses[1], src, sizeof(src));
    aprsAddr_toStr(&pkt->addresses[0], dest, sizeof(dest));
    aprsPath_toStr(pkt, path, sizeof(path));

    snprintf(out, len, "%s>%s%s%s:%.*s", src, dest,
             (path[0] != '\0') ? "," : "", path, (int)pkt->infoLen, pkt->info);
}

TEST_CASE("APRS demodulator: recorded packets decode", "[aprs]")
{
    FILE *file = fopen(TEST_FILE, "rb");
    REQUIRE(file != nullptr);

    int16_t samples[APRS_BUF_SIZE];
    dataBlock_t baseband = { samples, APRS_BUF_SIZE };
    APRS::Demodulator demodulator;
    size_t pktNum = 0;

    demodulator.init();

    while (fread(samples, sizeof(samples[0]), APRS_BUF_SIZE, file) > 0) {
        if (!demodulator.update(baseband))
            continue;

        uint8_t frame[APRS_PACLEN];
        const ssize_t frameLen = demodulator.getFrame(frame, sizeof(frame));
        REQUIRE(frameLen > 0);

        struct aprsPacket pkt;
        REQUIRE(aprsPkt_fromFrame(frame, (size_t)frameLen, &pkt) == 0);

        char line[APRS_PACLEN];
        tnc2FromPkt(&pkt, line, sizeof(line));

        INFO("frame " << pktNum);
        REQUIRE(pktNum < ARRAY_SIZE(TEST_PACKETS));
        CHECK(strcmp(line, TEST_PACKETS[pktNum]) == 0);
        pktNum++;
    }

    fclose(file);
    demodulator.terminate();

    REQUIRE(pktNum == ARRAY_SIZE(TEST_PACKETS));
}
