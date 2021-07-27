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

#ifndef OPMODE_M17_H
#define OPMODE_M17_H

#include "OpMode.h"
#include <interfaces/audio_stream.h>
#include <hwconfig.h>
#include <M17Modulator.h>
#include <array>

#if defined(PLATFORM_LINUX)
#include <iostream>
#include <fstream>
#endif

#define M17_VOICE_SAMPLE_RATE 8000
#define M17_RTX_SAMPLE_RATE   48000

// FRAME = SYNCWORD + PAYLOAD
#define M17_AUDIO_SIZE        320
#define M17_CODEC2_SIZE       368
#define M17_FRAME_SAMPLES     1920
#define M17_SYNCWORD_SYMBOLS  8
#define M17_PAYLOAD_SYMBOLS   184
#define M17_FRAME_SYMBOLS     192
#define M17_FRAME_BYTES       48

using audio_sample_t = int16_t;
using lsf_t = std::array<uint8_t, 30>;
using lich_segment_t = std::array<uint8_t, 96>;
using lich_t = std::array<lich_segment_t, 6>;
using queue_t = mobilinkd::queue<audio_sample_t, 320>;
using audio_frame_t = std::array<audio_sample_t, 320>;
using codec_frame_t = std::array<uint8_t, 16>;
using data_frame_t = std::array<int8_t, 272>;

/**
 * Specialisation of the OpMode class for the management of M17 operating mode.
 */

class OpMode_M17 : public OpMode
{
public:

    /**
     * Constructor.
     */
    OpMode_M17();

    /**
     * Destructor.
     */
    ~OpMode_M17();

    /**
     * Enable the operating mode.
     *
     * Application must ensure this function is being called when entering the
     * new operating mode and always before the first call of "update".
     */
    virtual void enable() override;

    /**
     * Disable the operating mode. This function stops the DMA transfers
     * between the baseband, microphone and speakers. It also ensures that
     * the radio, the audio amplifier and the microphone are in OFF state.
     *
     * Application must ensure this function is being called when exiting the
     * current operating mode.
     */
    virtual void disable() override;

    /**
     * Update the internal FSM.
     * Application code has to call this function periodically, to ensure proper
     * functionality.
     *
     * @param status: pointer to the rtxStatus_t structure containing the current
     * RTX status. Internal FSM may change the current value of the opStatus flag.
     * @param newCfg: flag used inform the internal FSM that a new RTX configuration
     * has been applied.
     */
    virtual void update(rtxStatus_t *const status, const bool newCfg) override;

    /**
     * Get the mode identifier corresponding to the OpMode class.
     *
     * @return the corresponding flag from the opmode enum.
     */
    virtual opmode getID() override
    {
        return M17;
    }

private:

    bool enterRx;                  ///< Flag for RX management.
    streamId input_id = 0;         ///< Input audio stream identifier
    lsf_t lsf = { 0 };             ///< Link Setup Frame data structure
    lich_t lich = { 0 };           ///< Link Information Channel
    struct CODEC2* codec2 = { 0 }; ///< Codec2 data structure

    uint16_t frame_number = 0;     ///< Number of frame in the sequence
    uint8_t lich_segment = 0;      ///< LICH segment to be used

    // Input audio stream, PCM_16 8KHz, double buffered
    stream_sample_t *input = nullptr;
    std::array<int8_t, M17_FRAME_SYMBOLS> *symbols = nullptr;
    std::array<int16_t, M17_FRAME_SAMPLES> *baseband_a = nullptr;
    std::array<int16_t, M17_FRAME_SAMPLES> *baseband_b = nullptr;
    std::array<int16_t, M17_FRAME_SAMPLES> *active_baseband = nullptr;

#if defined(PLATFORM_MDUV3x0) | defined(PLATFORM_MD3x0)
    /*
     * Receives audio from the STM32 ADCs, connected to the radio's mic
     */
    std::array<audio_sample_t, M17_AUDIO_SIZE> *input_audio_stm32();

    /*
     * Pushes the modulated baseband signal into the RTX sink, to transmit M17
     */
    void output_baseband_stm32(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband);
#elif defined(PLATFORM_LINUX)
    std::ifstream infile;

    /*
     * Reads the input audio from a file on Linux
     */
    std::array<audio_sample_t, M17_AUDIO_SIZE> *input_audio_linux();

    /*
     * Pushes the output baseband on a file in Linux
     */
    void output_baseband_linux(std::array<audio_sample_t, M17_FRAME_SAMPLES> *baseband);
#else
#error M17 protocol is not supported on this platform
#endif


    /*
     * Modulates and one M17 frame
     */
    void output_frame(std::array<uint8_t, 2> sync_word,
                      const std::array<int8_t, M17_CODEC2_SIZE> *frame);

    /*
     * Generates and modulates the M17 preamble alone, used to start an M17
     * transmission
     */
    void send_preamble();

    /*
     * Sends the Link Setup Frame
     */
    lsf_t send_lsf(const std::string& src, const std::string& dest);

    /**
     * Encodes 2 frames of data.  Caller must ensure that the audio is
     * padded with 0s if the incoming data is incomplete.
     */
    codec_frame_t encode(struct CODEC2* codec2, audio_frame_t& audio);

    /**
     * Encodes a single M17 data frame.
     */
    data_frame_t make_data_frame(uint16_t frame_number,
                                             const codec_frame_t& payload);

    /**
     * Encode each LSF segment into a Golay-encoded LICH segment bitstream.
     */
    lich_segment_t make_lich_segment(std::array<uint8_t, 5> segment,
                                                 uint8_t segment_number);

    /**
     * Modulates a single audio frame.
     */
    void send_audio_frame(const lich_segment_t& lich,
                                      const data_frame_t& data);

    /*
     * Starts the modulation of a single audio frame captured from the mic.
     */
    void m17_modulate(bool last_frame);
};

#endif /* OPMODE_M17_H */
