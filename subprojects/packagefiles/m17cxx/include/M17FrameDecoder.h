// Copyright 2020 Mobilinkd LLC.

#pragma once

#include "M17Randomizer.h"
#include "PolynomialInterleaver.h"
#include "Trellis.h"
#include "Viterbi.h"
#include "CRC16.h"
#include "LinkSetupFrame.h"
#include "Golay24.h"

#include <codec2/codec2.h>

#include <algorithm>
#include <array>
#include <functional>
//#include <iostream>
//#include <iomanip>

extern bool display_lsf;

namespace mobilinkd
{

struct M17FrameDecoder
{
    M17Randomizer<368> derandomize_;
    PolynomialInterleaver<45, 92, 368> interleaver_;
    Trellis<4,2> trellis_{makeTrellis<4, 2>({031,027})};
    Viterbi<decltype(trellis_), 4> viterbi_{trellis_};
    CRC16<0x5935, 0xFFFF> crc_;

    enum class State {LS_FRAME, LS_LICH, AUDIO};

    State state_ = State::LS_FRAME;

    using buffer_t = std::array<int8_t, 368>;

    using lsf_conv_buffer_t = std::array<uint8_t, 46>;
    using lsf_buffer_t = std::array<uint8_t, 30>;

    using audio_conv_buffer_t = std::array<uint8_t, 34>;
    using audio_buffer_t = std::array<uint8_t, 20>;

    using link_setup_callback_t = std::function<void(audio_buffer_t)>;
    using audio_callback_t = std::function<void(audio_buffer_t)>;
    
    link_setup_callback_t link_setup_callback_;
    audio_callback_t audio_callback_;
    lsf_buffer_t lsf_output;
    std::array<int8_t, 6> lich_buffer;
    uint8_t lich_segments{0};       ///< one bit per received LICH fragment.

    struct CODEC2 *codec2;

    M17FrameDecoder(
        link_setup_callback_t link_setup_callback = link_setup_callback_t(),
        audio_callback_t audio_callback = audio_callback_t()
    )
    : link_setup_callback_(link_setup_callback)
    , audio_callback_(audio_callback)
    {
        codec2 = ::codec2_create(CODEC2_MODE_3200);
    }

    ~M17FrameDecoder()
    {
        codec2_destroy(codec2);
    }

    void reset() { state_ = State::LS_FRAME; }

    void dump_lsf(const lsf_buffer_t& lsf)
    {
        LinkSetupFrame::encoded_call_t encoded_call;
        std::copy(lsf.begin() + 6, lsf.begin() + 12, encoded_call.begin());
        auto src = LinkSetupFrame::decode_callsign(encoded_call);
        std::cerr << "\nSRC: ";
        for (auto x : src) if (x) std::cerr << x;
        std::copy(lsf.begin(), lsf.begin() + 6, encoded_call.begin());
        std::cerr << ", DEST: ";
        for (auto x : encoded_call) if (x) std::cerr << std::hex << std::setw(2) << std::setfill('0') << int(x);
        uint16_t type = (lsf[12] << 8) | lsf[13];
        std::cerr << ", TYPE: " << std::setw(4) << std::setfill('0') << type;
        std::cerr << ", NONCE: ";
        for (size_t i = 14; i != 28; ++i) std::cerr << std::hex << std::setw(2) << std::setfill('0') << int(lsf[i]);
        uint16_t crc = (lsf[28] << 8) | lsf[29];
        std::cerr << ", CRC: " << std::setw(4) << std::setfill('0') << crc;
        std::cerr << std::dec << std::endl;
    }

    bool decode_lsf(buffer_t& buffer, size_t& ber)
    {
        std::array<uint8_t, 240> output;
        auto dp = depunctured<488>(P1, buffer);
        // std::cerr << std::endl;
        // for (auto i : dp) std::cerr << int(i) << ", ";
        // std::cerr << std::endl;
        ber = viterbi_.decode(dp, output) - 60;
        auto lsf = to_byte_array(output);
        crc_.reset();
        for (auto x : lsf) crc_(x);
        auto checksum = crc_.get();
        if (checksum != 0) 
        {
            // std::cerr << "\nLSF checksum failure." << std::endl;
            // for (auto c : lsf) std::cerr << std::hex << std::setw(2) << std::setfill('0') << int(c);
            // std::cerr << std::dec  << "BER = " << ber << std::endl;
            lsf_output.fill(0);
            state_ = State::LS_LICH;
            return false;
        }

        state_ = State::AUDIO;
        if (display_lsf) dump_lsf(lsf);
        return true;
    }

