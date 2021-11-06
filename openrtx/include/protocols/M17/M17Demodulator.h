/***************************************************************************
 *   Copyright (C) 2021 by Federico Amedeo Izzo IU2NUO,                    *
 *                         Niccolò Izzo IU2KIN                             *
 *                         Wojciech Kaczmarski SP5WWP                      *
 *                         Frederik Saraci IU2NRO                          *
 *                         Silvano Seva IU2KWO                             *
 *                                                                         *
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

#ifndef M17_DEMODULATOR_H
#define M17_DEMODULATOR_H

#ifndef __cplusplus
#error This header is C++ only!
#endif

namespace M17
{

class M17Demodulator
{
public:

    /**
     * Constructor.
     */
    M17Demodulator();

    /**
     * Destructor.
     */
    ~M17Demodulator();

    /**
     * Allocate buffers for baseband signal sampling and initialise demodulator.
     */
    void init();

    /**
     * Shutdown modulator and deallocate data buffers.
     */
    void terminate();

    /**
     * Starts the sampling of the baseband signal in a double buffer.
     */
    void startBasebandSampling();

    /**
     * Stops the sampling of the baseband signal in a double buffer.
     */
    void stopBasebandSampling();

    /**
     * Returns the next frame decoded from the baseband signal
     */
    const std::array<uint8_t, 48>& nextFrame();

};

private:

    /**
     * M17 syncword symbols after RRC sampled at 5 samples per symbol;
     */
    uint8_t syncword[] = { };

    using dataBuffer_t = std::array< int16_t, M17_FRAME_SAMPLES >;
    using dataFrame_t =  std::array< int8_t, M17_FRAME_SYMBOLS >;

    int16_t      *baseband_buffer; ///< Buffer for baseband audio handling.
    dataBuffer_t *activeBuffer;    ///< Half baseband buffer, in reception.
    dataBuffer_t *idleBuffer;      ///< Half baseband buffer, to be processed.
    dataFrame_t  *activeFrame;     ///< Half frame, in demodulation.
    dataFrame_t  *idleFrame;       ///< Half frame, free to be processed.

    /**
     * Finds the index of the next syncword in the baseband stream.
     *
     * @param baseband: buffer containing the sampled baseband signal
     * @param offset: offset of the buffer after which syncword are searched
     * @return uint16_t index of the first syncword in the buffer after the offset
     */ uint16_t nextSyncWord(int16_t *baseband);

#endif /* M17_DEMODULATOR_H */
