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
    demodFrame = std::make_unique<struct hdlcFrame>();
    readyFrame = std::make_unique<struct hdlcFrame>();
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

struct hdlcFrame &FrameDecoder::getFrame()
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
    if ((onesCount == STUFFED_ONES) && (bit == 0)) {
        // Stuffed bit, disregard
        onesCount = 0;
        return Signal::IGNORE;
    } else if ((onesCount == FLAG_ONES) && (bit == 0)) {
        // HDLC flag
        onesCount = 0;
        return Signal::FLAG;
    } else if ((onesCount == FLAG_ONES) && (bit == 1)) {
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
    if (demodFrame->len >= APRS_FCS_LEN) {
        const size_t len = demodFrame->len - APRS_FCS_LEN;
        uint16_t frameCrc = demodFrame->data[len]
                          | (demodFrame->data[len + 1] << 8);
        uint16_t calcCrc = crc_hdlc(demodFrame->data, len);

        if (calcCrc == frameCrc) {
            // Hand back the frame the FCS protects, without the FCS
            demodFrame->len = len;
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
    }
}

void FrameDecoder::dumpFrame()
{
    state = State::WAITING;
    demodFrame->len = 0;
    bitPos = 0;
    workingByte = 0;
}