    void demodulate_audio(audio_buffer_t audio)
    {
        std::array<int16_t, 160> buf;
        codec2_decode(codec2, buf.begin(), audio.begin() + 2);
        std::cout.write((const char*)buf.begin(), 320);
        codec2_decode(codec2, buf.begin(), audio.begin() + 10);
        std::cout.write((const char*)buf.begin(), 320);
    }

    bool decode_audio(buffer_t& buffer, size_t& ber)
    {
        std::array<int8_t, 272> tmp;
        std::copy(buffer.begin() + 96, buffer.end(), tmp.begin());
        std::array<uint8_t, 160> output;
        auto dp = depunctured<328>(P2, tmp);
        ber = viterbi_.decode(dp, output) - 28;
        auto audio = to_byte_array(output);
        crc_.reset();
        for (auto x : audio) crc_(x);
        auto checksum = crc_.get();
        demodulate_audio(audio);
        uint16_t fn = (audio[0] << 8) | audio[1];
        if (checksum == 0 && fn > 0x7fff)
        {
            std::cerr << "EOS" << std::endl; 
            state_ = State::LS_FRAME;
        }
        return true;
    }

    // Unpack  & decode LICH fragments into tmp_buffer.
    bool unpack_lich(buffer_t& buffer)
    {
        size_t index = 0;
        // Read the 4 24-bit codewords from LICH
        for (size_t i = 0; i != 4; ++i) // for each codeword
        {
            uint32_t codeword = 0;
            for (size_t j = 0; j != 24; ++j) // for each bit in codeword
            {
                codeword <<= 1;
                codeword |= (buffer[i * 24 + j] > 0);
            }
            uint32_t decoded = 0;
            if (!Golay24::decode(codeword, decoded))
            {
                return false;
            }
            decoded >>= 12; // Remove check bits and parity.
            // append codeword.
            if (i & 1)
            {
                lich_buffer[index++] |= (decoded >> 8);     // upper 4 bits
                lich_buffer[index++] = (decoded & 0xFF);    // lower 8 bits
            }
            else
            {
                lich_buffer[index++] |= (decoded >> 4);     // upper 8 bits
                lich_buffer[index] = (decoded & 0x0F) << 4; // lower 4 bits
            }
        }
        return true;
    }

    bool decode_lich(buffer_t& buffer, size_t& ber)
    {
        lich_buffer.fill(0);
        // Read the 4 12-bit codewords from LICH into tmp_buffer.
        if (!unpack_lich(buffer)) return false;

        uint8_t fragment_number = lich_buffer[5];   // Get fragment number.
        fragment_number >>= 5;

        // Copy decoded LICH to superframe buffer.
        std::copy(lich_buffer.begin(), lich_buffer.begin() + 5,
            lsf_output.begin() + (fragment_number * 5));

        lich_segments |= (1 << fragment_number);        // Indicate segment received.
        if (lich_segments != 0x3F) return false;        // More to go...

        crc_.reset();
        for (auto x : lsf_output) crc_(x);
        auto it = lsf_output.begin();
        auto checksum = crc_.get();
        if (checksum == 0)
        {
            state_ = State::AUDIO;
            if (display_lsf) dump_lsf(lsf_output);
            return true;
        }
        // Failed CRC... try again.
        lich_segments = 0;
        lsf_output.fill(0);
        return false;
    }

    bool operator()(buffer_t& buffer, size_t& ber)
    {
        derandomize_(buffer);
        interleaver_.deinterleave(buffer);
 
        switch(state_)
        {
        case State::LS_FRAME:
            return decode_lsf(buffer, ber);
        case State::LS_LICH:
            return decode_lich(buffer, ber);
        case State::AUDIO:
             return decode_audio(buffer, ber);
        }
        return false;
    }
};

} // mobilinkd
