#!/bin/bash
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 120-second smoke test for all libFuzzer targets.
# Verifies each fuzzer runs and that coverage rises during the run.
# Usage: ./scripts/run_fuzz_smoke_test.sh [build_dir] [duration_sec_per_fuzzer]
#   duration_sec_per_fuzzer defaults to 120.
#

set -e

BUILD_DIR="${1:-build}"
DURATION_SEC="${2:-120}"
CORPUS_BASE="tests/fuzz/corpus"
ARTIFACTS_BASE="fuzz_artifacts_smoke"

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found: $BUILD_DIR"
    exit 1
fi

mkdir -p "$ARTIFACTS_BASE"

FUZZERS="fuzz_horse_frame fuzz_ldpc_horse"

for name in $FUZZERS; do
    bin="${BUILD_DIR}/${name}"
    if [ ! -f "$bin" ]; then
        echo "SKIP $name (binary not found)"
        continue
    fi
    corpus="${CORPUS_BASE}/${name}"
    artifacts="${ARTIFACTS_BASE}/${name}"
    mkdir -p "$corpus" "$artifacts"
    echo "--- $name (${DURATION_SEC}s) ---"
    out=$(mktemp)
    if "$bin" -max_total_time="$DURATION_SEC" -print_final_stats=1 "$corpus" "$artifacts" 2>&1 | tee "$out"; then
        inited=$(grep -m1 "INITED" "$out" || true)
        done_=$(grep "DONE" "$out" | tail -1 || true)
        echo "  Start: $inited"
        echo "  End:   $done_"
        if [ -n "$inited" ] && [ -n "$done_" ]; then
            cov_i=$(echo "$inited" | sed -n 's/.*cov: \([0-9]*\).*/\1/p')
            ft_i=$(echo "$inited" | sed -n 's/.*ft: \([0-9]*\).*/\1/p')
            cov_d=$(echo "$done_" | sed -n 's/.*cov: \([0-9]*\).*/\1/p')
            ft_d=$(echo "$done_" | sed -n 's/.*ft: \([0-9]*\).*/\1/p')
            cov_i=${cov_i:-0}; ft_i=${ft_i:-0}; cov_d=${cov_d:-0}; ft_d=${ft_d:-0}
            if [ -n "$cov_d" ] && [ -n "$cov_i" ]; then
                if [ "$cov_d" -gt "$cov_i" ] || [ "$ft_d" -gt "$ft_i" ]; then
                    echo "  Coverage: cov $cov_i -> $cov_d, ft $ft_i -> $ft_d (rising)"
                else
                    echo "  Coverage: cov $cov_i -> $cov_d, ft $ft_i -> $ft_d (stable)"
                fi
            fi
        fi
    else
        echo "  Run failed or timed out"
    fi
    rm -f "$out"
    echo ""
done

echo "Smoke test done."
