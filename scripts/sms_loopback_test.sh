#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# TEST: SMS end-to-end loopback integration test (experimental).
#
# This is a developer/e2e smoke test.  It is intentionally fragile and
# not suitable for unattended CI: it depends on the Linux emulator binary,
# NVM state, the 'sox' audio tool, and timing assumptions that may vary
# across machines.  Use it manually to verify the full TX→RX path.
#
# It drives the Linux emulator binary via its stdin command interface to:
#
#   Phase 1 (TX): navigate the new UI (Messages > New Message > Compose),
#     type a digit, press PTT to trigger m17_sms_send(), and capture the
#     48 kHz baseband written to /tmp/m17_output.raw.
#
#   Phase 2: downsample 48 kHz → 24 kHz so the file_source audio driver
#     can feed it to the M17 demodulator.
#
#   Phase 3 (RX): launch the emulator in M17 RX mode with /tmp/baseband.raw
#     in place, and wait for an "SMS_RECEIVED" line on stderr.
#
# Prerequisites:
#   - build_linux/openrtx_linux already built (script assumes this).
#   - NVM (~/.local/state/OpenRTX/state.bin) has M17 opmode selected.
#     Run the radio once, switch to M17, and it persists.
#   - 'sox' installed for resampling.
#
# Usage:
#   bash scripts/sms_loopback_test.sh              # uses build_linux/
#   bash scripts/sms_loopback_test.sh /path/to/build

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd "${SCRIPT_DIR}/.." && pwd)"

BUILD_DIR="${1:-${REPO_ROOT}/build_linux}"
BINARY="${BUILD_DIR}/openrtx_linux"
TX_SCRIPT="${REPO_ROOT}/meta/sms_send_test.txt"
RX_SCRIPT="${REPO_ROOT}/meta/sms_rx_test.txt"
TX_RAW="/tmp/m17_output.raw"
RX_INPUT="/tmp/baseband.raw"

export SDL_VIDEODRIVER=dummy

cleanup()
{
    rm -f "${TX_RAW}" "${RX_INPUT}"
}
trap cleanup EXIT

# ── Phase 1: TX ──────────────────────────────────────────────────────────────
echo "==> [Phase 1] TX: generating SMS baseband"
rm -f "${TX_RAW}"
"${BINARY}" < "${TX_SCRIPT}" 2>/dev/null

if [[ ! -s "${TX_RAW}" ]]; then
    echo "FAIL [Phase 1]: ${TX_RAW} was not produced or is empty — TX did not fire." >&2
    exit 1
fi

TX_BYTES="$(wc -c < "${TX_RAW}")"
echo "    Captured ${TX_BYTES} bytes of 48 kHz baseband"
echo "PASS [Phase 1]: TX produced ${TX_BYTES} bytes."

# ── Phase 2: downsample 48 kHz → 24 kHz ─────────────────────────────────────
echo "==> [Phase 2] Resampling TX output to 24 kHz for RX injection"
sox -r 48000 -e signed-integer -b 16 -c 1 "${TX_RAW}" \
    -r 24000 "${RX_INPUT}"
echo "    Written ${RX_INPUT} ($(wc -c < "${RX_INPUT}") bytes)"

# ── Phase 3: RX ──────────────────────────────────────────────────────────────
echo "==> [Phase 3] RX: launching emulator, waiting for SMS_RECEIVED on stderr"
RX_STDERR="$(mktemp)"
"${BINARY}" < "${RX_SCRIPT}" 2>"${RX_STDERR}" || true

echo "--- stderr output ---"
cat "${RX_STDERR}"
echo "---------------------"

if grep -q "SMS_RECEIVED" "${RX_STDERR}"; then
    echo "PASS [Phase 3]: SMS_RECEIVED detected in stderr."
    rm -f "${RX_STDERR}"
    exit 0
else
    echo "FAIL [Phase 3]: SMS_RECEIVED not found in emulator stderr." >&2
    rm -f "${RX_STDERR}"
    exit 1
fi
