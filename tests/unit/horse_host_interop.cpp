/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Identity and peer blobs from horse_provision.py must load in the C
 * keystore and peer table. A wrong passphrase must be rejected.
 * Passphrase is passed via HORSE_PASSPHRASE, never argv.
 */

#include "protocols/horse/horse_keystore.h"
#include "protocols/horse/horse_peers.h"
#include "protocols/horse/horse_crypto.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <string>

#ifndef HORSE_PROVISION_PY
#error HORSE_PROVISION_PY is required
#endif

static int parse_hex(const char *hex, uint8_t *out, size_t n)
{
    if (strlen(hex) != n * 2)
        return -1;
    for (size_t i = 0; i < n; i++) {
        unsigned v = 0;
        if (sscanf(hex + 2 * i, "%2x", &v) != 1)
            return -1;
        out[i] = static_cast<uint8_t>(v);
    }
    return 0;
}

static int run_cmd(const std::string &cmd, std::string &out)
{
    FILE *fp = popen(cmd.c_str(), "r");
    if (fp == nullptr)
        return -1;
    char buf[512];
    out.clear();
    while (fgets(buf, sizeof buf, fp) != nullptr)
        out += buf;
    int st = pclose(fp);
    return st;
}

static int json_get_hex(const std::string &js, const char *key, char *hex64)
{
    std::string needle = std::string("\"") + key + "\": \"";
    std::string::size_type pos = js.find(needle);
    if (pos == std::string::npos) {
        needle = std::string("\"") + key + "\":\"";
        pos = js.find(needle);
        if (pos == std::string::npos)
            return -1;
    }
    pos += needle.size();
    if (pos + 64 > js.size())
        return -1;
    memcpy(hex64, js.c_str() + pos, 64);
    hex64[64] = 0;
    return 0;
}

int main()
{
#ifndef HAVE_LIBSODIUM
    std::printf("horse_host_interop: skipped (no libsodium)\n");
    return 77;
#else
    char dir[] = "/tmp/horse_interopXXXXXX";
    if (mkdtemp(dir) == nullptr)
        return -1;
    std::string xdg = std::string(dir);
    setenv("XDG_STATE_HOME", xdg.c_str(), 1);
    const char *pass = "host-interop-pass";
    setenv("HORSE_PASSPHRASE", pass, 1);
    std::string openrtx = xdg + "/OpenRTX";
    mkdir(openrtx.c_str(), 0700);

    std::string py = HORSE_PROVISION_PY;
    std::string cmd = std::string("python3 \"") + py
                    + "\" --json generate interop";
    std::string gen_out;
    if (run_cmd(cmd, gen_out) != 0) {
        std::printf("horse_host_interop: generate failed\n%s\n",
                    gen_out.c_str());
        return -1;
    }
    if (gen_out.find(pass) != std::string::npos) {
        std::printf("horse_host_interop: passphrase leaked in output\n");
        return -1;
    }

    char ed_hex[65] = { 0 };
    char x_hex[65] = { 0 };
    if (json_get_hex(gen_out, "ed25519_pk", ed_hex) != 0
        || json_get_hex(gen_out, "x25519_pk", x_hex) != 0) {
        std::printf("horse_host_interop: generate JSON missing keys\n%s\n",
                    gen_out.c_str());
        return -1;
    }

    std::string src = openrtx + "/horse_identity_interop.bin";
    std::string dst = openrtx + "/horse_identity.bin";
    cmd = "cp \"" + src + "\" \"" + dst + "\"";
    std::string unused;
    if (run_cmd(cmd, unused) != 0) {
        std::printf("horse_host_interop: copy identity failed\n");
        return -1;
    }

    cmd = std::string("python3 \"") + py
        + "\" --json export-peer 3 --address 010203040506 --x25519 " + x_hex
        + " --ed25519 " + ed_hex;
    if (run_cmd(cmd, unused) != 0) {
        std::printf("horse_host_interop: export-peer failed\n%s\n",
                    unused.c_str());
        return -1;
    }

    horse_keystore_init();
    if (horse_keystore_unlock("wrong-pass", 10)) {
        std::printf("horse_host_interop: wrong passphrase accepted\n");
        return -1;
    }
    if (!horse_keystore_unlock(pass, strlen(pass))) {
        std::printf("horse_host_interop: correct passphrase rejected\n");
        return -1;
    }

    horse_identity_keys_t id;
    memset(&id, 0, sizeof id);
    uint8_t ed_pk[32], x_pk[32];
    if (parse_hex(ed_hex, ed_pk, 32) != 0 || parse_hex(x_hex, x_pk, 32) != 0)
        return -1;
    if (!horse_keystore_copy_identity(&id)
        || memcmp(id.ed25519_pk, ed_pk, 32) != 0
        || memcmp(id.x25519_pk, x_pk, 32) != 0) {
        std::printf("horse_host_interop: C identity mismatch\n");
        return -1;
    }

    uint8_t fp[32];
    if (!horse_keystore_fingerprint(fp)) {
        std::printf("horse_host_interop: fingerprint failed\n");
        return -1;
    }
    char fp_hex[65] = { 0 };
    if (json_get_hex(gen_out, "fingerprint", fp_hex) != 0) {
        std::printf("horse_host_interop: generate JSON missing fingerprint\n");
        return -1;
    }
    uint8_t fp_py[32];
    if (parse_hex(fp_hex, fp_py, 32) != 0 || memcmp(fp, fp_py, 32) != 0) {
        std::printf("horse_host_interop: fingerprint mismatch C vs Python\n");
        return -1;
    }

    horse_peer_t peer;
    memset(&peer, 0, sizeof peer);
    uint8_t addr[6] = { 1, 2, 3, 4, 5, 6 };
    if (!horse_peer_read(3, &peer) || memcmp(peer.address, addr, 6) != 0
        || memcmp(peer.ed25519_pk, ed_pk, 32) != 0
        || memcmp(peer.x25519_pk, x_pk, 32) != 0) {
        std::printf("horse_host_interop: C peer table mismatch\n");
        return -1;
    }

    horse_crypto_memzero(&id, sizeof id);
    horse_keystore_lock();
    unsetenv("HORSE_PASSPHRASE");
    std::printf(
        "horse_host_interop: python blobs read by C, wrong pass rejected\n");
    return 0;
#endif
}
