/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 3 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, see <http://www.gnu.org/licenses/>   *
 ***************************************************************************/

/*
 * This code was partially adapted from m17-mod code by Mobilinkd
 */

#include <interfaces/platform.h>
#include <interfaces/delays.h>
#include <interfaces/radio.h>
#include <interfaces/audio.h>
#include <interfaces/audio_path.h>
#include <interfaces/audio_stream.h>
#include <OpMode_M17.h>
#include <M17Modulator.h>
#include <rtx.h>

// Generated using scikit-commpy
const auto rrc_taps = std::experimental::make_array<double>(
    -0.009265784007800534, -0.006136551625729697, -0.001125978562075172, 0.004891777252042491,
    0.01071805138282269, 0.01505751553351295, 0.01679337935001369, 0.015256245142156299,
    0.01042830577908502, 0.003031522725559901,  -0.0055333532968188165, -0.013403099825723372,
    -0.018598682349642525, -0.01944761739590459, -0.015005271935951746, -0.0053887880354343935,
    0.008056525910253532, 0.022816244158307273, 0.035513467692208076, 0.04244131815783876,
    0.04025481153629372, 0.02671818654865632, 0.0013810216516704976, -0.03394615682795165,
    -0.07502635967975885, -0.11540977897637611, -0.14703962203941534, -0.16119995609538576,
    -0.14969512896336504, -0.10610329539459686, -0.026921412469634916, 0.08757875030779196,
    0.23293327870303457, 0.4006012210123992, 0.5786324696325503, 0.7528286479934068,
    0.908262741447522, 1.0309661131633199, 1.1095611856548013, 1.1366197723675815,
    1.1095611856548013, 1.0309661131633199, 0.908262741447522, 0.7528286479934068,
    0.5786324696325503,  0.4006012210123992, 0.23293327870303457, 0.08757875030779196,
    -0.026921412469634916, -0.10610329539459686, -0.14969512896336504, -0.16119995609538576,
    -0.14703962203941534, -0.11540977897637611, -0.07502635967975885, -0.03394615682795165,
    0.0013810216516704976, 0.02671818654865632, 0.04025481153629372,  0.04244131815783876,
    0.035513467692208076, 0.022816244158307273, 0.008056525910253532, -0.0053887880354343935,
    -0.015005271935951746, -0.01944761739590459, -0.018598682349642525, -0.013403099825723372,
    -0.0055333532968188165, 0.003031522725559901, 0.01042830577908502, 0.015256245142156299,
    0.01679337935001369, 0.01505751553351295, 0.01071805138282269, 0.004891777252042491,
    -0.001125978562075172, -0.006136551625729697, -0.009265784007800534
);

/*
 * Converts bits of binary data into an encoding symbol
 */
int8_t bits_to_symbol(uint8_t bits)
{
    switch (bits)
    {
        case 0: return 1;
        case 1: return 3;
        case 2: return -1;
        case 3: return -3;
    }
    abort();
}

/*
 * Converts bits of binary data into encoding symbols
 */
template <typename T, size_t N>
void bits_to_symbols(const std::array<T, N>& bits,
                     std::array<int8_t, N / 2>& symbols)
{
    size_t index = 0;
    for (size_t i = 0; i != N; i += 2)
    {
        symbols[index++] = bits_to_symbol((bits[i] << 1) | bits[i + 1]);
    }
}

/*
 * Converts bytes of binary data into encoding symbols
 */
template <typename T, size_t N>
void bytes_to_symbols(const std::array<T, N>& bytes,
                      std::array<int8_t, N * 4>& symbols)
{
    size_t index = 0;
    for (auto b : bytes)
    {
        for (size_t i = 0; i != 4; ++i)
        {
            symbols[index++] = bits_to_symbol(b >> 6);
            b <<= 2;
        }
    }
}

/*
 * Converts symbols into the modulated waveform of the baseband
 */
void symbols_to_baseband(const std::array<int8_t, M17_FRAME_SYMBOLS>& symbols,
                    std::array<audio_sample_t, M17_FRAME_SAMPLES>& baseband)
{
    using namespace mobilinkd;

    static BaseFirFilter<double, std::tuple_size<decltype(rrc_taps)>::value> rrc = makeFirFilter(rrc_taps);

    baseband.fill(0);
    for (size_t i = 0; i != symbols.size(); ++i)
    {
        baseband[i * 10] = symbols[i];
    }

    for (auto& b : baseband)
    {
        b = rrc(b) * 25.0;
    }
}

/* 
 * Converts int16_t used by the modulator into uint8_t used by the PWM,
 * packed in a uint16_t array, as required by the STM32 DMA
 */
