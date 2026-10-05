/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Horse identity keystore: encrypted storage and runtime unlock.
 */

#ifndef HORSE_KEYSTORE_H
#define HORSE_KEYSTORE_H

#include "protocols/horse/horse_crypto.h"
#include <stddef.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void horse_keystore_init(void);
void horse_keystore_terminate(void);

bool horse_keystore_is_unlocked(void);
bool horse_keystore_copy_identity(horse_identity_keys_t *out);

bool horse_keystore_unlock(const char *passphrase, size_t passphrase_len);
void horse_keystore_lock(void);

bool horse_keystore_store_plaintext(const horse_identity_keys_t *identity,
                                    const char *passphrase,
                                    size_t passphrase_len);

bool horse_keystore_fingerprint(uint8_t fp_out[32]);

#ifdef __cplusplus
}
#endif

#endif
