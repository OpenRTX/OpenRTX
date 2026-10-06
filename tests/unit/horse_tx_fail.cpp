/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Encryption failure on TX must not send zeros. The RTX path encodes a
 * proper EOT, unkeys, and reports HORSE_ERR_TX_CRYPTO.
 */

#include "protocols/horse/HorseFrameEncoder.hpp"
#include "protocols/horse/HorseConstants.hpp"
#include "protocols/horse/horse_crypto.h"
#include "protocols/horse/horse_crypto_worker.h"
#include "core/horse_codec.h"
#include "rtx/rtx.h"
#include <cstdint>
#include <cstdio>
#include <cstring>

int horse_test_tx_crypto_fail(void);

static int test_eot_sync(void)
{
    horse::HorseFrameEncoder enc;
    horse::frame_t f;

    enc.encodeEotFrame(f);
    if (f[0] != horse::EOT_SYNC_WORD[0] || f[1] != horse::EOT_SYNC_WORD[1]) {
        std::printf("horse_tx_fail: EOT sync %02x %02x\n", f[0], f[1]);
        return -1;
    }
    return 0;
}

static int test_worker_dead_enc_fails(void)
{
    uint8_t k[32];
    uint8_t n[12];
    uint8_t pt[HORSE_CODEC_FRAME_BYTES];

    memset(k, 1, sizeof k);
    memset(n, 2, sizeof n);
    memset(pt, 3, sizeof pt);
    horse_crypto_worker_terminate();
    if (horse_crypto_req_voice_enc(k, k, HORSE_VOICE_DIR_FORWARD, 1, n, pt,
                                   sizeof pt)) {
        std::printf("horse_tx_fail: enc succeeded without worker\n");
        return -1;
    }
    return 0;
}

int main(void)
{
    int rc;

    rc = test_eot_sync();
    if (rc != 0)
        return rc;
    rc = test_worker_dead_enc_fails();
    if (rc != 0)
        return rc;
    rc = horse_test_tx_crypto_fail();
    if (rc != 0) {
        std::printf("horse_tx_fail: opmode path %d\n", rc);
        return rc;
    }
    if (HORSE_ERR_TX_CRYPTO != 4) {
        std::printf("horse_tx_fail: HORSE_ERR_TX_CRYPTO is not 4\n");
        return -5;
    }
    return 0;
}
