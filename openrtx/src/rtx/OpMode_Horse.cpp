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
#include "protocols/horse/horse_peers.h"
#include "protocols/horse/horse_crypto.h"
#include "protocols/horse/horse_crypto_worker.h"
#include "rtx/rtx.h"
#include <cstring>

static bool horse_is_sig_frame(uint16_t fn)
{
    return fn >= horse::SIG_FRAME_BASE
        && fn < (horse::SIG_FRAME_BASE + horse::SIG_FRAME_COUNT);
}

#ifdef PLATFORM_MOD17
#include "calibration/calibInfo_Mod17.h"
#endif

#if defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0)
#include "interfaces/platform.h"
#endif

static bool horse_wait_job(bool check_ptt)
{
    for (;;) {
        horse_crypto_st st = horse_crypto_poll();
        if (st == HORSE_CRYPTO_ST_PENDING) {
            if (check_ptt && !platform_getPttStatus()) {
                horse_crypto_cancel();
                (void)horse_crypto_wait();
                return false;
            }
            sleepFor(0, 5);
            continue;
        }
        return st == HORSE_CRYPTO_ST_DONE;
    }
}

using namespace horse;

OpMode_Horse::OpMode_Horse()
    : startRx(false)
    , startTx(false)
    , locked(false)
    , dataValid(false)
    , invertTxPhase(false)
    , invertRxPhase(false)
    , rxAudioPath(-1)
    , txAudioPath(-1)
    , sessionKey{}
    , frameAuthKey{}
    , rxSessionSig{}
    , txSessionSig{}
    , rxLsfEphPk{}
    , rxLsfFlags(0)
    , rxLsfVersion(0)
    , rxSigChunks(0)
    , rxLsfSrc{}
    , rxLsfDst{}
    , sessionValid(false)
    , encryptTx(false)
    , encryptRx(false)
    , signTx(false)
    , signRx(false)
    , txSigSent(false)
    , rxSigReady(false)
    , haveRxVoiceFn(false)
    , rxLastVoiceFn(0)
    , txOutFrame{}
    , txLsfFrames{}
    , txEphPk{}
    , txEphSk{}
    , txMelpe{}
    , txSessionMsg{}
    , txZeroTag{}
    , rxSoft{}
    , txPeer{}
    , txId{}
    , rxId{}
    , rxPeer{}
    , rxSessionMsg{}
    , rxNonce{}
    , rxPlain{}
    , rxMelpe{}
    , rxTag{}
    , txNonce{}
    , txCipher{}
    , txPayload{}
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
    if (rxSigReady)
        return;
    if (rxSigChunks < SIG_FRAME_COUNT)
        return;

    horse_peer_t &peer = rxPeer;
    memset(&peer, 0, sizeof peer);
    if (state.channel.horse.contact_index == 0
        || !horse_peer_read(state.channel.horse.contact_index, &peer)
        || !horse_peer_has_ed25519(&peer))
        return;

    horse_crypto_build_session_message(rxLsfSrc.data(), rxLsfDst.data(),
                                       rxLsfEphPk, rxLsfFlags, rxLsfVersion,
                                       rxSessionMsg);
    if (!horse_crypto_req_verify(peer.ed25519_pk, rxSessionMsg,
                                 sizeof rxSessionMsg, rxSessionSig)
        || !horse_wait_job(false))
        return;

    rxSigReady = true;
}

void OpMode_Horse::tryFinalizeRxSigFragments()
{
    if (rxSigReady || !signRx)
        return;
    if (!decoder.getSigFragments(rxSessionSig))
        return;
    rxSigChunks = SIG_FRAME_COUNT;
    tryFinalizeRxSessionSig();
}

