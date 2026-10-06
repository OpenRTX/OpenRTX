#!/bin/bash
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Run libFuzzer for OpenRTX fuzz targets.
# Build first with: meson setup build -Dfuzzing=true && ninja -C build
#

set -e

BUILD_DIR="${1:-build}"
TARGET="${2:-fuzz_horse_frame}"
CORPUS_DIR="${3:-corpus_${TARGET}}"
ARTIFACTS_DIR="${4:-artifacts_${TARGET}}"

BIN="${BUILD_DIR}/${TARGET}"
if [ ! -f "$BIN" ]; then
    echo "Fuzz binary not found: $BIN"
    echo "Build with: meson setup build -Dfuzzing=true && ninja -C build"
    exit 1
fi

mkdir -p "$CORPUS_DIR"
mkdir -p "$ARTIFACTS_DIR"

echo "Running $TARGET with corpus=$CORPUS_DIR artifacts=$ARTIFACTS_DIR"
exec "$BIN" "$CORPUS_DIR" "$ARTIFACTS_DIR"
