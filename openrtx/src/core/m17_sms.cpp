/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include <cerrno>
#include <cstddef>
#include <cstring>
#include <algorithm>
#include "core/m17_sms.h"
#include "core/messages.h"
#include "protocols/M17/SmsPacket.hpp"
#include "protocols/M17/m17.h"
#include "rtx/rtx.h"

/*
 * An M17 packet buffer carries the source and destination callsigns, as
 * NUL-padded 10-byte fields, in front of the packet payload. On reception
 * pktDesc::res holds the payload length.
 */
static constexpr size_t M17_PKT_HEADER = offsetof(struct m17Packet, payload);

static_assert(sizeof(struct m17Packet) <= MSG_PKT_MAX_SIZE,
              "M17 packets must fit in an inbox packet buffer");
static_assert(sizeof(m17Packet::src) <= MSG_ADDR_MAX_LEN,
              "M17 callsigns must fit in a message address");

/**
 * \internal
 * Copy a callsign between a message address field and a fixed-width M17
 * packet callsign field, in either direction: copy up to the field width,
 * zero-padding any remainder, and keep the destination NUL-terminated.
 */
static void copyCallsign(char *dst, const char *src)
{
    strncpy(dst, src, sizeof(m17Packet::src));
    dst[sizeof(m17Packet::src) - 1] = '\0';
}

static int processRx(const struct pktDesc *pkt)
{
    if ((pkt->buffer == nullptr) || (pkt->res <= 0))
        return -EINVAL;

    auto *packet = static_cast<const struct m17Packet *>(pkt->buffer);
    size_t textLen = 0;
    const char *text = M17::sms_packet_text(packet->payload, pkt->res,
                                            &textLen);
    if (text == nullptr)
        return -EPROTO;

    struct message msg = {};
    msg.body = text;
    msg.bodyLen = textLen;
    msg.mode = OPMODE_M17;
    msg.direction = MSG_DIR_RX;
    msg.status = MSG_STATUS_RECEIVED;
    msg.unread = 1;
    copyCallsign(msg.sender, packet->src);
    copyCallsign(msg.recipient, packet->dst);

    int32_t seq = messages_store(&msg);
    return (seq < 0) ? seq : 0;
}

static int formatTx(const struct message *msg, struct pktDesc *pkt)
{
    if ((pkt->buffer == nullptr) || (pkt->size <= M17_PKT_HEADER))
        return -EINVAL;

    if (msg->bodyLen == 0)
        return -EINVAL;

    auto *packet = static_cast<struct m17Packet *>(pkt->buffer);
    size_t capacity = std::max(pkt->size - M17_PKT_HEADER, M17_MAX_PACKET_DATA);

    size_t len = M17::sms_format_packet(msg->body, msg->bodyLen,
                                        packet->payload, capacity);
    if (len == 0)
        return -EMSGSIZE;

    copyCallsign(packet->src, msg->sender);
    copyCallsign(packet->dst, msg->recipient);
    pkt->size = M17_PKT_HEADER + len;

    return 0;
}

const struct messageOps m17_sms_ops = {
    processRx,
    formatTx,
    "M17 SMS",
    OPMODE_M17,
};
