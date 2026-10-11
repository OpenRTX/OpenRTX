#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Native checks matching GitHub "Format sources" and "Build and test"
# unit-test (not ARM firmware / Zephyr). Run before pushing.
#
# Usage:
#   bash scripts/ci_local.sh
#   bash scripts/ci_local.sh BUILD_DIR

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
cd "$ROOT"
BUILD_DIR="${1:-build_linux}"

echo "== clang-format --check =="
./scripts/clang_format.sh --check

echo "== check_strings =="
python3 ./scripts/check_strings.py

echo "== REUSE lint =="
if command -v reuse >/dev/null 2>&1; then
    reuse lint
elif command -v uvx >/dev/null 2>&1; then
    uvx reuse lint
else
    echo "reuse not installed; skipping (CI runs fsfe/reuse-action@v6)" >&2
fi

if [[ ! -d "$BUILD_DIR" ]]; then
    meson setup "$BUILD_DIR"
else
    meson setup --reconfigure "$BUILD_DIR"
fi

echo "== meson compile (native tests) =="
meson compile -C "$BUILD_DIR" linux \
  m17_golay_test m17_viterbi_test m17_callsign_test m17_metatext_test \
  m17_demodulator_test m17_rrc_test cps_test minmea_conversion_test \
  horse_frame_test horse_crypto_test horse_info_test horse_codec_test \
  horse_peers_test horse_keystore_test horse_host_interop_test \
  horse_loopback_test ui_check_standby_test m17_packet_test \
  dsp_oversampling_test gfx_text_test m17_replay_test cps_layout_test \
  horse_fec_v2_sim horse_crypto_worker_test horse_tx_fail_test \
  horse_randombytes_test

echo "== meson test --test-args '--reporter junit' =="
meson test -C "$BUILD_DIR" --no-rebuild --print-errorlogs \
  --test-args '--reporter junit'

echo "ci_local.sh: all checks passed"
