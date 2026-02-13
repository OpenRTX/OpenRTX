/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/HDLC.hpp"

using namespace APRS;

Decoder::Decoder()
{
}

Decoder::~Decoder()
{
}

/**
 * @brief Checks the CRC of the currentFrame
 *
 * Calculates the CRC for the currentFrame excepting the last two byes and
 * compares the result to the last two bytes
 *
 * @return true if the frame is valid, false otherwise
 */
bool Decoder::checkCRC()
{
    if (currentFrame->size() < 3)
        return false;

    // CRC16 via table lookup by nibble
    // source: https://gist.github.com/andresv/4611897
    uint16_t crc = 0xFFFF;
    uint16_t crcTable[] = { 0x0000, 0x1081, 0x2102, 0x3183, 0x4204, 0x5285,
                            0x6306, 0x7387, 0x8408, 0x9489, 0xa50a, 0xb58b,
                            0xc60c, 0xd68d, 0xe70e, 0xf78f };

    for (size_t i = 0; i < (currentFrame->size() - 2); i++) {
        crc = (crc >> 4)
            ^ crcTable[(crc & 0x0f) ^ (currentFrame->at(i) & 0x0f)];
        crc = (crc >> 4) ^ crcTable[(crc & 0x0f) ^ (currentFrame->at(i) >> 4)];
    }

    crc = ~crc;

    DEBUG_PRINT("CRC Data %02x %02x Calculated %02x %02x\n",
                currentFrame->at(currentFrame->size() - 2),
                currentFrame->at(currentFrame->size() - 1), (uint8_t)(crc >> 8),
                (uint8_t)(crc & 0xff));

    if ((currentFrame->at(currentFrame->size() - 2) == (crc & 0xff))
        && (currentFrame->at(currentFrame->size() - 1) == ((crc >> 8) & 0xff)))
        return true;
    return false;
}

/*
 * @brief Performs HDLC decoding for incoming bytes from the slicer
 *
 * @param slicerBytes A uint8_t array of bytes from the slicer
 * @param slicerBytesSize The size_t size of the slicerBytes array
 *
 * @return A vector of vectors of uint8_t, each one being a decoded HDLC frame
 */
std::vector<std::vector<uint8_t> *> Decoder::decode(const uint8_t *slicerBytes,
                                                    size_t slicerBytesSize)
{
    std::vector<std::vector<uint8_t> *> frames;

    for (size_t i = 0; i < slicerBytesSize; i++) {
        for (int8_t j = 7; j >= 0; j--) {
            // pull out the bit we're dealing with working from left-to-right
            uint8_t bit = (slicerBytes[i] >> j) & 0b00000001;

            // perform NRZI decoding
            uint8_t nrziBit;
            if (bit != prevBit)
                nrziBit = 0; // the bit has changed, this is a '0'
            else
                nrziBit = 1; // the bit has NOT changed, this is a '1'

            DEBUG_PRINT("bit=%d prevBit=%d nrziBit=%d onesCount=%d"
                        "workingByte=%08b bitPos=%d\n",
                        bit, prevBit, nrziBit, onesCount, workingByte, bitPos);

            prevBit = bit;

            // unstuff bits and watch for flags/errors
            if ((onesCount == 5) && (nrziBit == 0)) {
                // if we have 5 ones and get a zero, it's a stuffed bit, disregard
                onesCount = 0;
                continue;
            } else if ((onesCount == 6) && (nrziBit == 0)) {
                // if we have 6 ones and get a zero, it's an HDLC flag
                DEBUG_PRINT("Flag received\n");

                // save and close out a frame if needed
                if (currentFrame && (currentFrame->size() > 0)) {
                    if (checkCRC()) {
                        DEBUG_PRINT(
                            "Adding valid frame to list and setting currentFrame to NULL\n");
                        // remove the last two bytes (CRC)
                        currentFrame->pop_back();
                        currentFrame->pop_back();
                        frames.push_back(currentFrame);
                        currentFrame = NULL;
                    } else {
                        DEBUG_PRINT("Clearing currentFrame\n");
                        currentFrame->clear();
                    }
                }

                // start a new frame if needed
                if (!currentFrame) {
                    DEBUG_PRINT("Starting new frame\n");
                    currentFrame = new std::vector<uint8_t>;
                }

                // clear our bit data
                bitPos = 0;
                workingByte = 0;
                onesCount = 0;

                continue;
            } else if ((onesCount == 6) and (nrziBit == 1)) {
                // if we have 6 ones and get a one it's an error
                DEBUG_PRINT("Invalid bit sequence, too many ones received\n");
                if (currentFrame) {
                    DEBUG_PRINT("Deleting current frame\n");
                    delete currentFrame;
                    currentFrame = NULL;
                }

                // clear our bit data
                bitPos = 0;
                workingByte = 0;
                onesCount = 0;
            }

            // keep track of the amount of ones received
            if (nrziBit)
                onesCount++;
            else
                onesCount = 0;

            // if we're working on a frame, pack bits into bytes
            if (currentFrame) {
                // add bits to the working byte (reversing them)
                workingByte |= nrziBit << bitPos;
                bitPos++;
                // if the working byte is full add it to the current frame
                if (bitPos >= 8) {
                    currentFrame->push_back(workingByte);
#ifdef APRS_DEBUG
                    printf("Frame: ");
                    for (auto &byte : *currentFrame)
                        printf("%02x ", byte);
                    printf("\n");
#endif
                    bitPos = 0;
                    workingByte = 0;
                }
            }
        }
    }
    return frames;
}
