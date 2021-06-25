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
#include <hwconfig.h>
#include <OpMode_M17.h>
#include <M17Modulator.h>
#include <LinkSetupFrame.h>
#include <cstring>
#include <rtx.h>

#include <iostream>
#include <fstream>

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
template <typename T, size_t N, size_t M>
void bits_to_symbols(const std::array<T, N> *bits,
                     std::array<int8_t, M> *symbols,
                     std::size_t offset = 0)
{
    // Check that destination array has enough space
    static_assert(M >= N / 2);

    size_t index = 0;
    for (size_t i = 0; i != N; i += 2)
    {
        (*symbols)[offset + index++] = bits_to_symbol(((*bits)[i] << 1) | (*bits)[i + 1]);
    }
}

/*
 * Converts bytes of binary data into encoding symbols
 */
template <typename T, size_t N, size_t M>
void bytes_to_symbols(const std::array<T, N> *bytes,
                      std::array<int8_t, M> *symbols)
{
    // Check that destination array has enough space
    static_assert(M >= N * 4);

    size_t index = 0;
    for (auto b : *bytes)
    {
        for (size_t i = 0; i != 4; ++i)
        {
            (*symbols)[index++] = bits_to_symbol(b >> 6);
            b <<= 2;
        }
    }
}

/*
 * Converts symbols into the modulated waveform of the baseband
 */
