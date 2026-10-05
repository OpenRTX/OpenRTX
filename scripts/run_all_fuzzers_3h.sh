#!/bin/bash
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Run all libFuzzer targets in parallel for 3 hours each, using nohup.
# Usage: ./scripts/run_all_fuzzers_3h.sh [build_dir]
# Build first: meson setup build -Dfuzzing=true && ninja -C build
# Logs: fuzz_artifacts/logs/<name>.log
#

set -e

BUILD_DIR="${1:-build}"
DURATION_SEC=$((3 * 3600))   # 3 hours

if [ ! -d "$BUILD_DIR" ]; then
    echo "Build directory not found: $BUILD_DIR"
    echo "Build with: CC=clang CXX=clang++ meson setup $BUILD_DIR -Dfuzzing=true && ninja -C $BUILD_DIR"
    exit 1
fi

FUZZERS="fuzz_horse_frame fuzz_ldpc_horse"
CORPUS_BASE="tests/fuzz/corpus"
ARTIFACTS_BASE="fuzz_artifacts"
LOG_DIR="${ARTIFACTS_BASE}/logs"
DICT_FRAME="tests/fuzz/dict/frame_sync_words.dict"
mkdir -p "$LOG_DIR"

pids=""
for name in $FUZZERS; do
    bin="${BUILD_DIR}/${name}"
    if [ ! -f "$bin" ]; then
        echo "Skipping $name (binary not found: $bin)"
        continue
    fi
    corpus="${CORPUS_BASE}/${name}"
    artifacts="${ARTIFACTS_BASE}/${name}"
    log="${LOG_DIR}/${name}.log"
    mkdir -p "$corpus" "$artifacts"
    extra_args=""
    case "$name" in
        fuzz_ldpc_horse)  extra_args="-max_len=92" ;;
        fuzz_horse_frame) extra_args="-max_len=240"
            [ -f "$DICT_FRAME" ] && extra_args="$extra_args -dict=$DICT_FRAME" ;;
    esac
    echo "Starting $name in background (3h, log=$log)"
    nohup "$bin" -max_total_time="$DURATION_SEC" -print_final_stats=1 $extra_args "$corpus" "$artifacts" > "$log" 2>&1 &
    pids="$pids $!"
done

if [ -z "$pids" ]; then
    echo "No fuzzers started."
    exit 1
fi

echo "PIDs:$pids"
echo "Wait for all with: wait $pids"
echo "Or run in background: nohup $0 $BUILD_DIR > ${LOG_DIR}/master.log 2>&1 &"
wait $pids
echo "All fuzzers completed."
