/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "interfaces/platform.h"
#include "interfaces/delays.h"
#include "interfaces/audio.h"
#include "interfaces/radio.h"
#include "protocols/M17/Callsign.hpp"
#include "protocols/horse/HorseDatatypes.hpp"
#include "rtx/OpMode_Horse.hpp"
#include "core/audio_codec.h"
#include "core/horse_codec.h"
#include "core/state.h"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/horse_keystore.h"
#include "interfaces/cps_io.h"
#include "protocols/horse/horse_crypto.h"
#include "rtx/rtx.h"
#include <cstring>
#include <string>

static bool horse_is_sig_frame(uint16_t fn)
{
    return fn >= horse::SIG_FRAME_BASE &&
           fn < (horse::SIG_FRAME_BASE + horse::SIG_FRAME_COUNT);
}

static bool horse_contact_has_x25519(const contact_t *contact)
{
    if (contact == NULL)
        return false;

    for (size_t i = 0; i < sizeof contact->info.horse.x25519_pk; i++)
    {
        if (contact->info.horse.x25519_pk[i] != 0)
            return true;
    }
    return false;
}

static bool horse_contact_has_ed25519(const contact_t *contact)
{
    if (contact == NULL)
        return false;

    for (size_t i = 0; i < sizeof contact->info.horse.ed25519_pk; i++)
    {
        if (contact->info.horse.ed25519_pk[i] != 0)
            return true;
    }
    return false;
}

#ifdef PLATFORM_MOD17
#include "calibration/calibInfo_Mod17.h"
#endif

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include "interfaces/platform.h"
#endif

using namespace horse;

OpMode_Horse::OpMode_Horse()
    : startRx(false),
      startTx(false),
      locked(false),
      dataValid(false),
      invertTxPhase(false),
      invertRxPhase(false),
      rxAudioPath(-1),
      txAudioPath(-1),
      sessionKey{},
      frameAuthKey{},
      rxSessionSig{},
      txSessionSig{},
      rxLsfEphPk{},
      rxLsfFlags(0),
      rxLsfVersion(0),
      rxSigChunks(0),
      rxLsfSrc{},
      rxLsfDst{},
      sessionValid(false),
      encryptTx(false),
      encryptRx(false),
      signTx(false),
      signRx(false),
      txSigSent(false),
      rxSigReady(false),
      haveRxVoiceFn(false),
      rxLastVoiceFn(0)
{
}

OpMode_Horse::~OpMode_Horse()
{
    disable();
}

void OpMode_Horse::resetRxCrypto()
{
    sessionValid = false;
    encryptRx = false;
    signRx = false;
    rxSigReady = false;
    rxSigChunks = 0;
    rxLsfFlags = 0;
    rxLsfVersion = 0;
    haveRxVoiceFn = false;
    rxLastVoiceFn = 0;
    horse_crypto_memzero(sessionKey, sizeof sessionKey);
    horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
    horse_crypto_memzero(rxSessionSig, sizeof rxSessionSig);
    horse_crypto_memzero(rxLsfEphPk, sizeof rxLsfEphPk);
    memset(&rxLsfSrc, 0, sizeof rxLsfSrc);
    memset(&rxLsfDst, 0, sizeof rxLsfDst);
}

void OpMode_Horse::tryFinalizeRxSessionSig()
{
    if (rxSigChunks < SIG_FRAME_COUNT)
        return;

    contact_t contact;
    memset(&contact, 0, sizeof contact);
    if (state.channel.horse.contact_index == 0 ||
        cps_readContact(&contact, state.channel.horse.contact_index) != 0 ||
        contact.mode != OPMODE_HORSE ||
        !horse_contact_has_ed25519(&contact))
        return;

    uint8_t session_msg[HORSE_SESSION_MSG_BYTES];
    horse_crypto_build_session_message(rxLsfSrc.data(), rxLsfDst.data(),
                                       rxLsfEphPk, rxLsfFlags, rxLsfVersion,
                                       session_msg);
    if (!horse_crypto_verify(contact.info.horse.ed25519_pk, session_msg,
                             sizeof session_msg, rxSessionSig))
        return;

    rxSigReady = true;
}

