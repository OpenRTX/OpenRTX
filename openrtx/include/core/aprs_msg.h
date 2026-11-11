/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef APRS_MSG_H
#define APRS_MSG_H

#include "core/messages.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Protocol source translating between APRS messages and message inbox
 * entries, to be registered with messages_registerSource(). Only messages
 * addressed to the APRS callsign in the settings are stored; transmit is not
 * supported yet.
 */
extern const struct messageOps aprs_msg_ops;

#ifdef __cplusplus
}
#endif

#endif /* APRS_MSG_H */