void OpMode_Horse::maybeStartRxAudio()
{
    const bool may_audio = horse_rx_may_output_voice(
        (rxLsfFlags & LSF_FLAG_ENCRYPTED) != 0, sessionValid,
        (rxLsfFlags & LSF_FLAG_SIGNED) != 0, rxSigReady,
        state.channel.horse.encrypt_en, state.channel.horse.sign_en);
    if (may_audio && rxAudioPath < 0) {
        rxAudioPath = audioPath_request(SOURCE_MCU, SINK_SPK, PRIO_RX);
        if (rxAudioPath >= 0 && audioPath_getStatus(rxAudioPath) == PATH_OPEN)
            horse_codec_startDecode(rxAudioPath);
    }
}

bool OpMode_Horse::applyLsfIfReady(rtxStatus_t *const status)
{
    if (!decoder.lsfReady())
        return false;
    if (dataValid) {
        tryFinalizeRxSigFragments();
        maybeStartRxAudio();
        return horse_crypto_lsf_version_ok(rxLsfVersion);
    }

    status->horseLsfOk = true;
    resetRxCrypto();
    dataValid = true;
    decoder.getLsfCallsigns(rxLsfSrc, rxLsfDst);
    {
        M17::Callsign srcCs(rxLsfSrc);
        M17::Callsign dstCs(rxLsfDst);
        strncpy(status->horse_src, srcCs, 9);
        status->horse_src[9] = '\0';
        strncpy(status->horse_dst, dstCs, 9);
        status->horse_dst[9] = '\0';
    }

    uint8_t flags = 0;
    uint8_t version = 0;
    if (!decoder.getLsfCrypto(rxLsfEphPk, &flags, &version))
        return false;
    rxLsfFlags = flags;
    rxLsfVersion = version;

    if (!horse_crypto_lsf_version_ok(version))
        return false;

    demodulator.noteValidTag();

    if (horse_keystore_is_unlocked()) {
        horse_identity_keys_t &id = rxId;
        if (horse_keystore_copy_identity(&id)
            && horse_crypto_req_derive(id.x25519_sk, rxLsfEphPk,
                                       rxLsfSrc.data(), rxLsfDst.data(),
                                       rxLsfEphPk, flags, version)
            && horse_wait_job(false)
            && horse_crypto_take_session(sessionKey, frameAuthKey)) {
            sessionValid = true;
            if (flags & LSF_FLAG_ENCRYPTED)
                encryptRx = true;
        }
        horse_crypto_memzero(&id, sizeof id);
    }

    if (flags & LSF_FLAG_SIGNED)
        signRx = true;

    tryFinalizeRxSigFragments();
    maybeStartRxAudio();
    return true;
}

void OpMode_Horse::sendTxVoiceFrame(const uint8_t *melpe, bool isLast,
                                    horse::frame_t &outFrame)
{
    uint8_t voiceTag[HORSE_VOICE_TAG_BYTES] = { 0 };
    memcpy(txPayload, melpe, sizeof txPayload);

    uint16_t fn = encoder.currentVoiceFrameNumber();
    if (encryptTx && sessionValid) {
        horse_crypto_voice_nonce_from_fn(fn, txNonce);
        if (!horse_crypto_req_voice_enc(sessionKey, frameAuthKey,
                                        HORSE_VOICE_DIR_FORWARD, fn, txNonce,
                                        txPayload, HORSE_CODEC_FRAME_BYTES)
            || !horse_wait_job(true)
            || !horse_crypto_take_voice_enc(txCipher, voiceTag,
                                            HORSE_CODEC_FRAME_BYTES)) {
            memset(txCipher, 0, sizeof txCipher);
            memset(voiceTag, 0, sizeof voiceTag);
        }
        encoder.encodeVoiceFrame(txCipher, voiceTag, outFrame, isLast);
    } else if (sessionValid) {
        if (!horse_crypto_req_voice_tag(frameAuthKey, HORSE_VOICE_DIR_FORWARD,
                                        fn, txPayload)
            || !horse_wait_job(true)
            || !horse_crypto_take_voice_tag(voiceTag))
            memset(voiceTag, 0, sizeof voiceTag);
        encoder.encodeVoiceFrame(txPayload, voiceTag, outFrame, isLast);
    } else
        encoder.encodeVoiceFrame(txPayload, voiceTag, outFrame, isLast);
}

