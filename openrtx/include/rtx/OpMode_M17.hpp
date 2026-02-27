/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_M17_H
#define OPMODE_M17_H

#include "protocols/M17/FrameDecoder.hpp"
#include "protocols/M17/FrameEncoder.hpp"
#include "protocols/M17/Demodulator.hpp"
#include "protocols/M17/Modulator.hpp"
#include "protocols/M17/LinkSetupFrame.hpp"
#include "protocols/M17/PacketDisassembler.hpp"
#include "protocols/M17/MetaText.hpp"
#include "core/audio_path.h"
#include "rtx/SMSQueue.hpp"
#include "OpMode.hpp"

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

    /**
     * Return selected SMS message from queue if any.
     *
     * @param index: message index to retrieve.
     * @param sender: buffer to receive the sender callsign.
     * @param sender_len: size of the sender buffer in bytes.
     * @param message: buffer to receive the message text.
     * @param message_len: size of the message buffer in bytes.
     * @return true if a message was returned.
     */
    bool getSMSMessage(uint8_t index, char *sender, size_t sender_len,
                       char *message, size_t message_len) override;

    /**
     * Delete an SMS message from the queue.
     *
     * @param index: message index to delete.
     */
    void delSMSMessage(uint8_t index) override;

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

    // GPS update interval in superframes. Each superframe is 6 LICH frames
    // (~240 ms), so 25 superframes ≈ 6 seconds.
    static constexpr uint16_t GPS_UPDATE_TICKS = 25;

    bool startRx;                      ///< Flag for RX management.
    bool startTx;                      ///< Flag for TX management.
    bool locked;                       ///< Demodulator locked on data stream.
    bool dataValid;                    ///< Demodulated data is valid
    bool extendedCall;                 ///< Extended callsign data received
    bool invertTxPhase;                ///< TX signal phase inversion setting.
    bool invertRxPhase;                ///< RX signal phase inversion setting.
    pathId rxAudioPath;                ///< Audio path ID for RX
    pathId txAudioPath;                ///< Audio path ID for TX
    M17::Modulator    modulator;    ///< M17 modulator.
    M17::Demodulator  demodulator;  ///< M17 demodulator.
    M17::FrameDecoder decoder;      ///< M17 frame decoder
    M17::FrameEncoder encoder;      ///< M17 frame encoder
    uint16_t gpsTimer;                 ///< GPS data transmission interval timer
    M17::MetaText metaText;            ///< M17 metatext accumulator
    M17::PacketDisassembler pktDisasm;  ///< Data Link Layer packet reassembler
    bool     pktStarted;               ///< Packet reassembly in progress
    SMSQueue smsQueue;                 ///< Received SMS message queue
    uint16_t lastCRC;                  ///< CRC of last accepted packet (dedup)
    char     pendingSender[SMSQueue::CALLSIGN_LEN]; ///< Sender for in-progress RX
};

#endif /* OPMODE_M17_H */