void baseband_to_stream(const std::array<audio_sample_t, M17_FRAME_SAMPLES>& baseband,
                        std::array<stream_sample_t, M17_FRAME_SAMPLES>& stream)
{
    for (int i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        // XXX: This code relies on the CPU performing arithmetic right shift
        stream[i] = static_cast<stream_sample_t> ((baseband[i] >> 8) + 128);
    }
}

/*
 * Converts 12 bit unsigned values packed into uint16_t into int16_t samples.
 */
void stream_to_audio(const std::array<stream_sample_t, M17_AUDIO_SIZE>& stream,
                     std::array<audio_sample_t, M17_AUDIO_SIZE>& audio)
{
    for (int i = 0; i < M17_AUDIO_SIZE; i++)
    {
        audio[i] = static_cast<audio_sample_t> (stream[i] - 2048);
    }
}

/*
 * Pushes the modulated baseband signal into the RTX sink, to transmit M17
 */
void OpMode_M17::output_baseband(std::array<audio_sample_t, M17_FRAME_SAMPLES>& baseband)
{
    std::array<stream_sample_t, M17_FRAME_SAMPLES> stream = { 0 };

    // TODO: Apply PWM compensation FIR filter

    baseband_to_stream(baseband, stream);
    streamId output_id = outputStream_start(SINK_RTX,
                                            PRIO_TX,
                                            stream.data(),
                                            M17_FRAME_SAMPLES,
                                            M17_RTX_SAMPLE_RATE);
}

/*
 * Modulates and one M17 frame
 */
void OpMode_M17::output_frame(std::array<uint8_t, 2> sync_word,
                  const std::array<int8_t, M17_CODEC2_SIZE>& frame)
{
    std::array<int8_t, M17_SYNCWORD_SYMBOLS> syncword_symbols = { 0 };
    std::array<int8_t, M17_PAYLOAD_SYMBOLS> payload_symbols = { 0 };
    std::array<int8_t, M17_FRAME_SYMBOLS> frame_symbols = { 0 };
    std::array<audio_sample_t, M17_FRAME_SAMPLES> frame_baseband = { 0 };

    bytes_to_symbols(sync_word, syncword_symbols);
    bits_to_symbols(frame, payload_symbols);
    auto fit = std::copy(syncword_symbols.begin(),
                         syncword_symbols.end(),
                         frame_symbols.begin());
    std::copy(payload_symbols.begin(),
              payload_symbols.end(), fit);
    symbols_to_baseband(frame_symbols, frame_baseband);
    output_baseband(frame_baseband);
}

/*
 * Generates and modulates the M17 preamble alone, used to start an M17
 * transmission
 */
void OpMode_M17::send_preamble()
{
    std::array<uint8_t, M17_FRAME_BYTES> preamble_bytes = { 0x77 };
    std::array<int8_t, M17_FRAME_SYMBOLS> preamble_symbols = { 0 };
    std::array<int16_t, M17_FRAME_SAMPLES> preamble_baseband = { 0 };

    // Preamble is simple... bytes -> symbols -> baseband.
    bytes_to_symbols(preamble_bytes, preamble_symbols);
    symbols_to_baseband(preamble_symbols, preamble_baseband);
    output_baseband(preamble_baseband);
}

constexpr std::array<uint8_t, 2> SYNC_WORD = {0x32, 0x43};
constexpr std::array<uint8_t, 2> LSF_SYNC_WORD = {0x55, 0xF7};
constexpr std::array<uint8_t, 2> DATA_SYNC_WORD = {0xFF, 0x5D};

/*
 * Sends the Link Setup Frame
 */
