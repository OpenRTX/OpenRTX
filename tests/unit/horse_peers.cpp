/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 *
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "core/cps.h"
#include "protocols/horse/horse_peers.h"
#include "interfaces/cps_io.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include <stdio.h>

int main()
{
    if (sizeof(contact_t) != 39) {
        std::printf("horse_peers_test: contact_t is %zu, want 39\n",
                    sizeof(contact_t));
        return -1;
    }

    const char *path = "/tmp/horse_upstream.rtxc";
    FILE *f = fopen(path, "wb");
    if (f == NULL)
        return -1;

    cps_header_t hdr;
    contact_t ct;
    memset(&hdr, 0, sizeof hdr);
    memset(&ct, 0, sizeof ct);
    hdr.magic = CPS_MAGIC;
    hdr.version_number = CPS_VERSION_NUMBER;
    hdr.ct_count = 1;
    strncpy(ct.name, "UpstreamName", CPS_STR_SIZE);
    ct.mode = 0;
    ct.info.m17.address[0] = 0xAA;
    if (fwrite(&hdr, sizeof hdr, 1, f) != 1
        || fwrite(&ct, sizeof ct, 1, f) != 1) {
        fclose(f);
        return -1;
    }
    fclose(f);

    if (cps_open(const_cast<char *>(path)) != 0) {
        std::printf("horse_peers_test: cps_open failed\n");
        return -1;
    }
    contact_t got;
    memset(&got, 0xff, sizeof got);
    if (cps_readContact(&got, 0) != 0) {
        std::printf("horse_peers_test: cps_readContact failed\n");
        return -1;
    }
    cps_close();
    if (strncmp(got.name, "UpstreamName", CPS_STR_SIZE) != 0 || got.mode != 0
        || got.info.m17.address[0] != 0xAA) {
        std::printf("horse_peers_test: upstream contact mutated\n");
        return -1;
    }

    char xdg[64];
    snprintf(xdg, sizeof xdg, "/tmp/horse_peersXXXXXX");
    if (mkdtemp(xdg) == NULL)
        return -1;
    char appdir[96];
    snprintf(appdir, sizeof appdir, "%s/OpenRTX", xdg);
    mkdir(appdir, 0700);
    setenv("XDG_STATE_HOME", xdg, 1);
    char peersbin[128];
    snprintf(peersbin, sizeof peersbin, "%s/OpenRTX/horse_peers.bin", xdg);
    unlink(peersbin);

    horse_peer_t peer;
    horse_peer_t round;
    memset(&peer, 0, sizeof peer);
    peer.address[0] = 0x11;
    memset(peer.x25519_pk, 0x22, sizeof peer.x25519_pk);
    memset(peer.ed25519_pk, 0x33, sizeof peer.ed25519_pk);
    if (!horse_peer_write(1, &peer) || !horse_peer_read(1, &round)) {
        std::printf("horse_peers_test: peer rw failed\n");
        return -1;
    }
    if (memcmp(&peer, &round, sizeof peer) != 0) {
        std::printf("horse_peers_test: peer mismatch\n");
        return -1;
    }
    if (!horse_peer_has_x25519(&peer) || !horse_peer_has_ed25519(&peer))
        return -1;
    if (horse_peer_read(0, &round)
        || horse_peer_read(HORSE_PEER_MAX + 1, &round))
        return -1;

    std::printf("horse_peers_test: passed\n");
    return 0;
}
