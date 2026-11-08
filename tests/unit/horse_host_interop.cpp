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

    /* Optional: identity extracted from a throwaway OpenPGP key must unlock
     * in the C keystore the same way as generate. Requires gpg + helper. */
    const char *helper = getenv("HORSE_OPENPGP_HELPER");
    if (helper != nullptr && helper[0] != '\0' && access(helper, X_OK) == 0) {
        std::string gpghome = xdg + "/gnupg";
        mkdir(gpghome.c_str(), 0700);
        setenv("GNUPGHOME", gpghome.c_str(), 1);
        setenv("HORSE_OPENPGP_PASSPHRASE", "", 1);
        std::string genkey =
            "gpg --batch --pinentry-mode loopback --passphrase '' "
            "--quick-gen-key 'Horse Interop <interop@example.test>' "
            "ed25519 cert 2y >/dev/null 2>&1";
        if (system(genkey.c_str()) == 0) {
            std::string list_out;
            run_cmd("gpg --batch --with-colons --list-secret-keys", list_out);
            std::string fpr;
            std::string::size_type p = list_out.find("fpr:");
            if (p != std::string::npos) {
                p = list_out.find(':', p);
                /* fpr:::::::::HEX: -> field 10 */
                int col = 0;
                std::string::size_type i = p;
                while (i < list_out.size() && col < 9) {
                    if (list_out[i] == ':')
                        col++;
                    i++;
                }
                std::string::size_type end = list_out.find(':', i);
                if (end != std::string::npos)
                    fpr = list_out.substr(i, end - i);
            }
            if (!fpr.empty()) {
                std::string add1 =
                    "gpg --batch --pinentry-mode loopback --passphrase '' "
                    "--quick-add-key "
                    + fpr + " ed25519 sign 2y >/dev/null 2>&1";
                std::string add2 =
                    "gpg --batch --pinentry-mode loopback --passphrase '' "
                    "--quick-add-key "
                    + fpr + " cv25519 encr 2y >/dev/null 2>&1";
                system(add1.c_str());
                system(add2.c_str());
                std::string ext = std::string("python3 \"") + py
                                + "\" --json openpgp-extract-identity opgp "
                                + "--gpg " + fpr + " --allow-unmarked --force";
                std::string ext_out;
                if (run_cmd(ext, ext_out) == 0) {
                    std::string src2 = openrtx + "/horse_identity_opgp.bin";
                    cmd = "cp \"" + src2 + "\" \"" + dst + "\"";
                    run_cmd(cmd, unused);
                    horse_keystore_init();
                    if (!horse_keystore_unlock(pass, strlen(pass))) {
                        std::printf(
                            "horse_host_interop: OpenPGP identity unlock failed\n");
                        return -1;
                    }
                    horse_keystore_lock();
                    std::printf(
                        "horse_host_interop: OpenPGP extract unlocked\n");
                } else {
                    std::printf(
                        "horse_host_interop: OpenPGP extract skipped/failed\n%s\n",
                        ext_out.c_str());
                }
            }
        }
        unsetenv("GNUPGHOME");
        unsetenv("HORSE_OPENPGP_PASSPHRASE");
    }

    unsetenv("HORSE_PASSPHRASE");
    std::printf(
        "horse_host_interop: python blobs read by C, wrong pass rejected\n");
    return 0;
#endif
}
