/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Host test for Linux Horse provisioning Unix socket: happy path,
 * SIGPIPE-safe disconnect, oversized length, partial header, path/mode.
 */

#include "protocols/horse/horse_provision.h"
#include "protocols/horse/horse_keystore.h"
#include "protocols/horse/horse_crypto.h"
#include <cerrno>
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>

#ifdef HAVE_LIBSODIUM
#include <sodium.h>
#endif

#define HORSE_MSG_HELLO 0x01
#define HORSE_MSG_HELLO_ACK 0x02
#define HORSE_MSG_SEND_IDENTITY 0x03
#define HORSE_MSG_CONFIRM 0x04

static int connect_sock(const char *path)
{
    int fd = socket(AF_UNIX, SOCK_STREAM, 0);
    struct sockaddr_un addr;

    if (fd < 0)
        return -1;
    memset(&addr, 0, sizeof addr);
    addr.sun_family = AF_UNIX;
    strncpy(addr.sun_path, path, sizeof addr.sun_path - 1);
    if (connect(fd, (struct sockaddr *)&addr, sizeof addr) < 0) {
        close(fd);
        return -1;
    }
    return fd;
}

static int send_all(int fd, const void *buf, size_t n)
{
    const uint8_t *p = (const uint8_t *)buf;
    size_t sent = 0;

    while (sent < n) {
        ssize_t w = send(fd, p + sent, n - sent, MSG_NOSIGNAL);
        if (w <= 0)
            return -1;
        sent += (size_t)w;
    }
    return 0;
}

static int recv_exact(int fd, void *buf, size_t n, int timeout_ms)
{
    uint8_t *p = (uint8_t *)buf;
    size_t got = 0;

    while (got < n) {
        struct pollfd pfd;
        pfd.fd = fd;
        pfd.events = POLLIN;
        pfd.revents = 0;
        int pr = poll(&pfd, 1, timeout_ms);
        if (pr <= 0)
            return -1;
        ssize_t r = recv(fd, p + got, n - got, 0);
        if (r <= 0)
            return -1;
        got += (size_t)r;
    }
    return 0;
}

static void pump(int rounds)
{
    for (int i = 0; i < rounds; i++)
        horse_provision_poll();
}

static int test_happy(const char *sock, const horse_identity_keys_t *id,
                      const uint8_t expect_fp[32])
{
    uint8_t hdr[4];
    uint8_t conf_hdr[4];
    uint8_t fp[32];
    int fd = connect_sock(sock);
    if (fd < 0) {
        std::printf("horse_provision_sock: connect failed\n");
        return -1;
    }
    pump(8);
    hdr[0] = 1;
    hdr[1] = HORSE_MSG_HELLO;
    hdr[2] = 0;
    hdr[3] = 0;
    if (send_all(fd, hdr, 4) != 0) {
        close(fd);
        return -1;
    }
    pump(32);
    if (recv_exact(fd, conf_hdr, 4, 2000) != 0
        || conf_hdr[1] != HORSE_MSG_HELLO_ACK) {
        std::printf("horse_provision_sock: no HELLO_ACK\n");
        close(fd);
        return -1;
    }
    hdr[1] = HORSE_MSG_SEND_IDENTITY;
    hdr[2] = (uint8_t)(sizeof(*id) & 0xff);
    hdr[3] = (uint8_t)((sizeof(*id) >> 8) & 0xff);
    if (send_all(fd, hdr, 4) != 0 || send_all(fd, id, sizeof(*id)) != 0) {
        close(fd);
        return -1;
    }
    pump(64);
    if (recv_exact(fd, conf_hdr, 4, 2000) != 0
        || conf_hdr[1] != HORSE_MSG_CONFIRM) {
        std::printf("horse_provision_sock: no CONFIRM\n");
        close(fd);
        return -1;
    }
    if (recv_exact(fd, fp, 32, 2000) != 0 || memcmp(fp, expect_fp, 32) != 0) {
        std::printf("horse_provision_sock: fingerprint mismatch\n");
        close(fd);
        return -1;
    }
    close(fd);
    pump(8);
    return 0;
}

static int test_disconnect_before_reply(const char *sock)
{
    uint8_t hdr[4] = { 1, HORSE_MSG_HELLO, 0, 0 };
    int fd = connect_sock(sock);
    if (fd < 0)
        return -1;
    pump(8);
    if (send_all(fd, hdr, 4) != 0) {
        close(fd);
        return -1;
    }
    /* Close without reading HELLO_ACK — must not SIGPIPE the process. */
    close(fd);
    pump(64);
    /* Accept a new client afterwards. */
    fd = connect_sock(sock);
    if (fd < 0) {
        std::printf("horse_provision_sock: re-accept failed after drop\n");
        return -1;
    }
    if (send_all(fd, hdr, 4) != 0) {
        close(fd);
        return -1;
    }
    pump(32);
    uint8_t ack[4];
    if (recv_exact(fd, ack, 4, 2000) != 0 || ack[1] != HORSE_MSG_HELLO_ACK) {
        std::printf("horse_provision_sock: second HELLO_ACK failed\n");
        close(fd);
        return -1;
    }
    close(fd);
    pump(8);
    return 0;
}

