/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_FM_H
#define OPMODE_FM_H

#include "core/audio_path.h"
#include "OpMode.hpp"

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

    /**
     * Check if RX squelch is open.
     *
     * @return true if RX squelch is open.
     */
    virtual bool rxSquelchOpen() override;

private:

    bool   rfSqlOpen;   ///< Flag for RF squelch status (analog squelch).
    bool   sqlOpen;     ///< Flag for squelch status.
    bool   enterRx;     ///< Flag for RX management.
    pathId rxAudioPath; ///< Audio path ID for RX
    pathId txAudioPath; ///< Audio path ID for TX
};

#endif /* OPMODE_FM_H */