lsf_t OpMode_M17::send_lsf(const std::string& src, const std::string& dest)
{
    using namespace mobilinkd;

    lsf_t result;
    result.fill(0);
    
    M17Randomizer<M17_CODEC2_SIZE> randomizer;
    PolynomialInterleaver<45, 92, M17_CODEC2_SIZE> interleaver;
    CRC16<0x5935, 0xFFFF> crc;

    std::cerr << "Sending link setup." << std::endl;

    mobilinkd::LinkSetupFrame::call_t callsign;
    callsign.fill(0);
    std::copy(src.begin(), src.end(), callsign.begin());
    auto encoded_src = mobilinkd::LinkSetupFrame::encode_callsign(callsign);

     mobilinkd::LinkSetupFrame::encoded_call_t encoded_dest = {0xff,0xff,0xff,0xff,0xff,0xff};
     if (!dest.empty())
     {
        callsign.fill(0);
        std::copy(dest.begin(), dest.end(), callsign.begin());
        encoded_dest = mobilinkd::LinkSetupFrame::encode_callsign(callsign);
     }

    auto rit = std::copy(encoded_src.begin(), encoded_src.end(), result.begin());
    std::copy(encoded_dest.begin(), encoded_dest.end(), rit);
    result[12] = 0;
    result[13] = 5;

    crc.reset();
    for (size_t i = 0; i != 28; ++i)
    {
        crc(result[i]);
    }
    auto checksum = crc.get_bytes();
    result[28] = checksum[0];
    result[29] = checksum[1];

    std::array<uint8_t, 488> encoded;
    size_t index = 0;
    uint32_t memory = 0;
    for (auto b : result)
    {
        for (size_t i = 0; i != 8; ++i)
        {
            uint32_t x = (b & 0x80) >> 7;
            b <<= 1;
            memory = mobilinkd::update_memory<4>(memory, x);
            encoded[index++] = mobilinkd::convolve_bit(031, memory);
            encoded[index++] = mobilinkd::convolve_bit(027, memory);
        }
    }
    // Flush the encoder.
    for (size_t i = 0; i != 4; ++i)
    {
        memory = mobilinkd::update_memory<4>(memory, 0);
        encoded[index++] = mobilinkd::convolve_bit(031, memory);
        encoded[index++] = mobilinkd::convolve_bit(027, memory);
    }

    std::array<int8_t, M17_CODEC2_SIZE> punctured;
    auto size = puncture(encoded, punctured, P1);
    assert(size == M17_CODEC2_SIZE);

    interleaver.interleave(punctured);
    randomizer.randomize(punctured);
    output_frame(LSF_SYNC_WORD, punctured);

    return result;
}

/**
 * Encodes 2 frames of data.  Caller must ensure that the audio is
 * padded with 0s if the incoming data is incomplete.
 */
codec_frame_t OpMode_M17::encode(struct CODEC2* codec2, const audio_frame_t& audio)
{
    codec_frame_t result;
    codec2_encode(codec2, &result[0], const_cast<audio_sample_t*>(&audio[0]));
    codec2_encode(codec2, &result[8], const_cast<audio_sample_t*>(&audio[160]));
    return result;
}

data_frame_t OpMode_M17::make_data_frame(uint16_t frame_number, const codec_frame_t& payload)
{
    std::array<uint8_t, 20> data;   // FN, Audio, CRC = 2 + 16 + 2;
    data[0] = uint8_t((frame_number >> 8) & 0xFF);
    data[1] = uint8_t(frame_number & 0xFF);
    std::copy(payload.begin(), payload.end(), data.begin() + 2);

    mobilinkd::CRC16<0x5935, 0xFFFF> crc;
    crc.reset();
    for (size_t i = 0; i != 18; ++i) crc(data[i]);
    auto checksum = crc.get_bytes();
    data[18] = checksum[0];
    data[19] = checksum[1];

    std::array<uint8_t, 328> encoded;
    size_t index = 0;
    uint32_t memory = 0;
    for (auto b : data)
    {
        for (size_t i = 0; i != 8; ++i)
        {
            uint32_t x = (b & 0x80) >> 7;
            b <<= 1;
            memory = mobilinkd::update_memory<4>(memory, x);
            encoded[index++] = mobilinkd::convolve_bit(031, memory);
            encoded[index++] = mobilinkd::convolve_bit(027, memory);
        }
    }
    // Flush the encoder.
    for (size_t i = 0; i != 4; ++i)
    {
        memory = mobilinkd::update_memory<4>(memory, 0);
        encoded[index++] = mobilinkd::convolve_bit(031, memory);
        encoded[index++] = mobilinkd::convolve_bit(027, memory);
    }

    data_frame_t punctured;
    auto size = mobilinkd::puncture(encoded, punctured, mobilinkd::P2);
    assert(size == 272);
    return punctured;
}

/**
 * Encode each LSF segment into a Golay-encoded LICH segment bitstream.
 */
lich_segment_t OpMode_M17::make_lich_segment(std::array<uint8_t, 5> segment, uint8_t segment_number)
{
    lich_segment_t result;
    uint16_t tmp;
    uint32_t encoded;

    tmp = segment[0] << 4 | ((segment[1] >> 4) & 0x0F);
    encoded = mobilinkd::Golay24::encode24(tmp);
    for (size_t i = 0; i != 24; ++i)
    {
        result[i] = (encoded & (1 << 23)) != 0;
        encoded <<= 1;
    }

    tmp = ((segment[1] & 0x0F) << 8) | segment[2];
    encoded = mobilinkd::Golay24::encode24(tmp);
    for (size_t i = 24; i != 48; ++i)
    {
        result[i] = (encoded & (1 << 23)) != 0;
        encoded <<= 1;
    }

    tmp = segment[3] << 4 | ((segment[4] >> 4) & 0x0F);
    encoded = mobilinkd::Golay24::encode24(tmp);
    for (size_t i = 48; i != 72; ++i)
    {
        result[i] = (encoded & (1 << 23)) != 0;
        encoded <<= 1;
    }

    tmp = ((segment[4] & 0x0F) << 8) | (segment_number << 5);
    encoded = mobilinkd::Golay24::encode24(tmp);
    for (size_t i = 72; i != 96; ++i)
    {
        result[i] = (encoded & (1 << 23)) != 0;
        encoded <<= 1;
    }

    return result;
}

