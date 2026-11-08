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
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>
#elif defined(PLATFORM_MD3x0) || defined(PLATFORM_MDUV3x0) \
    || defined(PLATFORM_MD9600)
#include "usb_vcom.h"
#endif

#define HORSE_PROV_VERSION 1U
#define HORSE_MSG_HELLO 0x01U
#define HORSE_MSG_HELLO_ACK 0x02U
#define HORSE_MSG_SEND_IDENTITY 0x03U
#define HORSE_MSG_CONFIRM 0x04U
#define HORSE_MSG_ERROR 0xFFU

typedef struct {
    uint8_t version;
    uint8_t type;
    uint16_t len;
} horse_prov_hdr_t;

static uint8_t rx_buf[320];
static size_t rx_len;

#ifdef PLATFORM_LINUX
static int prov_listen_fd = -1;
static int prov_fd = -1;
static char prov_sock_path[sizeof(((struct sockaddr_un *)0)->sun_path)];

#define PROV_WRITE_TIMEOUT_MS 500

static void prov_drop_client(void)
{
    if (prov_fd >= 0) {
        close(prov_fd);
        prov_fd = -1;
    }
    rx_len = 0;
}

static void prov_accept_client(void)
{
    int cfd;

    if (prov_listen_fd < 0 || prov_fd >= 0)
        return;
    cfd = accept(prov_listen_fd, NULL, NULL);
    if (cfd < 0)
        return;
    fcntl(cfd, F_SETFL, O_NONBLOCK);
    prov_fd = cfd;
    rx_len = 0;
}

/**
 * Resolve the Unix socket path into out (NUL-terminated).
 * Order: OPENRTX_HORSE_PROV_SOCK, else $XDG_RUNTIME_DIR/..., else
 * $XDG_STATE_HOME/OpenRTX/... or ~/.local/state/OpenRTX/...
 */
int horse_provision_default_sock_path(char *out, size_t out_len)
{
    const char *env;
    const char *home;
    int n;

    if (out == NULL || out_len < 16)
        return -1;

    env = getenv("OPENRTX_HORSE_PROV_SOCK");
    if (env != NULL && env[0] != '\0') {
        if (strlen(env) >= out_len)
            return -1;
        memcpy(out, env, strlen(env) + 1);
        return 0;
    }

    env = getenv("XDG_RUNTIME_DIR");
    if (env != NULL && env[0] != '\0') {
        n = snprintf(out, out_len, "%s/openrtx_horse_prov.sock", env);
        if (n < 0 || (size_t)n >= out_len)
            return -1;
        return 0;
    }

    env = getenv("XDG_STATE_HOME");
    if (env != NULL && env[0] != '\0') {
        n = snprintf(out, out_len, "%s/OpenRTX/openrtx_horse_prov.sock", env);
        if (n < 0 || (size_t)n >= out_len)
            return -1;
        return 0;
    }

    home = getenv("HOME");
    if (home == NULL || home[0] == '\0')
        return -1;
    n = snprintf(out, out_len,
                 "%s/.local/state/OpenRTX/openrtx_horse_prov.sock", home);
    if (n < 0 || (size_t)n >= out_len)
        return -1;
    return 0;
}
#endif

static ssize_t prov_read(void *buf, size_t len)
{
#ifdef PLATFORM_LINUX
    ssize_t n;

    prov_accept_client();
    if (prov_fd < 0)
        return -1;
    n = read(prov_fd, buf, len);
    if (n == 0) {
        /* Peer closed. */
        prov_drop_client();
        return -1;
    }
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return -1;
        if (errno == EPIPE || errno == ECONNRESET) {
            prov_drop_client();
            return -1;
        }
        return -1;
    }
    return n;
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
    const uint8_t *p = (const uint8_t *)buf;
    size_t sent = 0;
    struct timespec t0, now;
    long elapsed_ms;

    if (prov_fd < 0 || buf == NULL)
        return -1;
    if (len == 0)
        return 0;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    while (sent < len) {
        ssize_t n = send(prov_fd, p + sent, len - sent, MSG_NOSIGNAL);

        if (n > 0) {
            sent += (size_t)n;
            continue;
        }
        if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
            struct pollfd pfd = { .fd = prov_fd, .events = POLLOUT };

            clock_gettime(CLOCK_MONOTONIC, &now);
            elapsed_ms = (now.tv_sec - t0.tv_sec) * 1000L
                       + (now.tv_nsec - t0.tv_nsec) / 1000000L;
            if (elapsed_ms >= PROV_WRITE_TIMEOUT_MS) {
                prov_drop_client();
                return -1;
            }
            poll(&pfd, 1, (int)(PROV_WRITE_TIMEOUT_MS - elapsed_ms));
            continue;
        }
        if (n < 0 && (errno == EPIPE || errno == ECONNRESET)) {
            prov_drop_client();
            return -1;
        }
        prov_drop_client();
        return -1;
    }
    return (ssize_t)sent;
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

    if (prov_write(&hdr, sizeof hdr) < 0)
        return;
    if (payload != NULL && len > 0)
        (void)prov_write(payload, len);
}