void OpMode_Horse::enable()
{
    horse_crypto_worker_init();
    codec_init();
    horse_codec_init();
    modulator.init();
    demodulator.init();
    demodulator.setDropWithoutTag(true);
    locked = false;
    dataValid = false;
    startRx = true;
    startTx = false;
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
    horse_crypto_cancel();
    horse_crypto_worker_terminate();
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

void OpMode_Horse::update(rtxStatus_t *const status, const bool newCfg)
{
    (void)newCfg;
    switch (status->opStatus) {
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
    switch (status->opStatus) {
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

void OpMode_Horse::offState(rtxStatus_t *const status)
{
    radio_disableRtx();
    codec_stop(txAudioPath);
    audioPath_release(txAudioPath);
    if (startRx) {
        status->opStatus = RX;
        return;
    }
    if (platform_getPttStatus() && (status->txDisable == 0)) {
        startTx = true;
        status->opStatus = TX;
        return;
    }
    sleepFor(0, 30);
}

void OpMode_Horse::rxState(rtxStatus_t *const status)
{
    if (startRx) {
        demodulator.startBasebandSampling();
        radio_enableRx();
        startRx = false;
    }
    bool newData = demodulator.update(invertRxPhase);
    bool lock = demodulator.isLocked();
    if (lock && !locked) {
        decoder.reset();
        locked = lock;
    }
    if (locked) {
        if (newData) {
            const frame_t &frame = demodulator.getFrame();
            demodulator.takeSoftBits(rxSoft);
            decoder.setAuthenticated(demodulator.lockAuthenticated());
            HorseFrameType type = decoder.decodeFrame(frame, rxSoft);
            (void)applyLsfIfReady(status);
            if (type == HorseFrameType::VOICE) {
                uint16_t fn = 0;
                decoder.getVoicePayload(frame, rxMelpe, rxTag, &fn);
                tryFinalizeRxSigFragments();

                bool drop_voice = false;
                if (horse_is_sig_frame(fn)) {
                    uint16_t chunk = fn - SIG_FRAME_BASE;
                    if (chunk < SIG_FRAME_COUNT) {
                        horse::horse_sig_store_chunk(rxSessionSig, chunk,
                                                     rxMelpe);
                        rxSigChunks++;
                        tryFinalizeRxSessionSig();
                        maybeStartRxAudio();
                    }
                    drop_voice = true;
                } else if (!horse_rx_may_output_voice(
                               (rxLsfFlags & LSF_FLAG_ENCRYPTED) != 0,
                               sessionValid,
                               (rxLsfFlags & LSF_FLAG_SIGNED) != 0, rxSigReady,
                               state.channel.horse.encrypt_en,
                               state.channel.horse.sign_en)) {
                    drop_voice = true;
                } else if (encryptRx && sessionValid) {
                    horse_crypto_voice_nonce_from_fn(fn, rxNonce);
                    if (!horse_crypto_req_voice_dec(
                            sessionKey, frameAuthKey, HORSE_VOICE_DIR_FORWARD,
                            fn, rxNonce, rxMelpe, HORSE_CODEC_FRAME_BYTES,
                            rxTag)
                        || !horse_wait_job(false)
                        || !horse_crypto_take_voice_dec(rxPlain,
                                                        HORSE_CODEC_FRAME_BYTES))
                        drop_voice = true;
                    else {
                        memcpy(rxMelpe, rxPlain, sizeof rxMelpe);
                        demodulator.noteValidTag();
                    }
                } else if (sessionValid) {
                    if (!horse_crypto_req_voice_verify(
                            frameAuthKey, HORSE_VOICE_DIR_FORWARD, fn, rxMelpe,
                            rxTag)
                        || !horse_wait_job(false)
                        || horse_crypto_poll() != HORSE_CRYPTO_ST_DONE)
                        drop_voice = true;
                    else
                        demodulator.noteValidTag();
                } else
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
                    horse_codec_pushFrame(rxMelpe, false);
            }
        }
    }
    locked = lock;
    if (platform_getPttStatus()) {
        demodulator.stopBasebandSampling();
        locked = false;
        status->opStatus = OFF;
    }
    if (!locked) {
        status->horseLsfOk = false;
        dataValid = false;
        status->horse_dst[0] = '\0';
        status->horse_src[0] = '\0';
        resetRxCrypto();
        if (rxAudioPath >= 0) {
            horse_codec_stop(rxAudioPath);
            audioPath_release(rxAudioPath);
            rxAudioPath = -1;
        }
    }
}

void OpMode_Horse::abortTx(rtxStatus_t *const status, bool stop_mod)
{
    horse_crypto_cancel();
    if (stop_mod)
        modulator.stop();
    if (txAudioPath >= 0) {
        horse_codec_stop(txAudioPath);
        audioPath_release(txAudioPath);
        txAudioPath = -1;
    }
    radio_disableRtx();
    status->opStatus = OFF;
}

void OpMode_Horse::txState(rtxStatus_t *const status)
{
    horse::frame_t &outFrame = txOutFrame;
    if (startTx) {
        startTx = false;
        txAudioPath = audioPath_request(SOURCE_MIC, SINK_RTX, PRIO_TX);
        if (txAudioPath < 0) {
            abortTx(status, false);
            return;
        }
        if (!horse_codec_startEncode(txAudioPath)) {
            abortTx(status, false);
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

        memset(txEphPk, 0, sizeof txEphPk);
        memset(txEphSk, 0, sizeof txEphSk);
        uint8_t flags = 0;
        memset(&txPeer, 0, sizeof txPeer);
        memset(&txId, 0, sizeof txId);
        bool have_peer = false;
        bool want_encrypt = state.channel.horse.encrypt_en;
        bool want_sign = state.channel.horse.sign_en;

        if (!want_encrypt && !want_sign)
            want_encrypt = true;

        if (state.channel.horse.contact_index != 0
            && horse_peer_read(state.channel.horse.contact_index, &txPeer))
            have_peer = true;

        const bool have_id = horse_keystore_copy_identity(&txId);
        if (!horse_tx_allowed(want_encrypt, want_sign, horse_crypto_available(),
                              have_id,
                              have_peer && horse_peer_has_x25519(&txPeer),
                              have_peer && horse_peer_has_ed25519(&txPeer))) {
            if (!horse_crypto_available())
                status->horseError = HORSE_ERR_NO_CRYPTO;
            else
                status->horseError = HORSE_ERR_NO_KEYS;
            horse_crypto_memzero(&txId, sizeof txId);
            abortTx(status, false);
            return;
        }
        status->horseError = HORSE_ERR_NONE;

        if (!horse_crypto_req_x25519_keypair() || !horse_wait_job(true)
            || !horse_crypto_take_keypair(txEphPk, txEphSk)) {
            status->horseError = HORSE_ERR_NO_KEYS;
            horse_crypto_memzero(txEphSk, sizeof txEphSk);
            horse_crypto_memzero(&txId, sizeof txId);
            abortTx(status, false);
            return;
        }
        if (want_encrypt) {
            encryptTx = true;
            flags |= LSF_FLAG_ENCRYPTED;
        }
        if (want_sign) {
            flags |= LSF_FLAG_SIGNED;
            horse_crypto_build_session_message(srcCall.data(), dstCall.data(),
                                               txEphPk, flags,
                                               HORSE_LSF_VERSION, txSessionMsg);
            if (horse_crypto_req_sign(txId.ed25519_sk, txSessionMsg,
                                      sizeof txSessionMsg)
                && horse_wait_job(true)
                && horse_crypto_take_sig(txSessionSig))
                signTx = true;
            else
                flags = static_cast<uint8_t>(flags & ~LSF_FLAG_SIGNED);
        }
        if (!horse_crypto_req_derive(txEphSk, txPeer.x25519_pk, srcCall.data(),
                                     dstCall.data(), txEphPk, flags,
                                     HORSE_LSF_VERSION)
            || !horse_wait_job(true)
            || !horse_crypto_take_session(sessionKey, frameAuthKey)) {
            status->horseError = HORSE_ERR_NO_KEYS;
            horse_crypto_memzero(txEphSk, sizeof txEphSk);
            horse_crypto_memzero(&txId, sizeof txId);
            abortTx(status, false);
            return;
        }
        sessionValid = true;
        horse_crypto_memzero(txEphSk, sizeof txEphSk);
        horse_crypto_memzero(&txId, sizeof txId);

        if ((want_encrypt && !encryptTx) || (want_sign && !signTx)
            || !sessionValid) {
            status->horseError = HORSE_ERR_NO_KEYS;
            abortTx(status, false);
            return;
        }

        radio_enableTx();
        encoder.encodeLsf(srcCall, dstCall, txEphPk, flags, txLsfFrames);
        if (signTx)
            encoder.setSignatureFragments(txSessionSig);
        else
            encoder.setSignatureFragments(nullptr);
        modulator.invertPhase(invertTxPhase);
        if (!modulator.start()) {
            abortTx(status, false);
            return;
        }
        modulator.sendPreamble();
        for (size_t i = 0; i < LSF_OPENING_FRAMES; i++) {
            modulator.sendFrame(txLsfFrames[i]);
            sleepFor(0u, 40u);
        }

        if (signTx) {
            memset(txZeroTag, 0, sizeof txZeroTag);
            for (uint16_t i = 0; i < SIG_FRAME_COUNT; i++) {
                const size_t n = horse::sig_chunk_bytes(i);
                encoder.encodeVoiceFrameWithFn(
                    txSessionSig + (i * horse::SIG_CHUNK_BYTES), txZeroTag,
                    SIG_FRAME_BASE + i, outFrame, false, n);
                modulator.sendFrame(outFrame);
                sleepFor(0u, 40u);
            }
            txSigSent = true;
        }
    }

    uint16_t nextFn = encoder.currentVoiceFrameNumber();
    if (nextFn > VOICE_FN_MAX) {
        encoder.encodeEotFrame(outFrame);
        modulator.sendFrame(outFrame);
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
        status->horseError = HORSE_ERR_CALL_LIMIT;
        abortTx(status, true);
        return;
    }
    const bool last_legal = (nextFn == VOICE_FN_MAX);

    if (horse_codec_popFrame(txMelpe, true) != 0)
        memset(txMelpe, 0, sizeof(txMelpe));

    sendTxVoiceFrame(txMelpe, last_legal, outFrame);
    modulator.sendFrame(outFrame);
    sleepFor(0u, 40u);

    if (last_legal) {
        encoder.encodeEotFrame(outFrame);
        modulator.sendFrame(outFrame);
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
        status->horseError = HORSE_ERR_CALL_LIMIT;
        abortTx(status, true);
        return;
    }

    if (!platform_getPttStatus()) {
        if (horse_codec_popFrame(txMelpe, false) != 0)
            memset(txMelpe, 0, sizeof(txMelpe));
        sendTxVoiceFrame(txMelpe, true, outFrame);
        modulator.sendFrame(outFrame);
        encoder.encodeEotFrame(outFrame);
        modulator.sendFrame(outFrame);
        sessionValid = false;
        encryptTx = false;
        signTx = false;
        txSigSent = false;
        horse_crypto_memzero(sessionKey, sizeof sessionKey);
        horse_crypto_memzero(frameAuthKey, sizeof frameAuthKey);
        horse_crypto_memzero(txSessionSig, sizeof txSessionSig);
        abortTx(status, true);
    }
}
