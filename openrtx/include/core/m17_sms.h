/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef M17_SMS_H
#define M17_SMS_H

#include "core/messages.h"
#include <stddef.h>

/* Default sizing tier, used by most targets. Targets with more RAM (e.g.
 * CS7000/CS7000-PLUS/linux) define larger values in their own hwconfig.h,
 * which take precedence over these fallbacks. */
#ifndef M17_SMS_MAX_MESSAGES
#define M17_SMS_MAX_MESSAGES 32
#endif
#ifndef M17_SMS_POOL_BYTES
#define M17_SMS_POOL_BYTES 3200
#endif

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Initialise the M17 SMS source.  Called once from rtx_threadFunc before
 * the RTX loop starts.  Prepares packet descriptors and entry pool.
 */
void m17_sms_init(void);

/**
 * RTX-thread poll function.  Called from rtx_threadFunc every iteration.
 *
 * Checks for a pending struct pkt_tx_request from the UI thread, formats and
 * submits it via rtx_addPacketTx(), and monitors completion.  Arms the RX
 * descriptor and ingests completed RX packets, posting struct pkt_rx_event back
 * to the UI thread via packet_io_enqueue_rx().
 */
void m17_sms_task_rtx(void);

/** Ops instance for the M17 SMS source. Referenced by messages.cpp. */
extern const struct message_ops m17_sms_ops;

#ifdef __cplusplus
}
#endif

#endif /* M17_SMS_H */
