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
#include "M17ConvolutionalEncoder.h"
#include "M17LinkSetupFrame.h"
#include "M17StreamFrame.h"
#include "M17Modulator.h"

namespace M17
{

/**
 * M17 transmitter.
 */
class M17Transmitter
{
public:

    /**
     * Constructor.
     *
     * @param modulator: reference to M17 4FSK modulator driver.
     */
    M17Transmitter(M17Modulator& modulator);

    /**
     * Destructor.
     */
    ~M17Transmitter();

    /**
     * Start a new data stream with broadcast destination callsign.
     *
     * @param src: source callsign.
     */
    void start(const std::string& src);

    /**
     * Start a new data stream with given destination callsign. If destination
     * callsing is empty, the stream falls back to broadcast transmission.
     *
     * @param src: source callsign.
     * @param dst: destination callsign.
     */
    void start(const std::string& src, const std::string& dst);

    /**
     * Send a block of data.
     *
     * @param payload: payload data.
     * @param isLast: if true, current frame is marked as the last one to be
     * transmitted.
     */
    void send(const payload_t& payload, const bool isLast = false);

private:

    M17ConvolutionalEncoder      encoder;    ///< Convolutional encoder.
    M17LinkSetupFrame                lsf;    ///< Link Setup Frame handler.
    M17StreamFrame             dataFrame;    ///< Data frame Handler.
    M17Modulator&              modulator;    ///< 4FSK modulator.
    std::array< lich_t, 6 > lichSegments;    ///< Encoded LSF chunks for LICH generation.
    uint8_t                  currentLich;    ///< Index of current LSF chunk.
    uint16_t                 frameNumber;    ///< Current frame number.
};

} /* M17 */

#endif /* M17TRANSMITTER_H */
