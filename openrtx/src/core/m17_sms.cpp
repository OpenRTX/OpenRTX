/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

/**
  * m17_sms.cpp — M17 SMS message source for the generic message inbox.
  *
  * Thread model:
  *   RTX thread: m17_sms_task_rtx() — owns tx_pkt/tx_desc/rx_pkt/rx_desc.
  *     * Drains struct pkt_tx_request from packet_io; formats and submits pktDesc.
  *     * On TX completion posts a struct pkt_tx_done (status = SENT or FAILED).
  *     * On RX completion parses payload; posts struct pkt_rx_event (received text).
  *
  *   UI thread: sms_tick() (called via ops->tick from messages_tick())
  *     and sms_send() (called via ops->send from messages_send()).
  *     Both run on the UI thread and are the sole owners of entries[]/body_pool.
  *
  * packet_io provides the single mutex protecting the queue slots; it is held
  * only during memcpy, keeping contention negligible.
  */

#include "hwconfig.h"

#include "core/m17_sms.h"
#include "core/messages.h"
#include "core/packet_io.h"
#include "core/state.h"
#include "protocols/M17/m17.h"
#include "protocols/M17/SmsPacket.hpp"
#include "rtx/rtx.h"

#include <cerrno>
#ifdef PLATFORM_LINUX
#include <cstdio>
#endif
#include <cstdint>
#include <cstring>

/* m17Packet struct: src[10] + dst[10] + payload[M17_MAX_PACKET_DATA(=825)] = 845 bytes. */
static_assert(sizeof(struct m17Packet) == M17_MAX_PACKET_DATA + 20,
              "m17Packet must be exactly 20 header bytes + payload");

static_assert(M17_SMS_MAX_MESSAGES > 0, "M17_SMS_MAX_MESSAGES must be > 0");
static_assert(M17_SMS_POOL_BYTES >= PKT_BODY_MAX_LEN,
              "M17_SMS_POOL_BYTES must be >= PKT_BODY_MAX_LEN");
static_assert(M17_SMS_POOL_BYTES <= 65535,
              "M17_SMS_POOL_BYTES exceeds uint16_t range; "
              "reduce the value or widen pool_head / pool offsets");
static_assert(PKT_ADDR_MAX_LEN == MSG_ADDR_MAX_LEN,
              "packet_io address width must match the message header's");

/* ===================================================================
 * UI-thread storage (entries[], body_pool)
 * Owned exclusively by the UI thread; no lock needed.
 * =================================================================== */

/**
 * One stored SMS.  The struct message_header MUST be the first member so that
 * a ops get() return value can be reinterpret_cast'd back to SmsEntry.
 */
struct SmsEntry {
    struct message_header hdr; /**< Registry-visible header. Must be first. */
    uint16_t pool_offset;      /**< Byte offset of body text in body_pool. */
    uint16_t pool_len;         /**< Length of body text (not counting NUL). */
    bool active;               /**< Slot is in use. */
    uint32_t tag; /**< Echoed from struct pkt_tx_request for completion
                               matching; 0 for RX entries. */
};

static SmsEntry entries[M17_SMS_MAX_MESSAGES];

/**
 * Number of currently-active entries, maintained incrementally so sms_count()
 * need not rescan entries[] on every UI frame.  Every write of entries[i]
 * .active must go through the counter: incremented when a slot becomes active
 * (sms_store_entry), decremented on every eviction/delete (alloc_slot,
 * pool_alloc, MSG_ACTION_DELETE).  Reset to 0 in m17_sms_init().
 */
static size_t active_count = 0;

/**
 * Variable-length body text pool.  Bodies are stored as NUL-terminated
 * strings in a ring buffer.  pool_head points to the next byte to write.
 * When a new body would wrap or overwrite existing data, any active entry
 * whose pool region overlaps the write range is evicted first.
 */
static char body_pool[M17_SMS_POOL_BYTES];
static uint16_t pool_head = 0;

/* ===================================================================
 * RTX-thread storage (tx_pkt, tx_desc, rx_pkt, rx_desc)
 * Owned exclusively by the RTX thread; no lock needed.
 * =================================================================== */

static struct m17Packet tx_pkt;
static struct pktDesc tx_desc;
static uint32_t tx_inflight_tag = 0; /* tag of the in-flight TX request */

