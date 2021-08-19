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

#ifndef M17TRANSMITTER_H
#define M17TRANSMITTER_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

#include <string>
#include <array>
#include "M17/M17ConvolutionalEncoder.h"
#include "M17LinkSetupFrame.h"
#include "M17Frame.h"

class M17Transmitter
{
public:

    /**
     *
     */
    M17Transmitter();

    /**
     *
     */
    ~M17Transmitter();

    /**
     *
     */
    void start(const std::string& src, const std::string& dst);

    /**
     *
     */
    void send(const payload_t& payload);

    /**
     *
     */
    void stop(const payload_t& payload = {0});

private:

    static constexpr size_t M17_RTX_SAMPLE_RATE = 48000;
    static constexpr size_t M17_FRAME_SAMPLES   = 1920;
    static constexpr size_t M17_FRAME_SYMBOLS   = 192;
    static constexpr size_t M17_FRAME_SIZE      = 46;

    /**
     *
     */
    void sendPreamble();

    /**
     *
     */
    void sendData(const std::array< uint8_t, 2 >& sync,
                  const std::array< uint8_t, M17_FRAME_SIZE >& frame);

    /**
     *
     */
    void encodeSymbols(const std::array< uint8_t, 2 >& sync,
                       const std::array< uint8_t, M17_FRAME_SIZE >& frame);

    /**
     *
     */
    void generateBaseband();

    M17ConvolutionalEncoder      encoder;
    M17LinkSetupFrame                lsf;
    M17Frame                   dataFrame;
    std::array< lich_t, 6 > lichSegments;
    uint8_t                  currentLich;
    uint16_t                 frameNumber;
    std::array< int16_t, M17_FRAME_SYMBOLS > symbols;
    std::array< int16_t, M17_FRAME_SAMPLES > active_outBuffer;
    std::array< int16_t, M17_FRAME_SAMPLES > idle_outBuffer;
};

#endif /* M17TRANSMITTER_H */
