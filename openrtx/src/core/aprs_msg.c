/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <errno.h>
#include <stdbool.h>
#include <string.h>
#include "core/aprs_msg.h"
#include "core/crc.h"
#include "core/messages.h"
#include "core/packet_engine.h"
#include "core/state.h"
#include "interfaces/delays.h"
#include "protocols/APRS/packet.h"
#include "rtx/rtx.h"

#if APRS_PACLEN > MSG_PKT_MAX_SIZE
#error "An AX.25 frame must fit in an inbox packet buffer"
#endif

#if APRS_ADDR_STR_LEN > MSG_ADDR_MAX_LEN
#error "A formatted APRS address must fit in a message address"
#endif

/**
 * Copies of one message are stored once. A numbered message is repeated by
 * its sender until it is acknowledged; any message is repeated by the
 * digipeaters that relay it. Times in getTick() milliseconds.
 */
#define HEARD_ENTRIES 8
#define HEARD_NUMBERED_WINDOW (30 * 60 * 1000)
#define HEARD_PLAIN_WINDOW (30 * 1000)

/**
 * A message recently stored.
 */
struct heardMsg {
    long long time; ///< When its latest copy was heard.
    uint32_t key;   ///< CRC of the sender and of the info field.
    bool used;      ///< Entry holds a message.
};

static struct heardMsg heard[HEARD_ENTRIES];
static size_t heardNext;

/**
 * Whether an addressee names this station. They are compared as addresses,
 * so "n0call-07" still reaches N0CALL-7, and another SSID does not.
 */
static bool addressedToMe(const char *addressee)
{
    struct aprsAddress to;
    struct aprsAddress me;

    if ((aprsAddr_fromStr(addressee, &to) != 0)
        || (aprsAddr_fromStr(state.settings.aprs_callsign, &me) != 0))
        return false;

    return (strcmp(to.addr, me.addr) == 0) && (to.ssid == me.ssid);
}

/**
 * Whether a message for this station is a copy of one already stored, and
 * remember it if it is not. The info field holds the addressee, the text and
 * the message number, so equal fields from the same sender are the same
 * message.
 */
static bool isRepeat(const char *sender, const struct aprsPacket *pkt)
{
    const long long now = getTick();
    const uint32_t key = ((uint32_t)crc_ccitt(sender, strlen(sender)) << 16)
                       | crc_ccitt(pkt->info, pkt->infoLen);
    const long long window = (memchr(pkt->info, '{', pkt->infoLen) != NULL) ?
                                 HEARD_NUMBERED_WINDOW :
                                 HEARD_PLAIN_WINDOW;

    for (size_t i = 0; i < HEARD_ENTRIES; i++) {
        struct heardMsg *h = &heard[i];

        if ((h->used) && (h->key == key) && ((now - h->time) < window)) {
            h->time = now;
            return true;
        }
    }

    heard[heardNext].time = now;
    heard[heardNext].key = key;
    heard[heardNext].used = true;
    heardNext = (heardNext + 1) % HEARD_ENTRIES;

    return false;
}

static int processRx(const struct pktDesc *pkt)
{
    struct aprsPacket aprs;
    char addressee[APRS_ADDR_STR_LEN];
    char text[APRS_MSG_TEXT_MAX + 1];
    struct message msg;
    int32_t seq;
    int ret;

    // A host on the virtual serial port sees every frame heard, stored or
    // not. With no host reading the frame is just not sent.
    packetEngine_send(pkt->buffer, (size_t)pkt->res);

    ret = aprsPkt_fromFrame(pkt->buffer, (size_t)pkt->res, &aprs);
    if (ret != 0)
        return ret;

    // Store only messages for this station: positions, objects and status,
    // most of the channel, would push them out of the inbox. The text is
    // what fits in an APRS message; anything past that is dropped.
    if (aprsMsg_unwrap(&aprs, addressee, sizeof(addressee), text, sizeof(text))
        != 0)
        return 0;

    if (!addressedToMe(addressee))
        return 0;

    memset(&msg, 0, sizeof(msg));
    aprsAddr_toStr(&aprs.addresses[APRS_ADDR_SRC], msg.sender,
                   sizeof(msg.sender));
    if (isRepeat(msg.sender, &aprs))
        return 0;

    msg.body = text;
    msg.bodyLen = (uint16_t)strlen(text);
    msg.type = (uint16_t)aprs.type;
    msg.mode = OPMODE_APRS;
    msg.direction = MSG_DIR_RX;
    msg.status = MSG_STATUS_RECEIVED;
    msg.unread = 1;
    memcpy(msg.recipient, addressee, sizeof(addressee));

    seq = messages_store(&msg);
    return (seq < 0) ? (int)seq : 0;
}

static int formatTx(const struct message *msg, struct pktDesc *pkt)
{
    (void)msg;
    (void)pkt;

    // APRS transmit is not implemented yet
    return -ENOTSUP;
}

const struct messageOps aprs_msg_ops = {
    .processRx = processRx,
    .formatTx = formatTx,
    .name = "APRS",
    .mode = OPMODE_APRS,
};
