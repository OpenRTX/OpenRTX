/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include "protocols/APRS/packet_list.h"
#include "protocols/APRS/constants.h"
#include "core/packet_engine.h"
#include "core/utils.h"
#include "core/state.h"
#include "core/slip.h"
#include "rtx/rtx.h"

struct packetElement {
    void *payload;
    struct pktDesc packet;
};

static struct packetElement pe[2];

static void sendKissFrame(uint8_t *payload, size_t size)
{
    uint8_t txBuf[256];
    struct slip kissFrame;
    int ret;

    payload[0] = 0;
    slip_init(&kissFrame, payload, size + 1);

    while ((ret = slip_encode(&kissFrame, txBuf, sizeof(txBuf))) > 0)
        write(STDOUT_FILENO, txBuf, ret);
}

static void handleRxPacket(struct packetElement *pe)
{
    struct aprsPacket *packet;

    switch (pe->packet.status) {
        case PKT_STATUS_IDLE:
            pe->packet.size = APRS_PACLEN;
            rtx_addPacketRx(&pe->packet);
            break;

        case PKT_STATUS_SUBMITTED:
            break;

        case PKT_STATUS_DONE:
            packet = aprsPktFromFrame(pe->packet.buffer, pe->packet.size);
            state.aprsStoredPkts = aprsPktList_insert(state.aprsStoredPkts,
                                                      packet);
            sendKissFrame(pe->payload, pe->packet.size);

            /* fallthrough */
        case PKT_STATUS_ERROR:
            pe->packet.status = PKT_STATUS_IDLE;
            break;
    }
}

int packetEngine_init()
{
    for (size_t i = 0; i < ARRAY_SIZE(pe); i++) {
        void *buf = malloc(APRS_PACLEN + 1);
        if (!buf) {
            packetEngine_terminate();
            return -ENOMEM;
        }

        pe[i].payload = buf;
        pe[i].packet.buffer = ((uint8_t *)buf) + 1;
        pe[i].packet.size = APRS_PACLEN;
        pe[i].packet.status = PKT_STATUS_IDLE;
    }

    return 0;
}

void packetEngine_terminate()
{
    for (size_t i = 0; i < ARRAY_SIZE(pe); i++)
        free(pe[i].payload);
}

void packetEngine_task()
{
    for (size_t i = 0; i < ARRAY_SIZE(pe); i++)
        handleRxPacket(&pe[i]);
}
