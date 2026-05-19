/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_H
#define OPMODE_H

#include "interfaces/delays.h"
#include "rtx/rtx.h"

/**
 * This class provides a standard interface for all the operating modes.
 * The class is then specialised for each operating mode and its implementation
 * groups all the common code required to manage the given mode, like data
 * encoding and decoding, squelch management, ...
 */

class OpMode
{
public:

    /**
     * Constructor.
     */
    OpMode() { }

    /**
     * Destructor.
     */
    virtual ~OpMode() { }

    /**
     * Enable the operating mode.
     *
     * Application must ensure this function is being called when entering the
     * new operating mode and always before the first call of "update".
     */
    virtual void enable() { }

    /**
     * Disable the operating mode. This function ensures that, after being
     * called, the radio, the audio amplifier and the microphone are in OFF state.
     *
     * Application must ensure this function is being called when exiting the
     * current operating mode.
     */
    virtual void disable() { }

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
    virtual void update(rtxStatus_t *const status, const bool newCfg)
    {
        (void) status;
        (void) newCfg;
        sleepFor(0u, 30u);
    }

    /**
     * Get the mode identifier corresponding to the OpMode class.
     *
     * @return the corresponding flag from the opmode enum.
     */
    virtual opmode getID()
    {
        return OPMODE_NONE;
    }

    /**
     * Check if RX squelch is open.
     *
     * @return true if RX squelch is open.
     */
    virtual bool rxSquelchOpen()
    {
        return false;
    }
};

#endif /* OPMODE_H */