static struct m17Packet rx_pkt;
static struct pktDesc rx_desc;

/* Working buffers for m17_sms_task_rtx().
 * Declared at file scope because the RTX thread has a 512-byte stack on
 * embedded targets.  struct pkt_tx_request (~843 B) cannot be allocated as
 * a local variable in that context. */
static struct pkt_tx_request rtx_req;
static struct pkt_tx_done rtx_tx_done; /* TX completion retry buffer */
static struct pkt_rx_event rtx_rx_evt; /* RX received event retry buffer */
static bool rtx_tx_done_ready = false;
static bool rtx_rx_evt_ready = false;
static char rtx_rx_text[PKT_BODY_MAX_LEN];

/* ===================================================================
 * UI-thread internal helpers
 * =================================================================== */

/**
 * Allocate len+1 bytes in body_pool for a new body.
 *
 * Advances pool_head by len+1 (wrapping at M17_SMS_POOL_BYTES).  Any active
 * entry whose pool region overlaps the write range is evicted — those entries
 * are always older because pool_head only moves forward.
 *
 * @param len: length of the body text (not counting NUL).
 * @return byte offset in body_pool at which to write.
 */
static uint16_t pool_alloc(uint16_t len)
{
    uint16_t need = len + 1; /* +1 for NUL */

    /* Wrap to start if the write would run off the end. */
    if ((uint32_t)pool_head + need > M17_SMS_POOL_BYTES)
        pool_head = 0;

    uint16_t ws = pool_head;
    uint16_t we = pool_head + need;

    /* Evict any active entry whose body overlaps [ws, we).  All bodies are
     * non-wrapping sub-ranges of [0, M17_SMS_POOL_BYTES) because pool_alloc()
     * resets pool_head to 0 when a write would exceed the boundary.  Casts to
     * uint32_t prevent overflow if pool or body size constraints change. */
    for (size_t i = 0; i < M17_SMS_MAX_MESSAGES; i++) {
        if (!entries[i].active)
            continue;
        uint32_t es = entries[i].pool_offset;
        uint32_t ee = (uint32_t)entries[i].pool_offset + entries[i].pool_len
                    + 1;
        /* Overlap test for half-open intervals [ws, we) and [es, ee). */
        if ((uint32_t)es < we && ee > ws) {
            entries[i].active = false;
            active_count--;
        }
    }

    pool_head = we;
    return ws;
}

/**
 * Find a free (inactive) slot.
 * If the pool is full, evict the entry with the oldest sequence number.
 * @return index into entries[].
 */
static size_t alloc_slot(void)
{
    /* First pass: look for a free slot. */
    for (size_t i = 0; i < M17_SMS_MAX_MESSAGES; i++) {
        if (!entries[i].active)
            return i;
    }

    /* Pool full — evict the chronologically oldest entry (lowest sequence). */
    size_t oldest = 0;
    for (size_t i = 1; i < M17_SMS_MAX_MESSAGES; i++) {
        if (entries[i].hdr.sequence < entries[oldest].hdr.sequence) {
            oldest = i;
        }
    }

    entries[oldest].active = false;
    active_count--;
    return oldest;
}

/**
 * Allocate a slot and pool region for a new message body, copy it in, and
 * populate the fields common to both the RX and TX creation paths (pool
 * bookkeeping, sequence, body pointer/length, active flag). The caller
 * fills in the direction-specific fields (status, unread, sender,
 * recipient) afterward.
 *
 * @param body: message body to copy (not required to be NUL-terminated).
 * @param body_len: length of body, not counting NUL.
 * @param tag: TX completion tag, or 0 for RX entries (which are never
 *             matched by tag).
 * @return reference to the newly populated entry.
 */
static SmsEntry &sms_store_entry(const char *body, size_t body_len,
                                 uint32_t tag)
{
    size_t slot = alloc_slot();
    SmsEntry &e = entries[slot];

    uint16_t off = pool_alloc((uint16_t)body_len);
    memcpy(&body_pool[off], body, body_len);
    body_pool[off + body_len] = '\0';

    memset(&e.hdr, 0, sizeof(e.hdr));
    e.pool_offset = off;
    e.pool_len = (uint16_t)body_len;
    e.tag = tag;

    e.hdr.sequence = messages_alloc_sequence();
    e.hdr.body = &body_pool[off];
    e.hdr.body_len = body_len;
    e.active = true;
    active_count++;

    return e;
}

