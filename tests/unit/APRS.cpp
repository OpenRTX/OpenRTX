/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/dsp.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/packet.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>

constexpr const char *TEST_FILE = "../tests/unit/assets/aprs.raw";
constexpr const char *TEST_PACKETS[] = {
    "N2BP-7>APRS::N2BP-7   :Testing 1", "N2BP-7>APRS::N2BP-7   :Testing 2",
    "N2BP-7>APRS::N2BP-7   :Testing 3", "N2BP-7>APRS::N2BP-7   :Testing 4",
    "N2BP-7>APRS::N2BP-7   :Testing 5", "N2BP-7>APRS::N2BP-7   :Testing 6",
    "N2BP-7>APRS::N2BP-7   :Testing 7", "N2BP-7>APRS::N2BP-7   :Testing 8",
    "N2BP-7>APRS::N2BP-7   :Testing 9", "N2BP-7>APRS::N2BP-7   :Testing 10"
};

/**
 * @brief Fills a buffer with the TNC2 text representation of an APRS packet
 *
 * @param pkt pointer to an aprsPacket
 * @param output pointer to char buffer to fill
 */
void strFromPkt(struct aprsPacket *pkt, char *output)
{
    char ssid[4];

    output[0] = '\0';
    strcat(output, pkt->addresses[1].addr);
    if (pkt->addresses[1].ssid) {
        sprintf(ssid, "-%d", pkt->addresses[1].ssid);
        strcat(output, ssid);
    }
    strcat(output, ">");
    strcat(output, pkt->addresses[0].addr);
    for (uint8_t i = 2; i < pkt->addressesLen; i++) {
        strcat(output, ",");
        strcat(output, pkt->addresses[i].addr);
        if (pkt->addresses[i].ssid) {
            sprintf(ssid, "-%d", pkt->addresses[i].ssid);
            strcat(output, ssid);
        }
        if (pkt->addresses[i].commandHeard)
            strcat(output, "*");
    }
    strcat(output, ":");
    strcat(output, pkt->info);
}

int main()
{
    // Test APRS Demodulator on sample data
    printf("Testing APRS\n");

    FILE *file = fopen(TEST_FILE, "rb");
    if (!file) {
        printf("Error opening APRS baseband file\n");
        return -1;
    }

    dataBlock_t baseband;
    baseband.data = (int16_t *)malloc(APRS_BUF_SIZE * sizeof(int16_t));
    baseband.len = APRS_BUF_SIZE;
    if (!baseband.data) {
        printf("Error allocating memory for baseband buffer\n");
        return -1;
    }

    APRS::Demodulator demodulator;
    demodulator.init();
    size_t readItems = 0;
    size_t pktNum = 0;
    bool errorFlag = false;
    while ((readItems = fread(baseband.data, 2, APRS_BUF_SIZE, file)) > 0) {
        if (demodulator.update(baseband)) {
            frameData &frame = demodulator.getFrame();
            char pktStr[256];
            struct aprsPacket *pkt = aprsPktFromFrame(&frame);
            strFromPkt(pkt, pktStr);
            printf("Frame %ld: %s\n", pktNum, pktStr);
            free(pkt);
            if (strcmp(TEST_PACKETS[pktNum], pktStr)) {
                printf("Packet %ld does not match:\n", pktNum);
                printf("    Expected \"%s\"\n", TEST_PACKETS[pktNum]);
                printf("     Decoded \"%s\"\n", pktStr);
                errorFlag = true;
            }
            pktNum++;
        }
    }
    fclose(file);
    free(baseband.data);

    if (errorFlag) {
        return -1;
    }
    if (pktNum != 10) {
        printf("%ld frames decoded, expected 10\n", pktNum);
        return -1;
    }
    return 0;
}
