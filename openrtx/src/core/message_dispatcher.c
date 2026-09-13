/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <string.h>
#include "core/message_dispatcher.h"
#include "core/utils.h"
#include "rtx/rtx.h"

/* Number of receive descriptors kept submitted to the rtx stage. */
#ifndef CONFIG_MESSAGES_RX_SLOTS
#define CONFIG_MESSAGES_RX_SLOTS 1
#endif

/**
 * \internal
 * A packet descriptor with its buffer and the operating mode it was
 * submitted to, which selects the source that handles it on completion.
 */
struct pktSlot {
    struct pktDesc desc;
    uint8_t mode;
    uint8_t buffer[MSG_PKT_MAX_SIZE];
};

static const struct message_ops *sources[CONFIG_MESSAGES_MAX_SOURCES];
static size_t numSources;
static struct pktSlot rxSlots[CONFIG_MESSAGES_RX_SLOTS];
static struct pktSlot txSlot;
static uint32_t txSequence;

/**
 * \internal
 * Drive one receive slot: hand a completed packet to its source, then keep
 * the slot submitted while a source is registered for the current mode.
 */
static void handleRx(struct pktSlot *slot, uint8_t mode)
{
    const struct message_ops *ops;

    switch (slot->desc.status) {
        case PKT_STATUS_DONE:
            ops = message_dispatcher_source(slot->mode);
            if (ops != NULL)
                ops->process_rx(&slot->desc);

            /* fallthrough */
        case PKT_STATUS_ERROR:
            slot->desc.status = PKT_STATUS_IDLE;

            /* fallthrough */
        case PKT_STATUS_IDLE:
            if (message_dispatcher_source(mode) == NULL)
                break;

            slot->desc.buffer = slot->buffer;
            slot->desc.size = sizeof(slot->buffer);
            slot->mode = mode;
            rtx_addPacketRx(&slot->desc);
            break;

        case PKT_STATUS_SUBMITTED:
            break;
    }
}

/**
 * \internal
 * Report the outcome of a completed transmission to the registry and free
 * the transmit slot.
 */
static void handleTx(void)
{
    switch (txSlot.desc.status) {
        case PKT_STATUS_DONE:
            messages_set_status(txSequence, MSG_STATUS_SENT);
            txSlot.desc.status = PKT_STATUS_IDLE;
            break;

        case PKT_STATUS_ERROR:
            messages_set_status(txSequence, MSG_STATUS_FAILED);
            txSlot.desc.status = PKT_STATUS_IDLE;
            break;

        case PKT_STATUS_IDLE:
        case PKT_STATUS_SUBMITTED:
            break;
    }
}

void message_dispatcher_init(void)
{
    memset(sources, 0, sizeof(sources));
    numSources = 0;

    for (size_t i = 0; i < ARRAY_SIZE(rxSlots); i++)
        rxSlots[i].desc.status = PKT_STATUS_IDLE;

    txSlot.desc.status = PKT_STATUS_IDLE;
    txSequence = 0;
}

int message_dispatcher_register(const struct message_ops *ops)
{
    if ((ops == NULL) || (ops->process_rx == NULL) || (ops->format_tx == NULL))
        return -EINVAL;

    if (message_dispatcher_source(ops->mode) != NULL)
        return -EEXIST;

    if (numSources == ARRAY_SIZE(sources))
        return -ENOSPC;

    sources[numSources++] = ops;
    return 0;
}

const struct message_ops *message_dispatcher_source(uint8_t mode)
{
    for (size_t i = 0; i < numSources; i++) {
        if (sources[i]->mode == mode)
            return sources[i];
    }

    return NULL;
}

void message_dispatcher_task(uint8_t mode)
{
    for (size_t i = 0; i < ARRAY_SIZE(rxSlots); i++)
        handleRx(&rxSlots[i], mode);

    handleTx();
}

int message_dispatcher_send(const struct message *msg)
{
    if (msg == NULL)
        return -EINVAL;

    const struct message_ops *ops = message_dispatcher_source(msg->mode);
    if (ops == NULL)
        return -ENOENT;

    if (txSlot.desc.status != PKT_STATUS_IDLE)
        return -EBUSY;

    txSlot.desc.buffer = txSlot.buffer;
    txSlot.desc.size = sizeof(txSlot.buffer);

    int ret = ops->format_tx(msg, &txSlot.desc);
    if (ret != 0)
        return ret;

    txSlot.mode = msg->mode;
    txSequence = msg->sequence;

    return rtx_addPacketTx(&txSlot.desc);
}