/* ===================================================================
 * Ops callbacks — all called from the UI thread
 * =================================================================== */

static size_t sms_count(void *ctx)
{
    (void)ctx;
    return active_count;
}

static struct message_header *sms_get(void *ctx, size_t idx)
{
    (void)ctx;
    size_t seen = 0;
    for (size_t i = 0; i < M17_SMS_MAX_MESSAGES; i++) {
        if (!entries[i].active)
            continue;
        if (seen == idx)
            return &entries[i].hdr;
        seen++;
    }
    return nullptr;
}

static uint32_t sms_supported_actions(const struct message_header *hdr)
{
    uint32_t acts = MSG_ACTION_VIEW | MSG_ACTION_DELETE;
    acts |= hdr->unread ? MSG_ACTION_MARK_READ : MSG_ACTION_MARK_UNREAD;
    if (hdr->direction == MSG_DIR_RX)
        acts |= MSG_ACTION_REPLY;
    return acts;
}

static int sms_invoke_action(struct message_header *hdr,
                             enum message_action action)
{
    SmsEntry *e = reinterpret_cast<SmsEntry *>(hdr);

    switch (action) {
        case MSG_ACTION_MARK_READ:
            hdr->unread = false;
            return 0;

        case MSG_ACTION_MARK_UNREAD:
            hdr->unread = true;
            return 0;

        case MSG_ACTION_DELETE:
            /* Guard the decrement so a double-delete can't underflow the
             * unsigned counter. */
            if (e->active)
                active_count--;
            e->active = false;
            return 0;

        case MSG_ACTION_REPLY:
            /* The UI reads hdr->sender directly to prefill the reply
             * recipient and no longer dispatches this action itself, but
             * this case stays as the documented, testable contract for
             * callers that do invoke it (see m17_sms_source_test.cpp). */
            return 0;

        case MSG_ACTION_VIEW:
            return 0;

        default:
            return -EINVAL;
    }
}

/**
 * UI-thread tick: drain struct pkt_rx_event and struct pkt_tx_done from packet_io.
 *
 * RX events carry received message text; allocate a slot and populate header.
 * TX completion events carry tag+status; find the matching in-flight entry
 * and update its status.
 *
 * @return true if any entries were added or changed.
 */
static bool sms_tick(void *ctx)
{
    (void)ctx;
    bool changed = false;

    /* Static: avoid large stack allocations on embedded targets. */
    static struct pkt_rx_event rx_evt;
    static struct pkt_tx_done tx_done;

    /* Drain RX received messages. */
    while (packet_io_dequeue_rx(&rx_evt)) {
        if (rx_evt.mode_id != (uint8_t)OPMODE_M17)
            continue;

        size_t text_len = rx_evt.body_len;
        if (text_len >= PKT_BODY_MAX_LEN)
            text_len = PKT_BODY_MAX_LEN - 1;

        SmsEntry &e = sms_store_entry(rx_evt.body, text_len, 0);
        e.hdr.direction = MSG_DIR_RX;
        e.hdr.status = MSG_STATUS_RECEIVED;
        e.hdr.unread = true;

        strncpy(e.hdr.sender, rx_evt.src, sizeof(e.hdr.sender) - 1);
        e.hdr.sender[sizeof(e.hdr.sender) - 1] = '\0';

        changed = true;

#ifdef PLATFORM_LINUX
        /* Signal reception on stderr so the loopback test script can detect
         * it with a simple grep.  Guarded: fprintf pulls in stdio and uses
         * significant stack — unsafe on embedded RTX thread (512 B stack). */
        fprintf(stderr, "SMS_RECEIVED from '%s': '%s'\n", rx_evt.src,
                rx_evt.body);
#endif
    }

    /* Drain TX completion events. */
    while (packet_io_dequeue_tx_done(&tx_done)) {
        if (tx_done.mode_id != (uint8_t)OPMODE_M17)
            continue;

        /* Find the matching in-flight TX entry by tag. */
        for (size_t i = 0; i < M17_SMS_MAX_MESSAGES; i++) {
            if (entries[i].active && entries[i].tag == tx_done.tag
                && entries[i].hdr.direction == MSG_DIR_TX
                && entries[i].hdr.status == MSG_STATUS_SENDING) {
                entries[i].hdr.status =
                    static_cast<enum message_status>(tx_done.status);
                changed = true;
                break;
            }
        }
    }

    return changed;
}

