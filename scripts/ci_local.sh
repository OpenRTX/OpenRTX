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

export HORSE_REQUIRE_CRYPTO=1

echo "== clang-format --check =="
./scripts/clang_format.sh --check

echo "== check_strings =="
python3 ./scripts/check_strings.py

echo "== ruff =="
if command -v ruff >/dev/null 2>&1; then
    ruff check scripts/horse_provision.py scripts/horse_openpgp.py tests/python
elif command -v uvx >/dev/null 2>&1; then
    uvx ruff check scripts/horse_provision.py scripts/horse_openpgp.py tests/python
else
    echo "ruff not installed; skipping (CI runs ruff in format.yml)" >&2
fi

echo "== REUSE lint =="
if command -v reuse >/dev/null 2>&1; then
    reuse lint
elif command -v uvx >/dev/null 2>&1; then
    uvx reuse lint
else
    echo "reuse not installed; skipping (CI runs fsfe/reuse-action@v6)" >&2
fi

echo "== horse-openpgp (cargo) =="
if command -v cargo >/dev/null 2>&1; then
    # Keep target/ out of the tree so REUSE lint does not scan build artifacts.
    export CARGO_TARGET_DIR="${CARGO_TARGET_DIR:-$ROOT/.cache/horse-openpgp-target}"
    mkdir -p "$CARGO_TARGET_DIR"
    (
        cd tools/horse-openpgp
        cargo test --locked
        cargo build --release --locked
    )
    export HORSE_OPENPGP_HELPER="$CARGO_TARGET_DIR/release/horse-openpgp"
    echo "HORSE_OPENPGP_HELPER=$HORSE_OPENPGP_HELPER"
else
    echo "cargo not installed; OpenPGP helper tests will fail under HORSE_REQUIRE_CRYPTO=1" >&2
fi

if command -v sq >/dev/null 2>&1; then
    echo "== sq $($(command -v sq) --version 2>/dev/null | head -1 || true) =="
else
    echo "sq not installed; apt package 'sq' (0.33 on Ubuntu 24.04) is used in CI" >&2
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
  horse_randombytes_test audio_codec_test horse_provision_sock_test

echo "== meson test --test-args '--reporter junit' =="
meson test -C "$BUILD_DIR" --no-rebuild --print-errorlogs \
  --test-args '--reporter junit'

echo "== cross-ARM byte-identical (optional) =="
if command -v arm-miosix-eabi-gcc >/dev/null 2>&1 \
    && [[ -f cross_cm4.txt ]]; then
    GV=ci-local-byte-identical
    meson setup --cross-file cross_cm4.txt build_cm4_byte -Dgit_version="$GV" \
        || meson setup --reconfigure --cross-file cross_cm4.txt build_cm4_byte \
            -Dgit_version="$GV"
    meson compile -C build_cm4_byte openrtx_mduv3x0 openrtx_gd77 openrtx_md3x0
    arm-miosix-eabi-objcopy -O binary build_cm4_byte/openrtx_mduv3x0 \
        /tmp/openrtx_mduv3x0_fork.bin
    arm-miosix-eabi-objcopy -O binary build_cm4_byte/openrtx_gd77 \
        /tmp/openrtx_gd77_fork.bin
    arm-miosix-eabi-size build_cm4_byte/openrtx_md3x0
    echo "Local fork images built; full fork-vs-upstream cmp runs in CI"
else
    echo "arm-miosix-eabi toolchain not present; skipping cross-ARM byte-identical"
    echo "(CI job byte-identical covers this)"
fi

echo "ci_local.sh: all checks passed"
