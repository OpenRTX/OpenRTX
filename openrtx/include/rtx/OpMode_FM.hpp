/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_FM_H
#define OPMODE_FM_H

#include "core/audio_path.h"
#include "hwconfig.h"
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

    #ifdef CONFIG_FM_INBAND_TONES
    /**
     * Play whatever tone is being transmitted on the speaker, so that the
     * operator hears what is going on air, and stop it once the key is
     * released or transmission ends. Neither the DTMF digits nor the tone
     * burst are otherwise audible locally: both are generated inside the
     * HR_C6000 and reach the modulator only.
     *
     * @param dtmfCode: DTMF digit being transmitted, or DTMF_CODE_NONE.
     * @param toneBurst: true while the repeater tone burst is transmitted.
     */
    void updateTxSidetone(const uint8_t dtmfCode, const bool toneBurst);

    uint8_t txSidetone;        ///< Tone currently played as sidetone.
    pathId  sidetoneAudioPath; ///< Audio path ID for the sidetone.
    #endif
};

#endif /* OPMODE_FM_H */
