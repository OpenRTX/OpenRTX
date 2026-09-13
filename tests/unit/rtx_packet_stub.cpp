/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include "rtx_packet_stub.h"

namespace
{

int rxResult = 0;
int txResult = 0;
struct pktDesc *rxDesc = nullptr;
struct pktDesc *txDesc = nullptr;
size_t rxSubmissions = 0;
size_t txSubmissions = 0;

/* Mirror the descriptor checks and state changes done by rtx.cpp. */
int submit(struct pktDesc *packet, int result, struct pktDesc **last,
           size_t *count)
{
    if (packet->status != PKT_STATUS_IDLE)
        return -EPERM;

    if ((packet->buffer == nullptr) || (packet->size == 0))
        return -EINVAL;

    if (result != 0)
        return result;

    packet->status = PKT_STATUS_SUBMITTED;
    packet->res = 0;
    *last = packet;
    (*count)++;

    return 0;
}

} // namespace

void rtxStub_reset()
{
    rxResult = 0;
    txResult = 0;
    rxDesc = nullptr;
    txDesc = nullptr;
    rxSubmissions = 0;
    txSubmissions = 0;
}

void rtxStub_setRxResult(int ret)
{
    rxResult = ret;
}

void rtxStub_setTxResult(int ret)
{
    txResult = ret;
}

struct pktDesc *rtxStub_rxDesc()
{
    return rxDesc;
}

struct pktDesc *rtxStub_txDesc()
{
    return txDesc;
}

size_t rtxStub_rxSubmissions()
{
    return rxSubmissions;
}

size_t rtxStub_txSubmissions()
{
    return txSubmissions;
}

extern "C" {

int rtx_addPacketRx(struct pktDesc *packet)
{
    return submit(packet, rxResult, &rxDesc, &rxSubmissions);
}

int rtx_addPacketTx(struct pktDesc *packet)
{
    return submit(packet, txResult, &txDesc, &txSubmissions);
}

} /* extern "C" */
