/***************************************************************************
 *   Copyright (C) 2021 - 2025 by Federico Amedeo Izzo IU2NUO,             *
 *                                Niccolò Izzo IU2KIN                      *
 *                                Frederik Saraci IU2NRO                   *
 *                                Silvano Seva IU2KWO                      *
 *                                and the OpenRTX contributors             *
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

#include <M17/M17FrameDecoder.hpp>
#include <M17/M17FrameEncoder.hpp>
#include <M17/M17Demodulator.hpp>
#include <M17/M17Modulator.hpp>
#include <M17/m17_constants.h>
#include <audio_path.h>
#include <vector>
#include "OpMode.hpp"
#include <M17/M17LinkSetupFrame.hpp>

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
        return OPMODE_M17;
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

    /**
     * Compare two callsigns in plain text form.
     * The comparison does not take into account the country prefixes (strips
     * the '/' and whatever is in front from all callsigns). It does take into
     * account the dash and whatever is after it. In case the incoming callsign
     * is "ALL" the function returns true.
     *
     * \param localCs plain text callsign from the user
     * \param incomingCs plain text destination callsign
     * \return true if local an incoming callsigns match.
     */
    bool compareCallsigns(const std::string& localCs, const std::string& incomingCs);

    /**
    * Prepare a meta text block for transmission
    *
    * @param text       The complete meta text to send
    * @param last_block Reference to the current block index counter
    * @param buffer     Output buffer to store the prepared meta data (14 bytes)
    * @return           True if all blocks have been prepared, false otherwise
    */
    bool prepareMetaTextBlock(const char* text, uint8_t& lastBlock, uint8_t* buffer);

    /**
     * Process incoming metadata text across multiple blocks.
     *
     * @param status Pointer to RTX status to update with received text
     * @param meta Reference to metadata structure from LSF
     * @return true if a complete text message was processed
     */
    bool processMetadataText(rtxStatus_t *const status, M17::meta_t& meta);

	uint8_t  textOffset = 0;              ///< Metatext offset
	uint8_t  blk_id_tot = 0;              ///< Metatext block Id total
	uint8_t  frameCnt = 0;                ///< Transmit frame counter
	uint8_t  last_text_blk =  0;          ///< Last metatext block counter
	uint8_t  lsfFragCount = 5;            ///< LSF fragment counter
	uint16_t numPacketbytes = 0;          ///< Number of packet bytes remaining
	bool     foundMetaTextStart = false;  ///< Metatext found flag
	char     textBuffer[M17_META_TEXT_DATA_MAX_LENGTH + 1];              ///< Temporary buffer for incoming metatext
    M17::M17LinkSetupFrame lsf;          ///< M17 link setup frame

    bool startRx;                      ///< Flag for RX management.
    bool startTx;                      ///< Flag for TX management.
    bool locked;                       ///< Demodulator locked on data stream.
    bool dataValid;                    ///< Demodulated data is valid
    bool extendedCall;                 ///< Extended callsign data received
    bool invertTxPhase;                ///< TX signal phase inversion setting.
    bool invertRxPhase;                ///< RX signal phase inversion setting.
    pathId rxAudioPath;                ///< Audio path ID for RX
    pathId txAudioPath;                ///< Audio path ID for TX
    M17::M17Modulator    modulator;    ///< M17 modulator.
    M17::M17Demodulator  demodulator;  ///< M17 demodulator.
    M17::M17FrameDecoder decoder;      ///< M17 frame decoder
    M17::M17FrameEncoder encoder;      ///< M17 frame encoder
};

#endif /* OPMODE_M17_H */
