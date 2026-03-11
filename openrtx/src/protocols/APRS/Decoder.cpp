/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/Decoder.hpp"
#include <cmath>

using namespace APRS;

Decoder::Decoder()
{
}

Decoder::~Decoder()
{
    terminate();
}

void Decoder::init()
{
    demodFrame = std::make_unique<struct frameData>();
    readyFrame = std::make_unique<struct frameData>();

    reset();
}

void Decoder::terminate()
{
    demodFrame.reset();
    readyFrame.reset();
}

void Decoder::reset()
{
    onesCount = 0;
    prevBit = 0;
    workingByte = 0;
    bitPos = 0;
    demodFrame->len = 0;
    readyFrame->len = 0;
    newFrame = false;
}

uint8_t Decoder::nrziDecode(const uint8_t bit)
{
    uint8_t nrziBit;

    if (bit != prevBit)
        nrziBit = 0; // the bit has changed, this is a '0'
    else
        nrziBit = 1; // the bit has NOT changed, this is a '1'
    prevBit = bit;
    return nrziBit;
}

Decoder::Signal Decoder::countBits(const uint8_t bit)
{
    if ((onesCount == 5) && (bit == 0)) {
        // if we have 5 ones and get a zero, it's a stuffed bit, disregard
        onesCount = 0;
        return Signal::IGNORE;
    } else if ((onesCount == 6) && (bit == 0)) {
        // if we have 6 ones and get a zero, it's an HDLC flag
        onesCount = 0;
        return Signal::FLAG;
    } else if ((onesCount == 6) and (bit == 1)) {
        // if we have 6 ones and get a one it's an error
        onesCount = 0;
        return Signal::ERROR;
    }

    // otherwise just keep track of the amount of ones received
    if (bit)
        onesCount++;
    else
        onesCount = 0;

    return Signal::NONE;
}

void Decoder::closeFrame()
{
    bitPos = 0;
    workingByte = 0;

    // we need at least 2 bytes to perform CRC
    if (demodFrame->len < 2) {
        demodFrame->len = 0;
        return;
    }

    // remove the last two bytes (CRC)
    uint16_t frameCRC = demodFrame->data[demodFrame->len - 2]
                      | (demodFrame->data[demodFrame->len - 1] << 8);
    demodFrame->len -= 2;

    // check CRC and swap if valid
    uint16_t calculatedCRC = crc_hdlc(demodFrame->data, demodFrame->len);
    DEBUG_PRINT("frameCRC=%04x calculatedCRC=%04x\n", frameCRC, calculatedCRC);
    if (calculatedCRC == frameCRC) {
        DEBUG_PRINT("Swapping frames and setting newFrame\n");
        std::swap(demodFrame, readyFrame);
        newFrame = true;
    }
    demodFrame->len = 0; // this is for the new frame
}

void Decoder::addToFrame(uint8_t bit)
{
    if (demodFrame->len >= APRS_PACLEN)
        return;

    workingByte |= bit << bitPos;
    bitPos++;
    if (bitPos >= 8) {
        demodFrame->data[demodFrame->len++] = workingByte;
#ifdef APRS_DEBUG
        printf("Frame: ");
        for (size_t i = 0; i < demodFrame->len; i++)
            printf("%02x ", demodFrame->data[i]);
        printf("\n");
#endif
        bitPos = 0;
        workingByte = 0;
    }
}

void Decoder::dumpFrame()
{
    state = State::WAITING;
    demodFrame->len = 0;
    bitPos = 0;
    workingByte = 0;
}

bool Decoder::decode(uint8_t inputBit)
{
    uint8_t bit = nrziDecode(inputBit);

    Signal signal = countBits(bit);

    DEBUG_PRINT("state=%c signal=%c\n", (char)state, (char)signal);
    switch (state) {
        case State::WAITING:
            if (signal == Signal::FLAG)
                state = State::INFRAME;
            break;
        case State::INFRAME:
            switch (signal) {
                case Signal::FLAG:
                    closeFrame();
                    break;
                case Signal::ERROR:
                    dumpFrame();
                    break;
                case Signal::NONE:
                    addToFrame(bit);
                    break;
                case Signal::IGNORE:
                    break;
            }
            break;
    }
    return newFrame;
}

struct frameData &Decoder::getFrame()
{
    newFrame = false;
    return *readyFrame;
}