void symbols_to_baseband(const std::array<int8_t, M17_FRAME_SYMBOLS> *symbols,
                         std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
{
    using namespace mobilinkd;

    static BaseFirFilter<double, std::tuple_size<decltype(rrc_taps)>::value> rrc = makeFirFilter(rrc_taps);

    baseband->fill(0);
    for (size_t i = 0; i != symbols->size(); ++i)
    {
        (*baseband)[i * 10] = (*symbols)[i];
    }

    for (auto& b : *baseband)
    {
        b = rrc(b) * 7168.0;
    }
}

/*
 * Converts int16_t used by the modulator into uint8_t used by the PWM,
 * packed in a uint16_t array, as required by the STM32 DMA
 */
void baseband_to_pwm_stm32(const std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
{
    // TODO: convert from int16_t to uint8_t (packed into uint16_t)
    //for (int i = 0; i < M17_FRAME_SAMPLES; i++)
    //{
    //    // XXX: This code relies on the CPU performing arithmetic right shift
    //    (*stream)[i] = static_cast<stream_sample_t> (((*baseband)[i] >> 8) + 128);
    //}
}

/*
 * Converts 12 bit unsigned values packed into uint16_t into int16_t samples,
 * perform in-place conversion to save space.
 */
void stream_to_audio(std::array<audio_sample_t, M17_AUDIO_SIZE> *audio)
{
    for (int i = 0; i < M17_AUDIO_SIZE; i++)
    {
        (*audio)[i] = (*audio)[i] << 3;
    }
}

/*
 * Pushes the modulated baseband signal into the RTX sink, to transmit M17
 */
void OpMode_M17::output_baseband_stm32(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
{
    // TODO: Apply PWM compensation FIR filter
    // TODO: Change phase
    // TODO: in-place cast to uint8_t packed into uint16_t for PWM

    baseband_to_pwm_stm32(baseband);
    streamId output_id = outputStream_start(SINK_RTX,
                                            PRIO_TX,
                                            (stream_sample_t *)baseband->data(),
                                            M17_FRAME_SAMPLES,
                                            M17_RTX_SAMPLE_RATE);
}

#if defined(PLATFORM_LINUX)
// Test M17 on linux
std::array<audio_sample_t, M17_AUDIO_SIZE> *OpMode_M17::input_audio_linux()
{
    if (!infile)
        exit(0);
    std::array<audio_sample_t, M17_AUDIO_SIZE> *audio = new std::array<audio_sample_t, M17_AUDIO_SIZE>;
    infile.read((char *)audio->data(), M17_AUDIO_SIZE * sizeof(int16_t));
    return audio;
}

void OpMode_M17::output_baseband_linux(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
{
    FILE *outfile = fopen ("./m17_output.raw", "ab");
    for(auto s : *baseband)
    {
        fwrite(&s, sizeof(s), 1, outfile);
    }
    fclose(outfile);
}
#endif

/*
 * Modulates and one M17 frame
 */
void OpMode_M17::output_frame(std::array<uint8_t, 2> sync_word,
                  const std::array<int8_t, M17_CODEC2_SIZE> *frame)
{
    bytes_to_symbols(&sync_word, symbols);
    bits_to_symbols(frame, symbols, M17_SYNCWORD_SYMBOLS);

    symbols_to_baseband(symbols, baseband);

#if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)
    output_baseband_stm32(baseband);
#elif defined(PLATFORM_LINUX)
    output_baseband_linux(baseband);
#else
#error M17 protocol is not supported on this platform
#endif
}

/*
 * Generates and modulates the M17 preamble alone, used to start an M17
 * transmission
 */
void OpMode_M17::send_preamble()
{
    std::array<uint8_t, M17_FRAME_BYTES> preamble_bytes = { 0 };
    preamble_bytes.fill(0x77);

    // Preamble is simple... bytes -> symbols -> baseband.
    bytes_to_symbols(&preamble_bytes, symbols);
    symbols_to_baseband(symbols, baseband);
#if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)
    output_baseband_stm32(baseband);
#elif defined(PLATFORM_LINUX)
    output_baseband_linux(baseband);
#else
#error M17 protocol is not supported on this platform
#endif
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

    // TODO: statically allocate these into heap and create them at init, deallocate at stop
    M17Randomizer<M17_CODEC2_SIZE> randomizer;
    PolynomialInterleaver<45, 92, M17_CODEC2_SIZE> interleaver;
    CRC16<0x5935, 0xFFFF> crc;

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

    auto rit = std::copy(encoded_dest.begin(), encoded_dest.end(), result.begin());
    std::copy(encoded_src.begin(), encoded_src.end(), rit);
    result[12] = 5;
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
    output_frame(LSF_SYNC_WORD, &punctured);

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
    output_frame(DATA_SYNC_WORD, &temp);
}

void OpMode_M17::m17_modulate(bool last_frame)
{
    using namespace mobilinkd;

#if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)
    // Get audio chunk from the microphone stream
    std::array<stream_sample_t, M17_AUDIO_SIZE> *stream =
        inputStream_getData<M17_AUDIO_SIZE>(input_id);
    std::array<audio_sample_t, M17_AUDIO_SIZE> *audio =
        reinterpret_cast<std::array<audio_sample_t, M17_AUDIO_SIZE>*>(stream);
    stream_to_audio(audio);
#elif defined(PLATFORM_LINUX)
    std::array<audio_sample_t, M17_AUDIO_SIZE> *audio = input_audio_linux();
#else
#error M17 protocol is not supported on this platform
#endif

    // TODO: Apply DC removal filter

    if (!last_frame)
    {
        auto data = make_data_frame(frame_number++, encode(codec2, *audio));
        if (frame_number == 0x8000) frame_number = 0;
        send_audio_frame(lich[lich_segment++], data);
        if (lich_segment == lich.size()) lich_segment = 0;
    }
    else
    {
        auto data = make_data_frame(frame_number | 0x8000, encode(codec2, *audio));
        send_audio_frame(lich[lich_segment], data);
    }

#if defined(PLATFORM_LINUX)
    // Test M17 on linux
    delete audio;
#endif

}

OpMode_M17::OpMode_M17() : enterRx(true),
                           input_id(0),
                           lsf({0}),
                           input(nullptr),
                           symbols(nullptr),
                           baseband(nullptr)
{
}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    // Allocate arrays for M17 processing
    input = (stream_sample_t *) malloc(2 * M17_AUDIO_SIZE * sizeof(stream_sample_t));
    memset(input, 0x00, 2 * M17_AUDIO_SIZE * sizeof(stream_sample_t));
    symbols = new std::array<int8_t, M17_FRAME_SYMBOLS>;
    symbols->fill({ 0 });
    baseband = new std::array<int16_t, M17_FRAME_SAMPLES>;
    baseband->fill({ 0 });

    // When starting, close squelch and prepare for entering in RX mode.
    enterRx   = true;

    // Start sampling from the microphone
    input_id = inputStream_start(SOURCE_MIC,
                                 PRIO_TX,
                                 input,
                                 M17_AUDIO_SIZE,
                                 BUF_CIRC_DOUBLE,
                                 M17_VOICE_SAMPLE_RATE);

#if defined(PLATFORM_LINUX)
    // Test M17 on linux
    infile.open ("./m17_input.raw", std::ios::in | std::ios::binary);
    FILE *outfile = fopen ("./m17_output.raw", "wb");
    fclose(outfile);
#endif
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

    // Deallocate arrays to save space
    free(input);
    delete symbols;
    delete baseband;

#if defined(PLATFORM_LINUX)
    // Test M17 on linux
    infile.close();
#endif
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
    if(platform_getPttStatus() && (status->txDisable == 0))
    {
        // Entering Tx mode right now, setup transmission
        if(status->opStatus != TX)
        {
            audio_disableAmp();
            radio_disableRtx();

            audio_enableMic();
            radio_enableTx();

            status->opStatus = TX;

            //char *source_address = status->source_address;
            std::string source_address("IU2KIN");
            //char *destination_address = status->destination_address;
            std::string destination_address("\0\0\0\0\0\0");

            // Send Link Setup Frame
            send_preamble();
            lsf = send_lsf(source_address, destination_address);

            // Setup LICH for use in following modulation
            for (size_t i = 0; i != lich.size(); ++i)
            {
                std::array<uint8_t, 5> segment;
                std::copy(lsf.begin() + i * 5, lsf.begin() + (i + 1) * 5, segment.begin());
                auto lich_segment = make_lich_segment(segment, i);
                std::copy(lich_segment.begin(), lich_segment.end(), lich[i].begin());
            }

            // Maybe allocate this during enable and deallocate during disable?
            codec2 = ::codec2_create(CODEC2_MODE_3200);
        } else {
            // Transmission is ongoing, just modulate
            m17_modulate(false);
        }
    }

    // PTT is off, transition to Rx state
    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        // Send last audio frame
        m17_modulate(true);

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
