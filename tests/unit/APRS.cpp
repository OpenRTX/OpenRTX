#include "core/dsp.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/Slicer.hpp"
#include "protocols/APRS/HDLC.hpp"
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
 * @param pkt pointer to an aprsPacket_t
 * @param output pointer to char buffer to fill
 */
void strFromPkt(aprsPacket_t *pkt, char *output)
{
    char ssid[4];

    aprsAddress_t *dst = pkt->addresses;
    aprsAddress_t *src = pkt->addresses->next;
    output[0] = '\0';
    strcat(output, src->addr);
    if (src->ssid) {
        sprintf(ssid, "-%d", src->ssid);
        strcat(output, ssid);
    }
    strcat(output, ">");
    strcat(output, dst->addr);
    if (src->next) {
        for (aprsAddress_t *address = src->next; address;
             address = address->next) {
            strcat(output, ",");
            strcat(output, address->addr);
            if (address->ssid) {
                sprintf(ssid, "-%d", address->ssid);
                strcat(output, ssid);
            }
            if (address->commandHeard)
                strcat(output, "*");
        }
    }
    strcat(output, ":");
    strcat(output, pkt->info);
}

int main()
{
    // Test APRS Demodulator, Slicer, Decoder, and packet on sample data
    printf("Testing APRS\n");

    FILE *basebandFile = fopen(TEST_FILE, "rb");
    if (!basebandFile) {
        printf("Error opening APRS baseband file\n");
        return -1;
    }
    int16_t *basebandBuffer =
        (int16_t *)malloc(APRS_BUF_SIZE * sizeof(int16_t));
    if (!basebandBuffer) {
        printf("Error allocating memory for baseband buffer\n");
        return -1;
    }
    FILE *outputFile = fopen("APRS_demodulator_output.csv", "w");
    if (!outputFile) {
        printf("Error opening APRS demodulator output file\n");
        return -1;
    }

    struct dcBlock dcBlock;
    dsp_resetState(dcBlock);
    APRS::Demodulator demodulator;
    APRS::Slicer slicer;
    APRS::Decoder decoder;
    size_t sampleNum = 0;
    size_t readItems = 0;
    size_t pktNum = 0;
    bool errorFlag = false;
    while ((readItems = fread(basebandBuffer, 2, APRS_BUF_SIZE, basebandFile))
           > 0) {
        // remove DC bias and scale down the baseband audio by 4
        for (size_t i = 0; i < readItems; i++)
            basebandBuffer[i] = dsp_dcBlockFilter(&dcBlock, basebandBuffer[i])
                             >> 2;
        const int16_t *demod_output = demodulator.demodulate(basebandBuffer);
        for (size_t i = 0; i < readItems; i++)
            fprintf(outputFile, "%ld,%d\n", sampleNum++, demod_output[i]);
        size_t slicedBytes = 0;
        const uint8_t *slicer_output = slicer.slice(demod_output, slicedBytes);
        std::vector<std::vector<uint8_t> *> frames =
            decoder.decode(slicer_output, slicedBytes);
        for (auto frame : frames) {
            char pktStr[256];
            aprsPacket_t *pkt = aprsPktFromFrame(frame->data(), frame->size());
            delete frame;
            strFromPkt(pkt, pktStr);
            aprsPktFree(pkt);
            if (strcmp(TEST_PACKETS[pktNum], pktStr)) {
                printf("Packet %ld does not match:\n", pktNum);
                printf("    Expected \"%s\"\n", TEST_PACKETS[pktNum]);
                printf("     Decoded \"%s\"\n", pktStr);
                errorFlag = true;
            }
            pktNum++;
        }
    }
    fclose(outputFile);
    fclose(basebandFile);
    free(basebandBuffer);

#ifdef APRS_DEBUG
    FILE *decisionsFile = fopen("APRS_slicer_decisions.csv", "w");
    if (!decisionsFile) {
        printf("Error opening APRS slicer decisions file\n");
        return -1;
    }
    for (auto decision : slicer.decisions)
        fprintf(decisionsFile, "%ld\n", decision);
    fclose(decisionsFile);
#endif

    if (errorFlag) {
        return -1;
    }
    return 0;
}
