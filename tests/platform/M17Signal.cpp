/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <algorithm>
#include <array>
#include <cstring>
#include <random>

#include "core/crc.h"
#include "core/fir.hpp"
#include "protocols/M17/DSP.hpp"
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "protocols/M17/PacketFrame.hpp"
#include "protocols/M17/Utils.hpp"
#include "M17Signal.hpp"

using namespace M17;

std::vector<int16_t> M17Signal::rrcBaseband(const std::vector<int8_t> &symbols,
                                            const float amplitude)
{
    static constexpr size_t SPS = 24000 / SYMBOL_RATE;
    static constexpr size_t NTAPS = rrc_taps_24k.size();

    Fir<NTAPS> txRrc(rrc_taps_24k);

    std::vector<int16_t> out;
    out.reserve(symbols.size() * SPS);

    for (size_t i = 0; i < symbols.size(); i++) {
        // First sample of the symbol period carries the impulse
        float imp = static_cast<float>(symbols[i]) * amplitude;
        out.push_back(static_cast<int16_t>(txRrc(imp) * SPS));

        for (size_t s = 1; s < SPS; s++)
            out.push_back(static_cast<int16_t>(txRrc(0.0f) * SPS));
    }

    return out;
}

void M17Signal::appendPreamble(std::vector<int8_t> &symbols, const size_t count)
{
    for (size_t i = 0; i < count; i++)
        symbols.push_back((i % 2 == 0) ? +3 : -3);
}

void M17Signal::appendFrame(std::vector<int8_t> &symbols, const frame_t &frame)
{
    for (uint8_t byte : frame) {
        auto s = byteToSymbols(byte);
        symbols.insert(symbols.end(), s.begin(), s.end());
    }
}

std::vector<int8_t> M17Signal::m17Symbols(const std::vector<frame_t> &frames)
{
    std::vector<int8_t> symbols;

    appendPreamble(symbols);
    for (const frame_t &frame : frames)
        appendFrame(symbols, frame);

    return symbols;
}

std::vector<int8_t> M17Signal::otherSymbols(const uint32_t seed,
                                            const double seconds)
{
    static constexpr int8_t LEVELS[] = { -3, -1, +1, +3 };
    std::vector<std::array<int8_t, 8>> syncwords;

    for (const syncw_t &sw :
         { LSF_SYNC_WORD, STREAM_SYNC_WORD, PACKET_SYNC_WORD }) {
        auto hi = byteToSymbols(sw[0]);
        auto lo = byteToSymbols(sw[1]);
        std::array<int8_t, 8> symbols;
        std::copy(hi.begin(), hi.end(), symbols.begin());
        std::copy(lo.begin(), lo.end(), symbols.begin() + 4);
        syncwords.push_back(symbols);
    }

    std::mt19937 rng(seed);
    std::vector<int8_t> symbols;
    const size_t len = std::lround(seconds * SYMBOL_RATE);

    while (symbols.size() < len) {
        symbols.push_back(LEVELS[rng() % 4]);
        if (symbols.size() < 8)
            continue;

        for (const auto &sw : syncwords) {
            if (std::equal(sw.begin(), sw.end(), symbols.end() - 8)) {
                symbols.pop_back();
                break;
            }
        }
    }

    return symbols;
}

static LinkSetupFrame linkSetup(const uint8_t dataMode, const uint8_t dataType)
{
    LinkSetupFrame lsf;
    lsf.clear();
    lsf.setSource(Callsign("N0CALL"));
    lsf.setDestination(Callsign("ALL"));

    streamType_t type;
    type.value = 0;
    type.fields.dataMode = dataMode;
    type.fields.dataType = dataType;
    lsf.setType(type);

    return lsf;
}

std::vector<frame_t> M17Signal::voiceFrames(const size_t numFrames,
                                            const bool eot)
{
    LinkSetupFrame lsf = linkSetup(DATAMODE_STREAM, DATATYPE_VOICE);
    FrameEncoder encoder;
    std::vector<frame_t> frames(1);
    encoder.encodeLsf(lsf, frames.back());

    for (size_t i = 0; i < numFrames; i++) {
        payload_t payload;
        payload.fill(static_cast<uint8_t>(0x11 * (i + 1)));

        frames.emplace_back();
        encoder.encodeStreamFrame(payload, frames.back(),
                                  eot && ((i + 1) == numFrames));
    }

    if (eot) {
        frames.emplace_back();
        encoder.encodeEotFrame(frames.back());
    }

    return frames;
}

std::vector<frame_t> M17Signal::smsFrames(const size_t numFrames,
                                          const char fill)
{
    const size_t dataSize = PacketFrame::DATA_SIZE;
    std::vector<uint8_t> packet(1, 0x05);
    packet.insert(packet.end(), (numFrames * dataSize) - 4, fill);
    packet.push_back(0x00);

    uint16_t crc = crc_m17(packet.data(), packet.size());
    packet.push_back(crc >> 8);
    packet.push_back(crc & 0xFF);

    LinkSetupFrame lsf = linkSetup(DATAMODE_PACKET, DATATYPE_DATA);
    FrameEncoder encoder;
    std::vector<frame_t> frames(1);
    encoder.encodeLsf(lsf, frames.back());

    for (size_t i = 0; i < numFrames; i++) {
        bool last = (i + 1) == numFrames;
        PacketFrame pf;
        std::memcpy(pf.data(), &packet[i * dataSize], dataSize);
        pf.setEof(last);
        pf.setCounter(last ? dataSize : i);

        frames.emplace_back();
        encoder.encodePacketFrame(pf, frames.back());
    }

    frames.emplace_back();
    encoder.encodeEotFrame(frames.back());

    return frames;
}

M17Channel::M17Channel(const uint32_t seed)
    : Channel(RATE, seed, IDLE_RMS, CARRIER_RMS)
{
}

void M17Channel::transmit(const std::vector<int8_t> &symbols,
                          const size_t phase)
{
    align(SAMPLES_PER_SYMBOL, phase);

    Fir<std::tuple_size<decltype(rrc_taps_48k)>::value> rrc(rrc_taps_48k);
    const size_t len = (symbols.size() * SAMPLES_PER_SYMBOL)
                     + rrc_taps_48k.size();
    std::vector<float> signal;
    signal.reserve(len);

    for (size_t i = 0; i < len; i++) {
        float pulse = 0.0f;
        size_t symbol = i / SAMPLES_PER_SYMBOL;
        if (((i % SAMPLES_PER_SYMBOL) == 0) && (symbol < symbols.size()))
            pulse = symbols[symbol] * SYMBOL_GAIN;

        signal.push_back(rrc(pulse));
    }

    transmit(signal);
}
