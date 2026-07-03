/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
 * Loopback stubs for m17_sms_loopback_test.
 *
 * Key difference from m17_sms_stubs.cpp: rtx_addPacketRx() saves the
 * pktDesc pointer so the test can write a formatted SMS into the live
 * RX buffer and mark the descriptor PKT_STATUS_DONE.
 */

#include <cstring>

extern "C" {
#include "rtx/rtx.h"
#include "protocols/M17/m17.h"
#include "core/messages.h"
#include "core/state.h"
}

/*
 * m17_sms.cpp (sms_send) reads the local callsign from state.settings.callsign
 * on the UI thread rather than through rtx_getStatus(), so the source needs the
 * global radio state to link.  Zero-initialised here, with the loopback
 * identity applied at static-init time so TX entries carry the expected sender,
 * matching rtx_getStatus() below.
 */
extern "C" {
state_t state;
}

namespace
{
struct StateInit {
    StateInit()
    {
        strcpy(state.settings.callsign, "W1AW");
    }
} g_state_init;
} /* namespace */

static struct pktDesc *g_rx_desc = nullptr;
static struct pktDesc *g_tx_desc = nullptr;
static char g_tx_captured_destination[10];
static uint32_t loopback_next_seq = 1;

extern "C" {

uint32_t messages_alloc_sequence(void)
{
    return loopback_next_seq++;
}

/**
 * Return the last pktDesc pointer registered via rtx_addPacketRx().
 * Valid after the first m17_sms_task_rtx() call following m17_sms_init().
 */
struct pktDesc *loopback_get_rx_desc(void)
{
    return g_rx_desc;
}

/**
 * Return the last pktDesc pointer passed to rtx_addPacketTx().
 * Valid after the first m17_sms_send() call.
 */
struct pktDesc *loopback_get_tx_desc(void)
{
    return g_tx_desc;
}

/**
 * Return the destination callsign captured from the m17Packet.dst field
 * at TX submission time.  Valid after rtx_addPacketTx().
 */
const char *loopback_get_tx_destination(void)
{
    return g_tx_captured_destination;
}

int rtx_addPacketRx(struct pktDesc *pkt)
{
    g_rx_desc = pkt;
    if (pkt != nullptr)
        pkt->status = PKT_STATUS_IDLE;
    return 0;
}

static rtxStatus_t g_rtx_status;
static bool g_rtx_status_init = false;

int rtx_addPacketTx(struct pktDesc *pkt)
{
    g_tx_desc = pkt;
    if (pkt != nullptr && pkt->buffer != nullptr) {
        const struct m17Packet *mpkt =
            static_cast<const struct m17Packet *>(pkt->buffer);
        strncpy(g_tx_captured_destination, mpkt->dst,
                sizeof(g_tx_captured_destination) - 1);
        g_tx_captured_destination[sizeof(g_tx_captured_destination) - 1] = '\0';
    } else {
        memset(g_tx_captured_destination, 0, sizeof(g_tx_captured_destination));
    }
    if (pkt != nullptr)
        pkt->status = PKT_STATUS_DONE;
    return 0;
}

const rtxStatus_t *rtx_getStatus(void)
{
    if (!g_rtx_status_init) {
        memset(&g_rtx_status, 0, sizeof(g_rtx_status));
        strcpy(g_rtx_status.source_address, "W1AW");
        strcpy(g_rtx_status.M17_src, "W1AW");
        g_rtx_status_init = true;
    }
    return &g_rtx_status;
}

} /* extern "C" */
