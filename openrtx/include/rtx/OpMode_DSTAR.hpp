#ifndef OPMODE_DSTAR_H
#define OPMODE_DSTAR_H

#include <DSTAR/DStar_IO.hpp>
#include <audio_path.h>
#include "OpMode.hpp"
#include <string>

/**
 * Specialisation of the OpMode class for the management of DSTAR operating mode.
 */
class OpMode_DSTAR : public OpMode
{
public:

    /**
     * Constructor.
     */
    OpMode_DSTAR();

    /**
     * Destructor.
     */
    ~OpMode_DSTAR();

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
        return OPMODE_DSTAR;
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


    bool startRx;                      ///< Flag for RX management.
    bool startTx;                      ///< Flag for TX management.
//    bool locked;                       ///< Demodulator locked on data stream.
    bool dataValid;                    ///< Demodulated data is valid
    bool invertTxPhase;                ///< TX signal phase inversion setting.
    bool invertRxPhase;                ///< RX signal phase inversion setting.
    pathId rxAudioPath;                ///< Audio path ID for RX
    pathId txAudioPath;                ///< Audio path ID for TX
    DStar_IO dstar_io;                 ///< DSTAR IO control.
};

#endif /* OPMODE_DSTAR_H */
