/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 *
 * Identity and peer blobs from horse_provision.py must load in the C
 * keystore and peer table. A wrong passphrase must be rejected.
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

int main()
{
#ifndef HAVE_LIBSODIUM
    std::printf("horse_host_interop: skipped (no libsodium)\n");
    return 0;
#else
    char dir[] = "/tmp/horse_interopXXXXXX";
    if (mkdtemp(dir) == nullptr)
        return -1;
    std::string xdg = std::string(dir);
    setenv("XDG_STATE_HOME", xdg.c_str(), 1);
    std::string openrtx = xdg + "/OpenRTX";
    mkdir(openrtx.c_str(), 0700);

    const char *pass = "host-interop-pass";
    std::string py = HORSE_PROVISION_PY;
    std::string cmd = std::string("python3 \"") + py + "\" --passphrase \""
                    + pass + "\" generate interop";
    std::string gen_out;
    if (run_cmd(cmd, gen_out) != 0) {
        std::printf("horse_host_interop: generate failed\n%s\n",
                    gen_out.c_str());
        return -1;
    }

    const char *ed_tok = "Ed25519 public key: ";
    const char *x_tok = "X25519 public key:  ";
    std::string::size_type epos = gen_out.find(ed_tok);
    std::string::size_type xpos = gen_out.find(x_tok);
    if (epos == std::string::npos || xpos == std::string::npos) {
        std::printf("horse_host_interop: generate output missing keys\n");
        return -1;
    }
    char ed_hex[65] = { 0 };
    char x_hex[65] = { 0 };
    if (sscanf(gen_out.c_str() + epos + strlen(ed_tok), "%64s", ed_hex) != 1)
        return -1;
    if (sscanf(gen_out.c_str() + xpos + strlen(x_tok), "%64s", x_hex) != 1)
        return -1;

    std::string src = openrtx + "/horse_identity_interop.bin";
    std::string dst = openrtx + "/horse_identity.bin";
    cmd = "cp \"" + src + "\" \"" + dst + "\"";
    std::string unused;
    if (run_cmd(cmd, unused) != 0) {
        std::printf("horse_host_interop: copy identity failed\n");
        return -1;
    }

    cmd = std::string("python3 \"") + py
        + "\" export-peer 3 --address 010203040506 --x25519 " + x_hex
        + " --ed25519 " + ed_hex;
    if (run_cmd(cmd, unused) != 0) {
        std::printf("horse_host_interop: export-peer failed\n");
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
    std::printf(
        "horse_host_interop: python blobs read by C, wrong pass rejected\n");
    return 0;
#endif
}
