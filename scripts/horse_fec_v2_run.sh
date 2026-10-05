#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
# Host Horse v2 FEC campaign. Fixed seeds. Does not change firmware.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
BIN="${ROOT}/build_linux/horse_fec_v2_sim"
OUT="${ROOT}/tests/unit/horse_fec_v2_results.txt"
: > "$OUT"
run() {
    echo "$*" | tee -a "$OUT"
    "$@" | tee -a "$OUT"
}
run "$BIN" selftest
# C20 re-check, seed 1, 200 transmissions.
for n in 2000 5000 10000 12500 15000; do
    run "$BIN" complete_v1 "$n" 200 1
done
# Voice FER: 10000 frames, seed 1. Polar L16 and CCSDS I=50 use 1000 frames
# after the main grid (decode cost).
NOISES="0 4000 6000 8000 10000 11000 12000 12500 14000 15000 16000 18000"
for n in $NOISES; do
    for id in 0 1 4 5 6; do
        run "$BIN" voice "$id" "$n" 10000 1
    done
    run "$BIN" voice 2 "$n" 2000 1
done
for n in 10000 12500 15000; do
    run "$BIN" voice 3 "$n" 1000 1
    run "$BIN" voice 8 "$n" 1000 1
done
for nsym in 4 8 16 32; do
    for intl in 0 1; do
        run "$BIN" burst "$nsym" "$intl" 1000
    done
done
echo "done" | tee -a "$OUT"
