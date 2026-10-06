/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#ifndef RTX_PACKET_STUB_H
#define RTX_PACKET_STUB_H

#include "rtx/rtx.h"

/*
 * Stand-in for the rtx stage packet interface, for tests exercising code
 * that submits packet descriptors. Submissions are recorded rather than
 * acted upon; the test completes a descriptor by filling its buffer and
 * setting its status, just as an opmode would.
 */

/* Forget every recorded submission and restore the default return codes. */
void rtxStub_reset();

/* Return code for the next rtx_addPacketRx() / rtx_addPacketTx() calls. */
void rtxStub_setRxResult(int ret);
void rtxStub_setTxResult(int ret);

/* Last descriptor submitted, or nullptr. */
struct pktDesc *rtxStub_rxDesc();
struct pktDesc *rtxStub_txDesc();

/* Number of successful submissions since the last reset. */
size_t rtxStub_rxSubmissions();
size_t rtxStub_txSubmissions();

#endif /* RTX_PACKET_STUB_H */
