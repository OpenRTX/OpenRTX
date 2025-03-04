/***************************************************************************
 *   Copyright (C) 2021 - 2023 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
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
 *                                                                         *
 *   (2025) Modified by KD0OSS for FM mode on Module17                     *
 ***************************************************************************/

#ifndef OPMODE_FM_H
#define OPMODE_FM_H

#include <stdint.h>
#include <cstddef>
#include <memory>
#include <audio_path.h>
#include "OpMode.hpp"
#include <audio_path.h>
#include <audio_stream.h>
#if defined(PLATFORM_MOD17)
#include <FM.h>
#include "FMCTCSSTX.h"
#endif

/**
 * Specialisation of the OpMode class for the management of analog FM operating
 * mode.
 */

class OpMode_FM : public OpMode
{
public:

    /**
     * Constructor.
     */
    OpMode_FM();

    /**
     * Destructor.
     */
    ~OpMode_FM();

    /**
     * Enable the operating mode.
     *
     * Application must ensure this function is being called when entering the
     * new operating mode and always before the first call of "update".
     */
    virtual void enable() override;

    /**
     * Disable the operating mode. This function ensures that, after being
     * called, the radio, the audio amplifier and the microphone are in OFF state.
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
        return OPMODE_FM;
    }

    #if defined(PLATFORM_MOD17)
    void reset();
    void startBasebandSampling(const bool isTx);
    void stopBasebandSampling();
    #else
    /**
     * Check if RX squelch is open.
     *
     * @return true if RX squelch is open.
     */
    virtual bool rxSquelchOpen() override;
    #endif

private:
    #if defined(PLATFORM_MOD17)
    std::unique_ptr< int16_t[] >   baseband_buffer; ///< Buffer for baseband audio handling.
    //  std::unique_ptr< int16_t[] >   baseband_txbuffer; ///< Buffer for baseband audio handling.
    pathId           basebandPath;
    streamId         basebandId;      ///< Id of the baseband input stream.
    stream_sample_t *idleBuffer;      ///< Half baseband buffer, free for processing.
    CFM              fm;
    CFMCTCSSTX       m_ctcssTX;
    uint8_t          m_ctcssRX_freq;
    uint8_t          m_ctcssTX_freq;
    uint8_t          m_ctcssRX_thrshLo;
    uint8_t          m_ctcssRX_thrshHi;
    uint8_t          m_noiseSq_thrshLo;
    uint8_t          m_noiseSq_thrshHi;
    bool             m_noiseSq_on;
    uint8_t          m_ctcssTX_level;
    uint8_t          m_rxLevel;
    uint8_t          m_txLevel;
    uint16_t         m_preRxLevel;
    uint8_t          m_maxDev;
    uint8_t          m_accessMode;
    #endif

    bool   rfSqlOpen;   ///< Flag for RF squelch status (analog squelch).
    bool   sqlOpen;     ///< Flag for squelch status.
    bool   enterRx;     ///< Flag for RX management.
    pathId rxAudioPath; ///< Audio path ID for RX
    pathId txAudioPath; ///< Audio path ID for TX
};

#endif /* OPMODE_FM_H */
