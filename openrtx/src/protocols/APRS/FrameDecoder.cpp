/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/APRS/FrameDecoder.hpp"
#include <cmath>

using namespace APRS;

FrameDecoder::FrameDecoder() :
    onesCount(0), prevBit(0), workingByte(0), bitPos(0), demodFrame(nullptr),
    readyFrame(nullptr), newFrame(false), state(State::WAITING)
{
}

FrameDecoder::~FrameDecoder()
{
}

void FrameDecoder::init()
{
    demodFrame = std::make_unique<struct frame>();
    readyFrame = std::make_unique<struct frame>();
    reset();
}

void FrameDecoder::terminate()
{
    demodFrame.reset();
    readyFrame.reset();
}

void FrameDecoder::reset()
{
    onesCount = 0;
    prevBit = 0;
    workingByte = 0;
    bitPos = 0;
    demodFrame->len = 0;
    readyFrame->len = 0;
    newFrame = false;
}

bool FrameDecoder::decode(uint8_t inputBit)
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

struct frame &FrameDecoder::getFrame()
{
    newFrame = false;
    return *readyFrame;
}

uint8_t FrameDecoder::nrziDecode(const uint8_t bit)
{
    uint8_t nrziBit;

    if (bit != prevBit)
        nrziBit = 0;
    else
        nrziBit = 1;

    prevBit = bit;

    return nrziBit;
}

FrameDecoder::Signal FrameDecoder::countBits(const uint8_t bit)
{
    if ((onesCount == 5) && (bit == 0)) {
        // Stuffed bit, disregard
        onesCount = 0;
        return Signal::IGNORE;
    } else if ((onesCount == 6) && (bit == 0)) {
        // HDLC flag
        onesCount = 0;
        return Signal::FLAG;
    } else if ((onesCount == 6) and (bit == 1)) {
        // Unexpected sequence
        onesCount = 0;
        return Signal::ERROR;
    }

    if (bit)
        onesCount++;
    else
        onesCount = 0;

    return Signal::NONE;
}

void FrameDecoder::closeFrame()
{
    if (demodFrame->len >= 2) {
        uint16_t frameCrc = demodFrame->data[demodFrame->len - 2]
                          | (demodFrame->data[demodFrame->len - 1] << 8);
        uint16_t calcCrc = crc_hdlc(demodFrame->data, demodFrame->len - 2);

        DEBUG_PRINT("frameCRC=%04x calculatedCRC=%04x\n", frameCrc, calcCrc);
        if (calcCrc == frameCrc) {
            DEBUG_PRINT("Swapping frames and setting newFrame\n");
            std::swap(demodFrame, readyFrame);
            newFrame = true;
        }
    }

    bitPos = 0;
    workingByte = 0;
    demodFrame->len = 0;
}

void FrameDecoder::addToFrame(uint8_t bit)
{
    if (demodFrame->len >= APRS_PACLEN)
        return;

    workingByte |= bit << bitPos;
    bitPos++;

    if (bitPos >= 8) {
        demodFrame->data[demodFrame->len] = workingByte;
        demodFrame->len++;
        bitPos = 0;
        workingByte = 0;

#ifdef APRS_DEBUG
        printf("Frame: ");
        for (size_t i = 0; i < demodFrame->len; i++)
            printf("%02x ", demodFrame->data[i]);
        printf("\n");
#endif
    }
}

void FrameDecoder::dumpFrame()
{
    state = State::WAITING;
    demodFrame->len = 0;
    bitPos = 0;
    workingByte = 0;
}
