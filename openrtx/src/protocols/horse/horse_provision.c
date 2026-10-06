/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "protocols/horse/horse_provision.h"
#include "protocols/horse/horse_keystore.h"
#include "protocols/horse/horse_crypto.h"
#include <string.h>

#ifdef PLATFORM_LINUX
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/stat.h>
#elif defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0) \
    || defined(PLATFORM_MD9600)
#include "usb_vcom.h"
#endif

#define HORSE_PROV_VERSION 1U
#define MSG_HELLO 0x01U
#define MSG_HELLO_ACK 0x02U
#define MSG_SEND_IDENTITY 0x03U
#define MSG_CONFIRM 0x04U
#define MSG_ERROR 0xFFU

typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t len;
} horse_prov_hdr_t;

static uint8_t rx_buf[320];
static size_t rx_len;

#ifdef PLATFORM_LINUX
static int prov_fd = -1;
#endif

static ssize_t prov_read(void *buf, size_t len)
{
#ifdef PLATFORM_LINUX
    if (prov_fd < 0)
        return -1;
    return read(prov_fd, buf, len);
#elif defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0) \
    || defined(PLATFORM_MD9600)
    return vcom_readBlock(buf, len);
#else
    (void)buf;
    (void)len;
    return -1;
#endif
}

static ssize_t prov_write(const void *buf, size_t len)
{
#ifdef PLATFORM_LINUX
    if (prov_fd < 0)
        return -1;
    return write(prov_fd, buf, len);
#elif defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0) \
    || defined(PLATFORM_MD9600)
    return vcom_writeBlock(buf, len);
#else
    (void)buf;
    (void)len;
    return -1;
#endif
}

static void prov_send(uint8_t type, const void *payload, uint16_t len)
{
    horse_prov_hdr_t hdr = {
        .version = HORSE_PROV_VERSION,
        .type = type,
        .len = len,
    };

    prov_write(&hdr, sizeof hdr);
    if (payload != NULL && len > 0)
        prov_write(payload, len);
}

static void prov_handle_identity(const uint8_t *payload, uint16_t len)
{
    if (len < sizeof(horse_identity_keys_t)) {
        prov_send(MSG_ERROR, "bad identity size", 17);
        return;
    }

    const horse_identity_keys_t *identity =
        (const horse_identity_keys_t *)payload;

    if (identity->version != 1) {
        prov_send(MSG_ERROR, "bad identity version", 20);
        return;
    }
    if (memcmp(identity->ed25519_sk + HORSE_ED25519_PUBLICKEY_BYTES,
               identity->ed25519_pk, HORSE_ED25519_PUBLICKEY_BYTES)
        != 0) {
        prov_send(MSG_ERROR, "bad ed25519 sk", 14);
        return;
    }
    if (!horse_keystore_has_passphrase()) {
        prov_send(MSG_ERROR, "no passphrase", 13);
        return;
    }

    if (!horse_keystore_store_with_held(identity)) {
        prov_send(MSG_ERROR, "store failed", 12);
        return;
    }

    uint8_t fp[32];
    if (!horse_keystore_fingerprint(fp)) {
        prov_send(MSG_ERROR, "fingerprint failed", 18);
        return;
    }

    prov_send(MSG_CONFIRM, fp, sizeof fp);
}

static void prov_feed_byte(uint8_t b)
{
    if (rx_len >= sizeof rx_buf)
        rx_len = 0;

    rx_buf[rx_len++] = b;

    if (rx_len < sizeof(horse_prov_hdr_t))
        return;

    horse_prov_hdr_t *hdr = (horse_prov_hdr_t *)rx_buf;
    if (hdr->version != HORSE_PROV_VERSION) {
        memmove(rx_buf, rx_buf + 1, rx_len - 1);
        rx_len--;
        return;
    }

    if (hdr->len > sizeof(rx_buf) - sizeof(horse_prov_hdr_t)) {
        rx_len = 0;
        return;
    }

    size_t need = sizeof(horse_prov_hdr_t) + hdr->len;
    if (rx_len < need)
        return;

    const uint8_t *payload = rx_buf + sizeof(horse_prov_hdr_t);

    switch (hdr->type) {
        case MSG_HELLO:
            prov_send(MSG_HELLO_ACK, NULL, 0);
            break;
        case MSG_SEND_IDENTITY:
            prov_handle_identity(payload, hdr->len);
            break;
        default:
            prov_send(MSG_ERROR, "unknown message", 15);
            break;
    }

    size_t extra = rx_len - need;
    if (extra > 0)
        memmove(rx_buf, rx_buf + need, extra);
    rx_len = extra;
}

static void prov_consume_bytes(const uint8_t *data, size_t n)
{
    for (size_t i = 0; i < n; i++)
        prov_feed_byte(data[i]);
}

void horse_provision_init(void)
{
    rx_len = 0;
#ifdef PLATFORM_LINUX
    const char *path = getenv("OPENRTX_HORSE_PROV_FIFO");
    if (path == NULL)
        path = "/tmp/openrtx_horse_prov.fifo";

    mkfifo(path, 0600);
    prov_fd = open(path, O_RDWR | O_NONBLOCK);
#endif
}

void horse_provision_poll(void)
{
    uint8_t chunk[64];
    ssize_t n = prov_read(chunk, sizeof chunk);

    if (n <= 0)
        return;

    prov_consume_bytes(chunk, (size_t)n);
}