void OpMode_Horse::sendTxVoiceFrame(const uint8_t *melpe, bool isLast,
                                    horse::frame_t &outFrame)
{
    uint8_t voiceTag[HORSE_VOICE_TAG_BYTES] = {0};
    uint8_t payload[HORSE_CODEC_FRAME_BYTES];
    memcpy(payload, melpe, sizeof payload);

    uint16_t fn = encoder.currentVoiceFrameNumber();
    if (encryptTx && sessionValid) {
        uint8_t nonce[12];
        uint8_t cipher[HORSE_CODEC_FRAME_BYTES];
        horse_crypto_voice_nonce_from_fn(fn, nonce);
        horse_crypto_voice_encrypt(sessionKey, frameAuthKey,
                                   HORSE_VOICE_DIR_FORWARD, fn, nonce, payload,
                                   HORSE_CODEC_FRAME_BYTES, cipher, voiceTag);
        encoder.encodeVoiceFrame(cipher, voiceTag, outFrame, isLast);
    } else if (sessionValid) {
        horse_crypto_voice_auth_tag(frameAuthKey, HORSE_VOICE_DIR_FORWARD, fn,
                                    payload, voiceTag);
        encoder.encodeVoiceFrame(payload, voiceTag, outFrame, isLast);
    } else
        encoder.encodeVoiceFrame(payload, voiceTag, outFrame, isLast);
}

void OpMode_Horse::enable()
{
    codec_init();
    horse_codec_init();
    modulator.init();
    demodulator.init();
    locked    = false;
    dataValid = false;
    startRx   = true;
    startTx   = false;
    sessionValid = false;
    encryptTx = false;
    encryptRx = false;
    signTx = false;
    signRx = false;
    txSigSent = false;
    rxSigReady = false;
    rxSigChunks = 0;
    horse_crypto_memzero(sessionKey, sizeof sessionKey);
    horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
    horse_crypto_memzero(rxSessionSig, sizeof rxSessionSig);
    horse_crypto_memzero(txSessionSig, sizeof txSessionSig);

    (void)horse_keystore_unlock_held();
#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
    invertTxPhase = true;
    if (platform_getHwInfo()->vhf_band == 1)
        invertRxPhase = true;
    else
        invertRxPhase = false;
#elif defined(PLATFORM_MOD17)
    extern mod17Calib_t mod17CalData;
    invertTxPhase = (mod17CalData.bb_tx_invert == 1);
    invertRxPhase = (mod17CalData.bb_rx_invert == 1);
#else
    invertTxPhase = true;
    invertRxPhase = false;
#endif
}

void OpMode_Horse::disable()
{
    startRx = false;
    startTx = false;
    platform_ledOff(GREEN);
    platform_ledOff(RED);
    horse_codec_stop(rxAudioPath);
    horse_codec_stop(txAudioPath);
    audioPath_release(rxAudioPath);
    audioPath_release(txAudioPath);
    horse_codec_terminate();
    codec_terminate();
    radio_disableRtx();
    modulator.terminate();
    demodulator.terminate();
    sessionValid = false;
    encryptTx = false;
    encryptRx = false;
    signTx = false;
    signRx = false;
    txSigSent = false;
    rxSigReady = false;
    rxSigChunks = 0;
    horse_crypto_memzero(sessionKey, sizeof sessionKey);
    horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
    horse_crypto_memzero(rxSessionSig, sizeof rxSessionSig);
    horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
    horse_keystore_lock();
}

void OpMode_Horse::update(rtxStatus_t* const status, const bool newCfg)
{
    (void)newCfg;
    switch (status->opStatus)
    {
    case OFF:
        offState(status);
        break;
    case RX:
        rxState(status);
        break;
    case TX:
        txState(status);
        break;
    default:
        break;
    }
    switch (status->opStatus)
    {
    case RX:
        if (dataValid)
            platform_ledOn(GREEN);
        else
            platform_ledOff(GREEN);
        break;
    case TX:
        platform_ledOff(GREEN);
        platform_ledOn(RED);
        break;
    default:
        platform_ledOff(GREEN);
        platform_ledOff(RED);
        break;
    }
}

void OpMode_Horse::offState(rtxStatus_t* const status)
{
    radio_disableRtx();
    codec_stop(txAudioPath);
    audioPath_release(txAudioPath);
    if (startRx)
    {
        status->opStatus = RX;
        return;
    }
    if (platform_getPttStatus() && (status->txDisable == 0))
    {
        startTx          = true;
        status->opStatus = TX;
        return;
    }
    sleepFor(0, 30);
}