/**
 * UI-thread send: create an inbox entry and enqueue a TX request.
 */
static int sms_send(void *ctx, const char *body, size_t body_len,
                    const char *recipient)
{
    (void)ctx;

    if (body == nullptr || recipient == nullptr || recipient[0] == '\0')
        return -EINVAL;

    if (body_len > PKT_BODY_MAX_LEN - 1)
        return -EMSGSIZE;

    /* Monotonically increasing tag for TX completion matching;
     * wraps at UINT32_MAX, fall back to 1 on zero. */
    static uint32_t tag_counter = 0;
    tag_counter++;
    if (tag_counter == 0)
        tag_counter = 1;
    uint32_t tag = tag_counter;

    /* Create the outbox entry before enqueue so that a failed TX still leaves
     * a visible record in the inbox.  This is safe because packet_io uses a
     * single-slot queue: only one send can be in flight at a time, and callers
     * check -EBUSY to decide whether to retry.  Storing here may evict an old
     * entry; that is permitted because send() is one of the UI-thread
     * callbacks covered by the registry's eviction invariant (see
     * messages.h) and marks the snapshot dirty for rebuild on the next tick. */
    SmsEntry &e = sms_store_entry(body, body_len, tag);
    e.hdr.direction = MSG_DIR_TX;
    e.hdr.unread = false;

    /* Read the local callsign from UI-owned state (this runs on the UI thread
     * under state_mutex, and state.settings.callsign is what feeds
     * rtxStatus.source_address in the first place — see threads.c).  Reading it
     * through rtx_getStatus() here would be a cross-thread torn read against the
     * RTX thread's status memcpy. */
    strncpy(e.hdr.sender, state.settings.callsign, sizeof(e.hdr.sender) - 1);
    e.hdr.sender[sizeof(e.hdr.sender) - 1] = '\0';
    strncpy(e.hdr.recipient, recipient, sizeof(e.hdr.recipient) - 1);
    e.hdr.recipient[sizeof(e.hdr.recipient) - 1] = '\0';

    /* Static buffer: packet_io_enqueue_tx copies data out via memcpy into its
     * own static tx_slot before this function returns.  No pointer to &req
     * escapes, and sms_send() is never re-entered on the single-threaded UI. */
    static struct pkt_tx_request req;
    req.mode_id = (uint8_t)OPMODE_M17;
    strncpy(req.dst, recipient, sizeof(req.dst) - 1);
    req.dst[sizeof(req.dst) - 1] = '\0';

    memcpy(req.body, body, body_len);
    req.body[body_len] = '\0';
    req.body_len = body_len;
    req.tag = tag;

    if (!packet_io_enqueue_tx(&req)) {
        /* TX queue full — mark entry as failed so the user sees it in the
         * inbox and can retry later.  The RTX thread never processes this
         * tag, so no completion event will arrive to update status further. */
        e.hdr.status = MSG_STATUS_FAILED;
        return -EBUSY;
    }

    e.hdr.status = MSG_STATUS_SENDING;
    return 0;
}

static void sms_start_compose(void *ctx)
{
    (void)ctx;
    /* Nothing to initialise: compose state is owned by the UI layer. */
}

/* ===================================================================
 * Exported ops
 * =================================================================== */

const struct message_ops m17_sms_ops = {
    /* name                */ "M17 SMS",
    /* count               */ sms_count,
    /* get                 */ sms_get,
    /* supported_actions   */ sms_supported_actions,
    /* invoke_action       */ sms_invoke_action,
    /* start_compose       */ sms_start_compose,
    /* tick                */ sms_tick,
    /* send                */ sms_send,
    /* mode_id             */ (uint8_t)OPMODE_M17,
};

/* ===================================================================
 * Public API
 * =================================================================== */

