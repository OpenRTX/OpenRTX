/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_SMS_H
#define M17_SMS_H

#include "core/messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Protocol source translating between M17 SMS packets and message inbox
 * entries, to be registered with messages_registerSource().
 */
extern const struct messageOps m17_sms_ops;

#ifdef __cplusplus
}
#endif

#endif /* M17_SMS_H */
