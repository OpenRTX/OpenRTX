/***************************************************************************
 *   Copyright (C) 2022 - 2023 by Federico Amedeo Izzo IU2NUO,             *
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
 *   Modified by KD0OSS for P25 on Module17/OpenRTX                        *
 ***************************************************************************/
#ifdef CONFIG_P25
#ifndef OPMODE_P25_H
#define OPMODE_P25_H

#include <P25/P25_IO.hpp>
#include <audio_path.h>
#include "OpMode.hpp"
#include <string>

/**
 * Specialisation of the OpMode class for the management of DSTAR operating mode.
 */
class OpMode_P25 : public OpMode
{
public:

    /**
     * Constructor.
     */
    OpMode_P25();

    /**
     * Destructor.
     */
    ~OpMode_P25();

    virtual void reset();

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
        return OPMODE_P25;
    }

    /**
     * Check if RX squelch is open.
     *
     * @return true if RX squelch is open.
     */
    virtual bool rxSquelchOpen() override
    {
        return dataValid;
    }

private:

    /**
     * Function handling the OFF operating state.
     *
     * @param status: pointer to the rtxStatus_t structure containing the
     * current RTX status.
     */
    void offState(rtxStatus_t *const status);

    /**
     * Function handling the RX operating state.
     *
     * @param status: pointer to the rtxStatus_t structure containing the
     * current RTX status.
     */
    void rxState(rtxStatus_t *const status);

    /**
     * Function handling the TX operating state.
     *
     * @param status: pointer to the rtxStatus_t structure containing the
     * current RTX status.
     */
    void txState(rtxStatus_t *const status);

    uint32_t m_dstid;                      ///< P25 Destination ID (DMR ID/TG).
    uint32_t m_srcid;                      ///< P25 Source Id (DMR ID).
    bool     startRx;                      ///< Flag for RX management.
    bool     startTx;                      ///< Flag for TX management.
    bool     dataValid;                    ///< Demodulated data is valid
    bool     invertTxPhase;                ///< TX signal phase inversion setting.
    bool     invertRxPhase;                ///< RX signal phase inversion setting.
    pathId   rxAudioPath;                  ///< Audio path ID for RX.
    pathId   txAudioPath;                  ///< Audio path ID for TX.
    P25_IO   p25_io;                       ///< P25 IO control.
};

#endif /* OPMODE_P25_H */
#endif // if P25