void m17_sms_init(void)
{
    memset(entries, 0, sizeof(entries));
    active_count = 0;
    memset(body_pool, 0, sizeof(body_pool));
    pool_head = 0;
    memset(&tx_pkt, 0, sizeof(tx_pkt));
    memset(&rx_pkt, 0, sizeof(rx_pkt));

    tx_desc.status = PKT_STATUS_IDLE;

    rx_desc.buffer = &rx_pkt;
    rx_desc.size = sizeof(rx_pkt);
    rx_desc.status = PKT_STATUS_IDLE;
}

/**
 * Fill rtx_tx_done and post it to the TX-completion queue.  On a full queue,
 * latch rtx_tx_done_ready so m17_sms_task_rtx() retries the post next tick
 * (the caller must not clobber rtx_tx_done before then).
 *
 * @return true if the event was enqueued, false if latched for retry.
 */
static bool post_tx_done(uint32_t tag, uint8_t status)
{
    memset(&rtx_tx_done, 0, sizeof(rtx_tx_done));
    rtx_tx_done.mode_id = (uint8_t)OPMODE_M17;
    rtx_tx_done.tag = tag;
    rtx_tx_done.status = status;
    if (packet_io_enqueue_tx_done(&rtx_tx_done))
        return true;
    rtx_tx_done_ready = true; /* retry next tick */
    return false;
}