void OpMode_Horse::rxState(rtxStatus_t* const status)
{
    if (startRx)
    {
        demodulator.startBasebandSampling();
        radio_enableRx();
        startRx = false;
    }
    bool newData = demodulator.update(invertRxPhase);
    bool lock    = demodulator.isLocked();
    if (lock && !locked)
    {
        decoder.reset();
        locked = lock;
    }
    if (locked)
    {
        if (newData)
        {
            const frame_t& frame = demodulator.getFrame();
            HorseFrameType type  = decoder.decodeFrame(frame);
            status->horseLsfOk  = (type == HorseFrameType::LINK_SETUP);
            if (status->horseLsfOk)
            {
                dataValid = true;
                resetRxCrypto();
                decoder.getLsfCallsigns(rxLsfSrc, rxLsfDst);
                std::string srcStr = M17::Callsign(rxLsfSrc);
                std::string dstStr = M17::Callsign(rxLsfDst);
                strncpy(status->horse_src, srcStr.c_str(), 9);
                status->horse_src[9] = '\0';
                strncpy(status->horse_dst, dstStr.c_str(), 9);
                status->horse_dst[9] = '\0';

                uint8_t eph_pk[HORSE_X25519_PUBLICKEY_BYTES];
                uint8_t flags = 0;
                uint8_t version = 0;
                if (decoder.getLsfCrypto(frame, eph_pk, &flags, &version))
                {
                    rxLsfFlags = flags;
                    rxLsfVersion = version;
                    memcpy(rxLsfEphPk, eph_pk, sizeof rxLsfEphPk);
                }

                if (horse_crypto_lsf_version_ok(version) &&
                    horse_keystore_is_unlocked())
                {
                    horse_identity_keys_t id;
                    if (horse_keystore_copy_identity(&id) &&
                        horse_crypto_derive_session_keys(id.x25519_sk, eph_pk,
                                                         sessionKey,
                                                         frameAuthKey))
                    {
                        sessionValid = true;
                        if (flags & LSF_FLAG_ENCRYPTED)
                            encryptRx = true;
                    }
                    horse_crypto_memzero(&id, sizeof id);
                }

                if (flags & LSF_FLAG_SIGNED)
                    signRx = true;

                const bool may_audio = horse_rx_may_output_voice(
                    (flags & LSF_FLAG_ENCRYPTED) != 0, sessionValid,
                    (flags & LSF_FLAG_SIGNED) != 0, rxSigReady);
                if (may_audio && rxAudioPath < 0)
                {
                    rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
                    if (rxAudioPath >= 0 && audioPath_getStatus(rxAudioPath) == PATH_OPEN)
                        horse_codec_startDecode(rxAudioPath);
                }
            }
            if (type == HorseFrameType::VOICE)
            {
                uint8_t melpe[HORSE_CODEC_FRAME_BYTES];
                uint8_t tag[4];
                uint16_t fn;
                decoder.getVoicePayload(frame, melpe, tag, &fn);

                bool drop_voice = false;
                if (horse_is_sig_frame(fn))
                {
                    uint16_t chunk = fn - SIG_FRAME_BASE;
                    if (chunk < SIG_FRAME_COUNT)
                    {
                        horse::horse_sig_store_chunk(rxSessionSig, chunk,
                                                     melpe);
                        rxSigChunks++;
                        tryFinalizeRxSessionSig();
                        if (horse_rx_may_output_voice(
                                (rxLsfFlags & LSF_FLAG_ENCRYPTED) != 0,
                                sessionValid,
                                (rxLsfFlags & LSF_FLAG_SIGNED) != 0,
                                rxSigReady) &&
                            rxAudioPath < 0)
                        {
                            rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK,
                                                            PRIO_RX);
                            if (rxAudioPath >= 0 &&
                                audioPath_getStatus(rxAudioPath) == PATH_OPEN)
                                horse_codec_startDecode(rxAudioPath);
                        }
                    }
                    drop_voice = true;
                }
                else if (!horse_rx_may_output_voice(
                             (rxLsfFlags & LSF_FLAG_ENCRYPTED) != 0,
                             sessionValid,
                             (rxLsfFlags & LSF_FLAG_SIGNED) != 0, rxSigReady))
                {
                    drop_voice = true;
                }
                else if (encryptRx && sessionValid)
                {
                    uint8_t nonce[12];
                    uint8_t plain[HORSE_CODEC_FRAME_BYTES];
                    horse_crypto_voice_nonce_from_fn(fn, nonce);
                    if (!horse_crypto_voice_decrypt(sessionKey, frameAuthKey,
                                                    HORSE_VOICE_DIR_FORWARD, fn,
                                                    nonce, melpe,
                                                    HORSE_CODEC_FRAME_BYTES,
                                                    tag, plain))
                        drop_voice = true;
                    else
                        memcpy(melpe, plain, sizeof melpe);
                }
                else if (sessionValid)
                {
                    if (!horse_crypto_voice_auth_verify(frameAuthKey,
                                                        HORSE_VOICE_DIR_FORWARD,
                                                        fn, melpe, tag))
                        drop_voice = true;
                }
                else
                    drop_voice = true;

                if (!drop_voice) {
                    if (!voice_fn_newer(haveRxVoiceFn, rxLastVoiceFn, fn))
                        drop_voice = true;
                    else {
                        haveRxVoiceFn = true;
                        rxLastVoiceFn = fn;
                    }
                }

                if (!drop_voice && rxAudioPath >= 0 && horse_codec_running())
                    horse_codec_pushFrame(melpe, false);
            }
        }
    }
    locked = lock;
    if (platform_getPttStatus())
    {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }
    if (!locked)
    {
        status->horseLsfOk = false;
        dataValid          = false;
        status->horse_dst[0] = '\0';
        status->horse_src[0] = '\0';
        resetRxCrypto();
        if (rxAudioPath >= 0)
        {
            horse_codec_stop(rxAudioPath);
            audioPath_release(rxAudioPath);
            rxAudioPath = -1;
        }
    }
}

