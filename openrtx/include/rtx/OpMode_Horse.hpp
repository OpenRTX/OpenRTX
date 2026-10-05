/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef OPMODE_HORSE_H
#define OPMODE_HORSE_H

#include "protocols/horse/HorseFrameDecoder.hpp"
#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseDemodulator.hpp"
#include "protocols/horse/HorseModulator.hpp"
#include "protocols/horse/horse_crypto.h"
#include "core/audio_path.h"
#include "OpMode.hpp"

#ifndef __cplusplus
#error This header is C++ only!
#endif

class OpMode_Horse : public OpMode
{
public:
    OpMode_Horse();
    ~OpMode_Horse();

    virtual void enable() override;
    virtual void disable() override;
    virtual void update(rtxStatus_t *const status, const bool newCfg) override;
    virtual opmode getID() override
    {
        return OPMODE_HORSE;
    }
    virtual bool rxSquelchOpen() override
    {
        return dataValid;
    }

private:
    void offState(rtxStatus_t *const status);
    void rxState(rtxStatus_t *const status);
    void txState(rtxStatus_t *const status);
    void abortTx(rtxStatus_t *const status, bool stop_mod);
    void resetRxCrypto();
    void tryFinalizeRxSessionSig();
    void sendTxVoiceFrame(const uint8_t *melpe, bool isLast,
                          horse::frame_t &outFrame);

    bool startRx;
    bool startTx;
    bool locked;
    bool dataValid;
    bool invertTxPhase;
    bool invertRxPhase;
    pathId rxAudioPath;
    pathId txAudioPath;
    horse::HorseModulator modulator;
    horse::HorseDemodulator demodulator;
    horse::HorseFrameDecoder decoder;
    horse::HorseFrameEncoder encoder;

    uint8_t sessionKey[HORSE_SESSION_KEY_BYTES];
    uint8_t frameAuthKey[HORSE_SESSION_KEY_BYTES];
    uint8_t rxSessionSig[HORSE_ED25519_SIGNATURE_BYTES];
    uint8_t txSessionSig[HORSE_ED25519_SIGNATURE_BYTES];
    uint8_t rxLsfEphPk[HORSE_X25519_PUBLICKEY_BYTES];
    uint8_t rxLsfFlags;
    uint8_t rxLsfVersion;
    uint8_t rxSigChunks;
    horse::call_t rxLsfSrc;
    horse::call_t rxLsfDst;
    bool sessionValid;
    bool encryptTx;
    bool encryptRx;
    bool signTx;
    bool signRx;
    bool txSigSent;
    bool rxSigReady;
    bool haveRxVoiceFn;
    uint16_t rxLastVoiceFn;
};

#endif // OPMODE_HORSE_H