void OpMode_M17::send_audio_frame(const lich_segment_t& lich, const data_frame_t& data)
{
    using namespace mobilinkd;

    std::array<int8_t, M17_CODEC2_SIZE> temp;
    auto it = std::copy(lich.begin(), lich.end(), temp.begin());
    std::copy(data.begin(), data.end(), it);

    M17Randomizer<M17_CODEC2_SIZE> randomizer;
    PolynomialInterleaver<45, 92, M17_CODEC2_SIZE> interleaver;

    interleaver.interleave(temp);
    randomizer.randomize(temp);
    output_frame(DATA_SYNC_WORD, temp);
}

void OpMode_M17::m17_modulate(const lsf_t& lsf)
{
    using namespace mobilinkd;

    lich_t lich;
    for (size_t i = 0; i != lich.size(); ++i)
    {
        std::array<uint8_t, 5> segment;
        std::copy(lsf.begin() + i * 5, lsf.begin() + (i + 1) * 5, segment.begin());
        auto lich_segment = make_lich_segment(segment, i);
        std::copy(lich_segment.begin(), lich_segment.end(), lich[i].begin());
    }
    
    struct CODEC2* codec2 = ::codec2_create(CODEC2_MODE_3200);

    M17Randomizer<M17_AUDIO_SIZE> randomizer;
    CRC16<0x5935, 0xFFFF> crc;

    uint16_t frame_number = 0;
    uint8_t lich_segment = 0;

    // Get audio chunk from the microphone stream
    audio_frame_t audio;
    std::array<stream_sample_t, M17_AUDIO_SIZE> stream = 
        inputStream_getData<M17_AUDIO_SIZE>(input_id);
    stream_to_audio(stream, audio);

    // TODO: Apply DC removal filter

    auto data = make_data_frame(frame_number++, encode(codec2, audio));
    if (frame_number == 0x8000) frame_number = 0;
    send_audio_frame(lich[lich_segment++], data);
    if (lich_segment == lich.size()) lich_segment = 0;
    audio.fill(0);

    // TODO: Set frame counter MSB when last frame is sent
}

OpMode_M17::OpMode_M17() : enterRx(true),
                           input_id(0),
                           input_buffer({0})
{
}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    // When starting, close squelch and prepare for entering in RX mode.
    enterRx   = true;

    // Start sampling from the microphone
    input_id = inputStream_start(SOURCE_MIC,
                                 PRIO_TX,
                                 input_buffer.data(),
                                 M17_AUDIO_SIZE,
                                 BUF_CIRC_DOUBLE,
                                 M17_VOICE_SAMPLE_RATE);
}

void OpMode_M17::disable()
{
    // Terminate microphone sampling
    inputStream_stop(input_id);
    // Clean shutdown.
    audio_disableAmp();
    audio_disableMic();
    radio_disableRtx();
    enterRx   = false;
}

void OpMode_M17::update(rtxStatus_t *const status, const bool newCfg)
{
    (void) newCfg;

    // RX logic
    if(status->opStatus == RX)
    {
        // TODO: Implement M17 Rx
    }
    else if((status->opStatus == OFF) && enterRx)
    {
        radio_disableRtx();

        radio_enableRx();
        status->opStatus = RX;
        enterRx = false;
    }

    // TX logic
    if(platform_getPttStatus() && (status->opStatus != TX) &&
                                  (status->txDisable == 0))
    {
        // Entering Tx mode right now, setup transmission
        if(status->opStatus != TX)
        {
            audio_disableAmp();
            radio_disableRtx();

            audio_enableMic();
            radio_enableTx();

            status->opStatus = TX;

            char *source_address = status->source_address;
            char *destination_address = status->destination_address;

            // Send Link Setup Frame
            send_preamble();
            auto lsf = send_lsf(source_address, destination_address);
        } else {
            // Transmission is ongoing, just modulate
            m17_modulate(lsf);
        }
    }

    // PTT is off, transition to Rx state
    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        audio_disableMic();
        radio_disableRtx();

        status->opStatus = OFF;
        enterRx = true;
    }

    // Led control logic
    switch(status->opStatus)
    {
        case RX:
            // TODO: Implement Rx LEDs
            break;

        case TX:
            platform_ledOff(GREEN);
            platform_ledOn(RED);
            break;

        default:
            platform_ledOff(GREEN);
            platform_ledOff(RED);
            break;
    }
}
