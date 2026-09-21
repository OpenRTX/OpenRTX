/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_APRS_H
#define OPMODE_APRS_H

#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "core/ringbuf.hpp"
#include "core/dsp.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/Modulator.hpp"
#include "protocols/APRS/Slicer.hpp"
#include "OpMode.hpp"
#include <atomic>
#include <memory>

/**
 * Specialization of the OpMode class for the management of APRS operating
 * mode.
 *
 * Receive feeds decoded frames into descriptors the inbox arms through
 * addPacketRx(). Transmit is driven the same way, through addPacketTx(): the
 * descriptor's buffer holds a complete AX.25 frame without its check
 * sequence, and one transmission is in flight at a time. There is no PTT
 * path, because holding the key down means nothing in a packet mode.
 */

class OpMode_APRS : public OpMode
{
public:
    /**
     * Constructor.
     */
    OpMode_APRS();

    /**
     * Destructor.
     */
    ~OpMode_APRS();

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
        return OPMODE_APRS;
    }

    /**
     * This always returns true as the squelch is open in this mode.
     *
     * @return true
     */
    virtual bool rxSquelchOpen() override
    {
        return true;
    }

    /**
     * Submit a packet reception request.
     *
     * @param packet: pointer to packet descriptor.
     * @return zero on success a negative error code otherwise.
     */
    virtual int addPacketRx(struct pktDesc *packet) override
    {
        return rxPktQueue.tryPush(packet);
    }

    /**
     * Submit a packet transmission request.
     *
     * The descriptor's buffer must hold a complete AX.25 frame without its
     * frame check sequence, and size must be its length. A single slot is
     * claimed atomically, so a second submission while one is in flight is
     * refused rather than queued.
     *
     * @param packet: pointer to packet descriptor.
     * @return zero on success, -EINVAL for a malformed request, -EBUSY when a
     * transmission is already pending.
     */
    virtual int addPacketTx(struct pktDesc *packet) override;

private:
    /**
     * Function handling the OFF operating state.
     *
     * @param status: pointer to the rtxStatus_t structure containing
     * the current RTX status.
     */
    void offState(rtxStatus_t *const status);

    /**
     * Function handling the RX operating state.
     */
    void rxState(rtxStatus_t *const status);

    /**
     * Function handling the TX operating state.
     */
    void txState(rtxStatus_t *const status);

    /**
     * Close the baseband input stream and switch to OFF state.
     *
     * @param status: pointer to RTX status data structure.
     */
    void stopRx(rtxStatus_t *const status);

    /**
     * Start a pending transmission if there is one and the channel allows it.
     * Called from both OFF and RX so a transmission is not held hostage by a
     * receive path that never came up.
     *
     * @return true if the operating status was switched to TX.
     */
    bool startTxIfPending(rtxStatus_t *const status);

    /**
     * Decide whether a pending transmission may start now: defer while the
     * demodulator reports carrier, but not past TX_DEFER_TIMEOUT.
     *
     * @return true if the transmission should start.
     */
    bool channelClear();

    /**
     * Complete the pending transmission and release its descriptor.
     *
     * @param result: value for res; negative marks the transmission failed.
     */
    void finishTx(ssize_t result);

    /** Flag sequence sent before a frame, in milliseconds. */
    static constexpr unsigned TX_DELAY_MS = 300;

    /** Flag sequence sent after a frame, in milliseconds. */
    static constexpr unsigned TX_TAIL_MS = 30;

    /** Longest a pending transmission defers to a busy channel, in ms. */
    static constexpr long long TX_DEFER_TIMEOUT = 5000;

    /**
     * Phases of one transmission, each advanced by a single update() call so
     * no tick blocks for the whole of a frame's airtime.
     */
    enum class TxPhase {
        KEYUP, ///< Key the radio and send the opening flags.
        FRAME, ///< Send the frame and its check sequence.
        TAIL,  ///< Send the closing flags, unkey, and report completion.
    };

    bool enterRx;                              ///< Flag for RX management.
    bool streamRunning;                        ///< Baseband stream is open.
    pathId rxAudioPath;                        ///< Audio path ID for RX
    std::unique_ptr<int16_t[]> basebandBuffer; ///< buffer for RX audio handling
    streamId basebandId;                       ///< Stream ID for RX to MCU
    APRS::Demodulator demodulator; ///< Demodulator for incoming samples
    APRS::Modulator modulator;     ///< Modulator for outgoing frames
    NbRingBuffer<struct pktDesc *, 2> rxPktQueue; ///< Pending RX descriptors

    std::atomic<struct pktDesc *> currTxPkt;      ///< Pending TX descriptor.
    TxPhase txPhase;        ///< Phase of the transmission in progress.
    long long txDeferStart; ///< Tick at which deferral began, 0 if not.
};

#endif /* OPMODE_APRS_H */