static void prov_handle_identity(const uint8_t *payload, uint16_t len)
{
    if (len < sizeof(horse_identity_keys_t)) {
        prov_send(HORSE_MSG_ERROR, "bad identity size", 17);
        return;
    }

    const horse_identity_keys_t *identity =
        (const horse_identity_keys_t *)payload;

    if (identity->version != 1) {
        prov_send(HORSE_MSG_ERROR, "bad identity version", 20);
        return;
    }
    if (memcmp(identity->ed25519_sk + HORSE_ED25519_PUBLICKEY_BYTES,
               identity->ed25519_pk, HORSE_ED25519_PUBLICKEY_BYTES)
        != 0) {
        prov_send(HORSE_MSG_ERROR, "bad ed25519 sk", 14);
        return;
    }
    if (!horse_keystore_has_passphrase()) {
        prov_send(HORSE_MSG_ERROR, "no passphrase", 13);
        return;
    }

    if (!horse_keystore_store_with_held(identity)) {
        prov_send(HORSE_MSG_ERROR, "store failed", 12);
        return;
    }

    uint8_t fp[32];
    if (!horse_keystore_fingerprint(fp)) {
        prov_send(HORSE_MSG_ERROR, "fingerprint failed", 18);
        return;
    }

    prov_send(HORSE_MSG_CONFIRM, fp, sizeof fp);
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

    /* C15: reject oversized length, reset parser. */
    if (hdr->len > sizeof(rx_buf) - sizeof(horse_prov_hdr_t)) {
        rx_len = 0;
        return;
    }

    size_t need = sizeof(horse_prov_hdr_t) + hdr->len;
    if (rx_len < need)
        return;

    const uint8_t *payload = rx_buf + sizeof(horse_prov_hdr_t);

    switch (hdr->type) {
        case HORSE_MSG_HELLO:
            prov_send(HORSE_MSG_HELLO_ACK, NULL, 0);
            break;
        case HORSE_MSG_SEND_IDENTITY:
            prov_handle_identity(payload, hdr->len);
            break;
        default:
            prov_send(HORSE_MSG_ERROR, "unknown message", 15);
            break;
    }

        /* Client may have been dropped mid-reply. */
#ifdef PLATFORM_LINUX
    if (prov_fd < 0)
        return;
#endif

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
    /*
     * Unix stream socket (not a FIFO). Default under XDG_RUNTIME_DIR
     * (or a per-user state dir). Override with OPENRTX_HORSE_PROV_SOCK.
     * Untested on hardware radios (USB VCOM path unchanged).
     */
    struct sockaddr_un addr;
    struct stat st;
    mode_t old_umask;
    char dirbuf[sizeof addr.sun_path];
    char *slash;

    if (horse_provision_default_sock_path(prov_sock_path, sizeof prov_sock_path)
        != 0) {
        fprintf(stderr, "horse_provision: cannot resolve socket path "
                        "(set OPENRTX_HORSE_PROV_SOCK or XDG_RUNTIME_DIR)\n");
        return;
    }
    if (strlen(prov_sock_path) >= sizeof addr.sun_path) {
        fprintf(stderr, "horse_provision: socket path too long\n");
        return;
    }

    /* Ensure parent directory exists (mode 0700 for fallback state dir). */
    memcpy(dirbuf, prov_sock_path, sizeof dirbuf);
    slash = strrchr(dirbuf, '/');
    if (slash != NULL && slash != dirbuf) {
        *slash = '\0';
        if (mkdir(dirbuf, 0700) < 0 && errno != EEXIST) {
            fprintf(stderr, "horse_provision: mkdir %s: %s\n", dirbuf,
                    strerror(errno));
            return;
        }
        if (chmod(dirbuf, 0700) < 0 && errno != EPERM) {
            /* Best-effort; continue. */
        }
    }

    if (lstat(prov_sock_path, &st) == 0) {
        if (!S_ISSOCK(st.st_mode)) {
            fprintf(stderr,
                    "horse_provision: %s exists and is not a socket; "
                    "refusing to unlink\n",
                    prov_sock_path);
            return;
        }
        unlink(prov_sock_path);
    } else if (errno != ENOENT) {
        fprintf(stderr, "horse_provision: lstat %s: %s\n", prov_sock_path,
                strerror(errno));
        return;
    }

    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, prov_sock_path, sizeof addr.sun_path - 1);

    prov_listen_fd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (prov_listen_fd < 0)
        return;
    fcntl(prov_listen_fd, F_SETFL, O_NONBLOCK);

    old_umask = umask(0077);
    if (bind(prov_listen_fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        umask(old_umask);
        fprintf(stderr, "horse_provision: bind %s: %s\n", prov_sock_path,
                strerror(errno));
        close(prov_listen_fd);
        prov_listen_fd = -1;
        return;
    }
    umask(old_umask);

    /* fchmod is not reliable on all AF_UNIX implementations; chmod path. */
    if (chmod(prov_sock_path, 0600) < 0) {
        fprintf(stderr, "horse_provision: chmod %s: %s\n", prov_sock_path,
                strerror(errno));
    }

    if (listen(prov_listen_fd, 1) < 0) {
        close(prov_listen_fd);
        prov_listen_fd = -1;
        unlink(prov_sock_path);
        return;
    }
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