void OpMode_Horse::txState(rtxStatus_t* const status)
{
    horse::frame_t outFrame;
    if (startTx)
    {
        startTx = false;
        txAudioPath = audioPath_request(SOURCE_MIC, SINK_RTX, PRIO_TX);
        if (txAudioPath < 0)
        {
            status->opStatus = OFF;
            return;
        }
        if (!horse_codec_startEncode(txAudioPath))
        {
            audioPath_release(txAudioPath);
            txAudioPath = -1;
            status->opStatus = OFF;
            return;
        }
        horse::call_t srcCall = M17::Callsign(status->source_address);
        horse::call_t dstCall = M17::Callsign(status->destination_address);
        encoder.reset();
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);

        uint8_t eph_pk[HORSE_X25519_PUBLICKEY_BYTES] = {0};
        uint8_t eph_sk[HORSE_X25519_SECRETKEY_BYTES] = {0};
        uint8_t flags = 0;
        contact_t contact;
        horse_identity_keys_t id;
        memset(&contact, 0, sizeof contact);
        memset(&id, 0, sizeof id);
        bool have_contact = false;
        bool want_encrypt = state.channel.horse.encrypt_en;
        bool want_sign = state.channel.horse.sign_en;

        if (!want_encrypt && !want_sign)
            want_encrypt = true;

        if (state.channel.horse.contact_index != 0 &&
            cps_readContact(&contact, state.channel.horse.contact_index) == 0 &&
            contact.mode == OPMODE_HORSE)
            have_contact = true;

        const bool have_id = horse_keystore_copy_identity(&id);
        if (!horse_tx_allowed(want_encrypt, want_sign, horse_crypto_available(),
                              have_id,
                              have_contact && horse_contact_has_x25519(&contact),
                              have_contact && horse_contact_has_ed25519(&contact)))
        {
            if (!horse_crypto_available())
                status->horseError = HORSE_ERR_NO_CRYPTO;
            else
                status->horseError = HORSE_ERR_NO_KEYS;
            horse_crypto_memzero(&id, sizeof id);
            horse_codec_stop(txAudioPath);
            audioPath_release(txAudioPath);
            txAudioPath = -1;
            status->opStatus = OFF;
            return;
        }
        status->horseError = HORSE_ERR_NONE;

        if (!horse_crypto_x25519_keypair(eph_pk, eph_sk) ||
            !horse_crypto_derive_session_keys(eph_sk,
                                              contact.info.horse.x25519_pk,
                                              sessionKey, frameAuthKey))
        {
            status->horseError = HORSE_ERR_NO_KEYS;
            horse_crypto_memzero(eph_sk, sizeof eph_sk);
            horse_crypto_memzero(&id, sizeof id);
            horse_codec_stop(txAudioPath);
            audioPath_release(txAudioPath);
            txAudioPath = -1;
            status->opStatus = OFF;
            return;
        }
        sessionValid = true;
        if (want_encrypt) {
            encryptTx = true;
            flags |= LSF_FLAG_ENCRYPTED;
        }
        horse_crypto_memzero(eph_sk, sizeof eph_sk);

        if (want_sign)
        {
            uint8_t session_msg[HORSE_SESSION_MSG_BYTES];
            horse_crypto_build_session_message(srcCall.data(), dstCall.data(),
                                               eph_pk, flags | LSF_FLAG_SIGNED,
                                               HORSE_LSF_VERSION, session_msg);
            if (horse_crypto_sign(id.ed25519_sk, session_msg, sizeof session_msg,
                                  txSessionSig))
            {
                signTx = true;
                flags |= LSF_FLAG_SIGNED;
            }
        }
        horse_crypto_memzero(&id, sizeof id);

        if ((want_encrypt && !encryptTx) || (want_sign && !signTx) ||
            !sessionValid)
        {
            status->horseError = HORSE_ERR_NO_KEYS;
            horse_codec_stop(txAudioPath);
            audioPath_release(txAudioPath);
            txAudioPath = -1;
            status->opStatus = OFF;
            return;
        }

        encoder.encodeLsf(srcCall, dstCall, eph_pk, flags, outFrame);
        modulator.invertPhase(invertTxPhase);
        if (!modulator.start())
        {
            horse_codec_stop(txAudioPath);
            audioPath_release(txAudioPath);
            txAudioPath = -1;
            status->opStatus = OFF;
            return;
        }
        modulator.sendPreamble();
        modulator.sendFrame(outFrame);

        if (signTx)
        {
            uint8_t zeroTag[HORSE_VOICE_TAG_BYTES] = {0};
            for (uint16_t i = 0; i < SIG_FRAME_COUNT; i++)
            {
                const size_t n = horse::sig_chunk_bytes(i);
                encoder.encodeVoiceFrameWithFn(txSessionSig + (i * horse::SIG_CHUNK_BYTES),
                                               zeroTag,
                                               SIG_FRAME_BASE + i, outFrame, false,
                                               n);
                modulator.sendFrame(outFrame);
                sleepFor(0u, 40u);
            }
            txSigSent = true;
        }
    }

    uint8_t melpeBuf[HORSE_CODEC_FRAME_BYTES];
    uint16_t nextFn = encoder.currentVoiceFrameNumber();
    if (nextFn > VOICE_FN_MAX)
    {
        encoder.encodeEotFrame(outFrame);
        modulator.sendFrame(outFrame);
        modulator.stop();
        horse_codec_stop(txAudioPath);
        audioPath_release(txAudioPath);
        txAudioPath = -1;
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
        status->horseError = HORSE_ERR_CALL_LIMIT;
        status->opStatus = OFF;
        return;
    }
    const bool last_legal = (nextFn == VOICE_FN_MAX);

    if (horse_codec_popFrame(melpeBuf, true) != 0)
        memset(melpeBuf, 0, sizeof(melpeBuf));

    sendTxVoiceFrame(melpeBuf, last_legal, outFrame);
    modulator.sendFrame(outFrame);
    sleepFor(0u, 40u);

    if (last_legal)
    {
        encoder.encodeEotFrame(outFrame);
        modulator.sendFrame(outFrame);
        modulator.stop();
        horse_codec_stop(txAudioPath);
        audioPath_release(txAudioPath);
        txAudioPath = -1;
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
        status->horseError = HORSE_ERR_CALL_LIMIT;
        status->opStatus = OFF;
        return;
    }

    if (!platform_getPttStatus())
    {
        if (horse_codec_popFrame(melpeBuf, false) != 0)
            memset(melpeBuf, 0, sizeof(melpeBuf));
        sendTxVoiceFrame(melpeBuf, true, outFrame);
        modulator.sendFrame(outFrame);
        encoder.encodeEotFrame(outFrame);
        modulator.sendFrame(outFrame);
        modulator.stop();
        horse_codec_stop(txAudioPath);
        audioPath_release(txAudioPath);
        txAudioPath = -1;
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
        status->opStatus = OFF;
    }
}
