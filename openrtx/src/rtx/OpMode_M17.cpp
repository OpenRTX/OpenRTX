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
#include <cstring>
#include <rtx.h>
#include <dsp.h>
#include <stdio.h>

#include <toneGenerator_MDx.h>

// Enable bitstream output on serial console
//#define M17_DEBUG

// Generated using scikit-commpy
const auto rrc_taps = std::experimental::make_array<float>(
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

const std::array<uint8_t, 46> mobilinkd::detail::DC =
{
    0xd6, 0xb5, 0xe2, 0x30, 0x82, 0xFF, 0x84, 0x62,
    0xba, 0x4e, 0x96, 0x90, 0xd8, 0x98, 0xdd, 0x5d,
    0x0c, 0xc8, 0x52, 0x43, 0x91, 0x1d, 0xf8, 0x6e,
    0x68, 0x2F, 0x35, 0xda, 0x14, 0xea, 0xcd, 0x76,
    0x19, 0x8d, 0xd5, 0x80, 0xd1, 0x33, 0x87, 0x13,
    0x57, 0x18, 0x2d, 0x29, 0x78, 0xc3
};

/*
 * Converts bits of binary data into an encoding symbol
 */
static constexpr int8_t bits_to_symbol(uint8_t bits)
{
    switch (bits)
    {
        case 0: return 1;
        case 1: return 3;
        case 2: return -1;
        case 3: return -3;
    }
    return 0;
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
        symbols->at(offset + index++) = bits_to_symbol((bits->at(i) << 1) |
                                                        bits->at(i + 1));
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
            symbols->at(index++) = bits_to_symbol(b >> 6);
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

    static BaseFirFilter<float, std::tuple_size<decltype(rrc_taps)>::value> rrc = makeFirFilter(rrc_taps);

    baseband->fill(0);
    for (size_t i = 0; i != symbols->size(); ++i)
    {
        baseband->at(i * 10) = symbols->at(i);
    }

    for (auto& b : *baseband)
    {
        b = rrc(b) * 7168.0;
    }
}

#if defined(PLATFORM_MDUV3x0) || defined(PLATFORM_MD3x0)
/*
 * Converts int16_t used by the modulator into uint8_t used by the PWM,
 * packed in a uint16_t array, as required by the STM32 DMA
 */
void baseband_to_pwm_stm32(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
{
    for (int i = 0; i < M17_FRAME_SAMPLES; i++)
    {
        int32_t pos_sample = baseband->at(i) + 32768;
        uint16_t shifted_sample = pos_sample >> 8;
        baseband->at(i) = shifted_sample;
    }
}
#endif
//
// /*
//  * Converts 12-bit unsigned values packed into uint16_t into int16_t samples,
//  * perform in-place conversion to save space.
//  */
// void adc_to_audio_stm32(std::array<audio_sample_t, M17_AUDIO_SIZE> *audio)
// {
//     for (int i = 0; i < M17_AUDIO_SIZE; i++)
//     {
//         (*audio)[i] = (*audio)[i] << 3;
//     }
// }
//
// /*
//  * Receives audio from the STM32 ADCs, connected to the radio's mic
//  */
// std::array<audio_sample_t, M17_AUDIO_SIZE> *OpMode_M17::input_audio_stm32()
// {
//     // Get audio chunk from the microphone stream
//     std::array<stream_sample_t, M17_AUDIO_SIZE> *stream =
//         inputStream_getData<M17_AUDIO_SIZE>(input_id);
//     std::array<audio_sample_t, M17_AUDIO_SIZE> *audio =
//         reinterpret_cast<std::array<audio_sample_t, M17_AUDIO_SIZE>*>(stream);
//     // Convert 12-bit unsigned values into 16-bit signed
//     adc_to_audio_stm32(audio);
//     // Apply DC removal filter
//     dsp_dcRemoval(audio->data(), audio->size());
//
//     return audio;
// }
//
// /*
//  * Pushes the modulated baseband signal into the RTX sink, to transmit M17
//  */
// void OpMode_M17::output_baseband_stm32(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband)
// {
//     // Apply PWM compensation FIR filter
//     dsp_pwmCompensate(baseband->data(), baseband->size());
//     // Invert phase
//     dsp_invertPhase(baseband->data(), baseband->size());
//     // In-place cast to uint8_t packed into uint16_t for PWM
//     baseband_to_pwm_stm32(baseband);
//     std::array<stream_sample_t, M17_FRAME_SAMPLES> *stream =
//         reinterpret_cast<std::array<stream_sample_t, M17_FRAME_SAMPLES>*>(baseband);
//
//     streamId output_id = outputStream_start(SINK_RTX,
//                                             PRIO_TX,
//                                             (stream_sample_t *)stream->data(),
//                                             M17_FRAME_SAMPLES,
//                                             M17_RTX_SAMPLE_RATE);
// }

constexpr std::array<uint8_t, 2> SYNC_WORD      = {0x32, 0x43};
constexpr std::array<uint8_t, 2> LSF_SYNC_WORD  = {0x55, 0xF7};
constexpr std::array<uint8_t, 2> DATA_SYNC_WORD = {0xFF, 0x5D};

OpMode_M17::OpMode_M17() : enterRx(true),
                           input(nullptr),
                           symbols(nullptr),
                           active_outBuffer(nullptr),
                           idle_outBuffer(nullptr)
{
    lsf.clear();
    dataFrame.clear();
}

OpMode_M17::~OpMode_M17()
{
}

void OpMode_M17::enable()
{
    // Allocate codec2 encoder
    codec2 = ::codec2_create(CODEC2_MODE_3200);

    // Allocate arrays for M17 processing
    symbols = new std::array<int8_t, M17_FRAME_SYMBOLS>;
    symbols->fill(0x00);

    active_outBuffer = new std::array<int16_t, M17_FRAME_SAMPLES>;
    active_outBuffer->fill(0x00);

    idle_outBuffer = new std::array<int16_t, M17_FRAME_SAMPLES>;
    idle_outBuffer->fill(0x00);

    // When starting, close squelch and prepare for entering in RX mode.
    enterRx   = true;

    // Start sampling from the microphone
    input = new stream_sample_t[2 * M17_AUDIO_SIZE];
    memset(input, 0x00, 2 * M17_AUDIO_SIZE * sizeof(stream_sample_t));

    input_id = inputStream_start(SOURCE_MIC,
                                 PRIO_TX,
                                 input,
                                 2 * M17_AUDIO_SIZE,
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

    // Deallocate arrays to save space
    delete symbols;
    delete active_outBuffer;
    delete idle_outBuffer;
    delete[] input;
    codec2_destroy(codec2);
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

            // TODO: Allow destinations different than broadcast
            std::string source_address(status->source_address);
            std::string destination_address;

            // Start sending the preamble
            send_preamble();

            // Assemble and send the Link Setup Frame
            send_lsf(source_address, destination_address);

            // Setup LICH for use in following modulation
            for(uint8_t i = 0; i < 6; i++) make_lich_segment(i);
        }
        else
        {
            // Transmission is ongoing, just modulate
            send_dataFrame(false);
        }
    }

    // PTT is off, transition to Rx state
    if(!platform_getPttStatus() && (status->opStatus == TX))
    {
        // Send last audio frame
        send_dataFrame(true);

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

void OpMode_M17::send_preamble()
{
    std::array<uint8_t, M17_FRAME_BYTES> preamble_bytes;
    preamble_bytes.fill(0x77);

    // Preamble is simple... bytes -> symbols -> baseband.
    bytes_to_symbols(&preamble_bytes, symbols);
    symbols_to_baseband(symbols, idle_outBuffer);
    output_baseband();
}

void OpMode_M17::send_lsf(const std::string& src, const std::string& dest)
{
    lsf.clear();
    lsf.setSource(src);
    if(!dest.empty()) lsf.setDestination(dest);

    streamType_t type;
    type.stream   = 1;    // Stream
    type.dataType = 2;    // Voice data
    type.CAN      = 0xA;  // Channel access number

    lsf.setType(type);
    lsf.updateCrc();

    mobilinkd::M17Randomizer<M17_CODEC2_SIZE> randomizer;
    mobilinkd::PolynomialInterleaver<45, 92, M17_CODEC2_SIZE> interleaver;

    auto *ptr = reinterpret_cast< uint8_t *>(&lsf.getData());
    std::array<uint8_t, 488> encoded;
    size_t index = 0;
    uint32_t memory = 0;

    for(size_t i = 0; i < sizeof(lsf_t); i++)
    {
        uint8_t b = ptr[i];
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
    auto size = mobilinkd::puncture(encoded, punctured, mobilinkd::P1);
    //assert(size == M17_CODEC2_SIZE);

    interleaver.interleave(punctured);
    randomizer.randomize(punctured);
    output_frame(LSF_SYNC_WORD, &punctured);
}

void OpMode_M17::send_dataFrame(bool last_frame)
{
    dataFrame.clear();
    dataFrame.setFrameNumber(frame_number);
    frame_number = (frame_number + 1) & 0x07FF;

    if(last_frame) dataFrame.lastFrame();

// #if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)
//     std::array<audio_sample_t, M17_AUDIO_SIZE> *audio = input_audio_stm32();
// #elif defined(PLATFORM_LINUX)
//     std::array<audio_sample_t, M17_AUDIO_SIZE> *audio = input_audio_linux();
// #else
// #error M17 protocol is not supported on this platform
// #endif

//     codec2_encode(codec2, &dataFrame.payload()[0], &(audio.data()[0]));
//     codec2_encode(codec2, &dataFrame.payload()[8], &(audio.data()[160]));

    for(size_t i = 0; i < dataFrame.payload().size(); i++)
    {
        dataFrame.payload()[i] = 'A' + i;
    }

    auto *ptr = reinterpret_cast< uint8_t *>(&dataFrame.getData());
    std::array<uint8_t, 296> encoded;
    size_t index    = 0;
    uint32_t memory = 0;

    for(size_t i = 0; i < sizeof(dataFrame_t); i++)
    {
        uint8_t b = ptr[i];
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
//     assert(size == 272);

    // Add LICH segment to coded data and send
    std::array<int8_t, M17_CODEC2_SIZE> temp;
    auto it = std::copy(lich[lich_segment].begin(), lich[lich_segment].end(),
                        temp.begin());
    std::copy(punctured.begin(), punctured.end(), it);

    mobilinkd::M17Randomizer<M17_CODEC2_SIZE> randomizer;
    mobilinkd::PolynomialInterleaver<45, 92, M17_CODEC2_SIZE> interleaver;

    interleaver.interleave(temp);
    randomizer.randomize(temp);
    output_frame(DATA_SYNC_WORD, &temp);
}

void OpMode_M17::output_frame(std::array<uint8_t, 2> sync_word,
                  const std::array<int8_t, M17_CODEC2_SIZE> *frame)
{

    bytes_to_symbols(&sync_word, symbols);
    bits_to_symbols(frame, symbols, M17_SYNCWORD_SYMBOLS);

    symbols_to_baseband(symbols, idle_outBuffer);
    output_baseband();
}

void OpMode_M17::output_baseband()
{
//     #if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)

    // Apply PWM compensation FIR filter
    dsp_pwmCompensate(idle_outBuffer->data(), idle_outBuffer->size());

    // Invert phase
    dsp_invertPhase(idle_outBuffer->data(), idle_outBuffer->size());
    std::swap(idle_outBuffer, active_outBuffer);

    outputStream_start(SINK_RTX, PRIO_TX,
                       reinterpret_cast< stream_sample_t *>(active_outBuffer->data()),
                       active_outBuffer->size(), M17_RTX_SAMPLE_RATE);
}

void OpMode_M17::make_lich_segment(uint8_t segment_number)
{
    lich_segment_t& result = lich[segment_number];
    auto segment = lsf.getSegment(segment_number).data;

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
}