void m17_sms_task_rtx(void)
{
    /*
     * Arm the RX descriptor whenever it is idle.  Using a retry-on-IDLE loop
     * rather than a one-shot flag handles two cases:
     *   1. The very first call may race before the M17 opmode is active;
     *      rtx_addPacketRx() then resets status to PKT_STATUS_IDLE on failure,
     *      so the next tick retries automatically.
     *   2. After a received packet is processed the descriptor is reset to
     *      PKT_STATUS_IDLE so we re-arm for the next incoming packet.
     */
    if (rx_desc.status == PKT_STATUS_IDLE)
        rtx_addPacketRx(&rx_desc);

    /* rtx_req, rtx_tx_done, rtx_rx_evt, rtx_tx_done_ready, rtx_rx_evt_ready,
     * rtx_rx_text are file-scope statics;
     * see the RTX-thread storage section above.
     *
     * rtx_tx_done_ready: a FAILED or SENT completion event is filled in
     * rtx_tx_done and waiting to be posted.  We retry posting on each tick
     * until the UI thread drains the TX-done queue (packet_io_dequeue_tx_done).
     * rtx_rx_evt_ready: a RECEIVED event is filled in rtx_rx_evt and
     * waiting to be posted similarly. */

    /* Retry posting a pending TX completion event. */
    if (rtx_tx_done_ready) {
        if (packet_io_enqueue_tx_done(&rtx_tx_done)) {
            rtx_tx_done_ready = false;
            tx_inflight_tag = 0;
            /* The failed-enqueue path deliberately left tx_desc.status at
             * PKT_STATUS_DONE/ERROR so this retry could re-post the same
             * completion; now that it's been posted, re-arm the
             * descriptor or the completion check below would fire again
             * next tick and post a spurious duplicate with a stale tag. */
            tx_desc.status = PKT_STATUS_IDLE;
        }
        return;
    }

    /* Retry posting a pending RX received event. */
    if (rtx_rx_evt_ready) {
        if (packet_io_enqueue_rx(&rtx_rx_evt)) {
            rtx_rx_evt_ready = false;
            /* Same reasoning as the TX-done retry above: re-arm the RX
             * descriptor now that the event has actually been posted, or
             * the still-DONE descriptor gets re-parsed and delivered a
             * second time next tick. */
            rx_desc.status = PKT_STATUS_IDLE;
        }
        return;
    }

    /* --- TX: pick up a new request if idle --- */
    if (tx_desc.status == PKT_STATUS_IDLE) {
        if (packet_io_dequeue_tx(&rtx_req)) {
            /* Zero the m17Packet to ensure src/dst are clean. */
            memset(&tx_pkt, 0, sizeof(tx_pkt));

            /* Format the M17 application-layer SMS packet into
             * tx_pkt.payload. */
            size_t pkt_len =
                M17::sms_format_packet(rtx_req.body, rtx_req.body_len,
                                       tx_pkt.payload, M17_MAX_PACKET_DATA);
            if (pkt_len == 0) {
                /* Format error — post a FAILED completion event. */
                post_tx_done(rtx_req.tag, MSG_STATUS_FAILED);
            } else {
                tx_inflight_tag = rtx_req.tag;
                strncpy(tx_pkt.dst, rtx_req.dst, sizeof(tx_pkt.dst) - 1);

                /* Set source callsign from RTX status — matches voice TX. */
                const rtxStatus_t *st = rtx_getStatus();
                if (st->source_address[0] != '\0') {
                    strncpy(tx_pkt.src, st->source_address,
                            sizeof(tx_pkt.src) - 1);
                }

                tx_desc.buffer = &tx_pkt;
                tx_desc.size = pkt_len + 20; /* payload + src[10] + dst[10] */
                if (rtx_addPacketTx(&tx_desc) != 0) {
                    /* Submission rejected (e.g. the operating mode was
                     * switched away from M17 between the UI thread's
                     * enqueue and this dequeue) — rtx_addPacketTx() already
                     * reset tx_desc.status to PKT_STATUS_IDLE on failure,
                     * so the DONE/ERROR completion check below will never
                     * see this request. Post a FAILED completion now
                     * instead of leaving the inbox entry stuck at
                     * MSG_STATUS_SENDING forever. */
                    post_tx_done(rtx_req.tag, MSG_STATUS_FAILED);
                    tx_inflight_tag = 0;
                }
            }
        }
    }

    /* --- TX completion check --- */
    if (tx_desc.status == PKT_STATUS_DONE
        || tx_desc.status == PKT_STATUS_ERROR) {
        uint8_t final_status = (tx_desc.status == PKT_STATUS_DONE) ?
                                   (uint8_t)MSG_STATUS_SENT :
                                   (uint8_t)MSG_STATUS_FAILED;

        /* On a full queue, post_tx_done() latches rtx_tx_done_ready so the
         * post retries next tick; leave tx_desc.status as-is until then. */
        if (post_tx_done(tx_inflight_tag, final_status)) {
            tx_desc.status = PKT_STATUS_IDLE;
            tx_inflight_tag = 0;
            memset(&tx_pkt, 0, sizeof(tx_pkt)); /* clear for next TX */
        }
    }

    /* --- RX completion check --- */
    if (rx_desc.status == PKT_STATUS_ERROR) {
        /* Deframer error (bad CRC, sequence, overflow); discard and re-arm. */
        rx_desc.status = PKT_STATUS_IDLE;
        return;
    }
    if (rx_desc.status != PKT_STATUS_DONE)
        return;

    /* Parse the received application-layer payload.  rtx_rx_text is a
     * file-scope static to avoid an 822-byte stack allocation on the
     * embedded RTX thread. */
    size_t pkt_len = (rx_desc.res > 0) ? (size_t)rx_desc.res : 0;

    bool ok =
        M17::sms_parse_packet(&((struct m17Packet *)rx_desc.buffer)->payload[0],
                              pkt_len, rtx_rx_text, sizeof(rtx_rx_text));

    if (ok) {
        size_t text_len = strlen(rtx_rx_text);

        memset(&rtx_rx_evt, 0, sizeof(rtx_rx_evt));
        rtx_rx_evt.mode_id = (uint8_t)OPMODE_M17;

        size_t copy_len = (text_len < PKT_BODY_MAX_LEN) ?
                              text_len :
                              (PKT_BODY_MAX_LEN - 1);
        memcpy(rtx_rx_evt.body, rtx_rx_text, copy_len);
        rtx_rx_evt.body[copy_len] = '\0';
        rtx_rx_evt.body_len = copy_len;

        /* Source callsign is embedded in the packet buffer by OpMode_M17. */
        const struct m17Packet *pkt = (const struct m17Packet *)rx_desc.buffer;
        strncpy(rtx_rx_evt.src, pkt->src, sizeof(rtx_rx_evt.src) - 1);
        rtx_rx_evt.src[sizeof(rtx_rx_evt.src) - 1] = '\0';

        if (!packet_io_enqueue_rx(&rtx_rx_evt)) {
            /* Queue full; retry next tick.  rx_desc will not be re-armed
             * until the event is successfully delivered. */
            rtx_rx_evt_ready = true;
            return;
        }
    }

    /* Re-arm for the next incoming packet. */
    rx_desc.status = PKT_STATUS_IDLE;
}
