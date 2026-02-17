/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_APRS_H
#define OPMODE_APRS_H

#include "core/audio_path.h"
#include "core/audio_stream.h"
#include "core/dsp.h"
#include "protocols/APRS/constants.h"
#include "protocols/APRS/Demodulator.hpp"
#include "protocols/APRS/Slicer.hpp"
#include "protocols/APRS/HDLC.hpp"
#include "OpMode.hpp"
#include <memory>

/**
 * Specialization of the OpMode class for the management of APRS operating
 * mode.
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

private:
    bool enterRx;                              ///< Flag for RX management.
    pathId rxAudioPath;                        ///< Audio path ID for RX
    std::unique_ptr<int16_t[]> basebandBuffer; ///< buffer for RX audio handling
    streamId basebandId;                       ///< Stream ID for RX to MCU
    struct dcBlock dcBlock;                    ///< DC block filter state
    APRS::Demodulator demodulator; ///< Demodulator for incoming samples
    APRS::Slicer slicer;           ///< Slicer for demodulator output
    APRS::Decoder decoder;         ///< Decoder for slicer output
};

#endif /* OPMODE_APRS_H */