static int test_oversized_len(const char *sock)
{
    /* len larger than buffer remainder: parser must reset, stay alive. */
    uint8_t hdr[4] = { 1, HORSE_MSG_HELLO, 0xff, 0xff };
    int fd = connect_sock(sock);
    if (fd < 0)
        return -1;
    pump(8);
    if (send_all(fd, hdr, 4) != 0) {
        close(fd);
        return -1;
    }
    pump(16);
    close(fd);
    pump(8);
    fd = connect_sock(sock);
    if (fd < 0)
        return -1;
    hdr[1] = HORSE_MSG_HELLO;
    hdr[2] = 0;
    hdr[3] = 0;
    if (send_all(fd, hdr, 4) != 0) {
        close(fd);
        return -1;
    }
    pump(32);
    uint8_t ack[4];
    if (recv_exact(fd, ack, 4, 2000) != 0 || ack[1] != HORSE_MSG_HELLO_ACK) {
        std::printf("horse_provision_sock: recover after oversized failed\n");
        close(fd);
        return -1;
    }
    close(fd);
    pump(8);
    return 0;
}

static int test_partial_header(const char *sock)
{
    uint8_t partial[2] = { 1, HORSE_MSG_HELLO };
    int fd = connect_sock(sock);
    if (fd < 0)
        return -1;
    pump(8);
    if (send_all(fd, partial, 2) != 0) {
        close(fd);
        return -1;
    }
    pump(8);
    close(fd);
    pump(16);
    fd = connect_sock(sock);
    if (fd < 0) {
        std::printf("horse_provision_sock: reconnect after partial failed\n");
        return -1;
    }
    uint8_t hdr[4] = { 1, HORSE_MSG_HELLO, 0, 0 };
    if (send_all(fd, hdr, 4) != 0) {
        close(fd);
        return -1;
    }
    pump(32);
    uint8_t ack[4];
    if (recv_exact(fd, ack, 4, 2000) != 0 || ack[1] != HORSE_MSG_HELLO_ACK) {
        std::printf("horse_provision_sock: after partial header failed\n");
        close(fd);
        return -1;
    }
    close(fd);
    pump(8);
    return 0;
}

static int test_path_and_mode(const char *runtime_dir)
{
    char resolved[108];
    char expect[160];
    struct stat st;

    setenv("XDG_RUNTIME_DIR", runtime_dir, 1);
    unsetenv("OPENRTX_HORSE_PROV_SOCK");
    if (horse_provision_default_sock_path(resolved, sizeof resolved) != 0)
        return -1;
    snprintf(expect, sizeof expect, "%s/openrtx_horse_prov.sock", runtime_dir);
    if (strcmp(resolved, expect) != 0) {
        std::printf("horse_provision_sock: path %s != %s\n", resolved, expect);
        return -1;
    }
    if (lstat(expect, &st) != 0 || !S_ISSOCK(st.st_mode)) {
        std::printf("horse_provision_sock: socket missing at %s\n", expect);
        return -1;
    }
    if ((st.st_mode & 0777) != 0600) {
        std::printf("horse_provision_sock: mode %o != 0600\n",
                    st.st_mode & 0777);
        return -1;
    }
    return 0;
}

int main()
{
#ifndef HAVE_LIBSODIUM
    std::printf("horse_provision_sock: skipped (no libsodium)\n");
    return 77;
#else
    char tmpl[] = "/tmp/horse_prov_rtXXXXXX";
    char *runtime = mkdtemp(tmpl);
    char sock[160];
    horse_identity_keys_t id;
    uint8_t fp[32];
    const char *pass = "sock-test-pass";

    if (runtime == NULL)
        return -1;
    if (sodium_init() < 0)
        return -1;

    snprintf(sock, sizeof sock, "%s/openrtx_horse_prov.sock", runtime);
    setenv("XDG_RUNTIME_DIR", runtime, 1);
    setenv("OPENRTX_HORSE_PROV_SOCK", sock, 1);
    setenv("XDG_STATE_HOME", runtime, 1);

    memset(&id, 0, sizeof id);
    id.version = 1;
    crypto_sign_ed25519_keypair(id.ed25519_pk, id.ed25519_sk);
    crypto_kx_keypair(id.x25519_pk, id.x25519_sk);

    horse_keystore_init();
    if (!horse_keystore_hold_passphrase(pass, strlen(pass))) {
        std::printf("horse_provision_sock: hold passphrase failed\n");
        return -1;
    }
    if (!horse_crypto_identity_fingerprint(&id, fp))
        return -1;

    horse_provision_init();

    if (test_path_and_mode(runtime) != 0)
        return -1;
    if (test_happy(sock, &id, fp) != 0)
        return -1;
    if (test_disconnect_before_reply(sock) != 0)
        return -1;
    if (test_oversized_len(sock) != 0)
        return -1;
    if (test_partial_header(sock) != 0)
        return -1;

    /* Happy path again after abuse. */
    if (test_happy(sock, &id, fp) != 0)
        return -1;

    horse_crypto_memzero(&id, sizeof id);
    horse_keystore_lock();
    std::printf("horse_provision_sock: all tests passed\n");
    return 0;
#endif
}
