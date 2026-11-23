/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_CW_H
#define OPMODE_CW_H

#include "core/cw_keyer.h"
#include "OpMode.hpp"

/**
 * Specialisation of the OpMode class for CW, sent from a paddle through the
 * iambic keyer. The paddle may share the PTT line, so the PTT does not key
 * anything in this mode. The first key down starts transmitting;
 * transmission ends once the key has been up for longer than a word space.
 * The receiver stays on between transmissions, without audio.
 */
class OpMode_CW : public OpMode
{
public:
    /**
     * Constructor.
     */
    OpMode_CW();

    /**
     * Destructor.
     */
    ~OpMode_CW();

    /**
     * Enable the operating mode.
     *
     * Application must ensure this function is being called when entering the
     * new operating mode and always before the first call of "update".
     */
    virtual void enable() override;

    /**
     * Disable the operating mode. This function ensures that, after being
     * called, the radio, the audio amplifier and the microphone are in OFF
     * state.
     *
     * Application must ensure this function is being called when exiting the
     * current operating mode.
     */
    virtual void disable() override;

    /**
     * Update the internal FSM.
     * Application code has to call this function periodically, to ensure
     * proper functionality.
     *
     * @param status: pointer to the rtxStatus_t structure containing the
     * current RTX status. Internal FSM may change the current value of the
     * opStatus flag.
     * @param newCfg: flag used inform the internal FSM that a new RTX
     * configuration has been applied.
     */
    virtual void update(rtxStatus_t *const status, const bool newCfg) override;

    /**
     * Get the mode identifier corresponding to the OpMode class.
     *
     * @return the corresponding flag from the opmode enum.
     */
    virtual opmode getID() override
    {
        return OPMODE_CW;
    }

private:
    /**
     * Read the paddles and run the keyer.
     *
     * @param now: current time, in ms.
     * @return true for key down.
     */
    bool readKey(long long now);

    /**
     * Start transmitting, key up, and restart the keyer so that the first
     * element is not shortened by the time taken to start.
     *
     * @param status: current RTX status.
     * @return true for key down.
     */
    bool startTx(rtxStatus_t *const status);

    static constexpr unsigned int POLL_MS = 1;   ///< Keyer polling period.
    static constexpr unsigned int HANG_DITS = 8; ///< TX hang, a word space +1.

    long long lastKeyDown; ///< Last time the key was down, in ms.
    struct cwKeyer keyer;  ///< Paddle keyer.
    uint8_t wpm;           ///< Keyer speed, in words per minute.
    bool keyDown;          ///< Key state sent to the radio.
    bool enterRx;          ///< Flag for RX management.
};

#endif /* OPMODE_CW_H */
