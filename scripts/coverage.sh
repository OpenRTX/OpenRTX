#!/usr/bin/env bash
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Generate a code coverage report for OpenRTX unit tests.
#
# Usage:
#   bash scripts/coverage.sh              # full local workflow (setup, test, report)
#   bash scripts/coverage.sh --ci BUILD   # report-only mode for CI (skips setup/test)

set -euo pipefail

CI_MODE=false
BUILD_DIR=""

while [[ $# -gt 0 ]]; do
    case "$1" in
        --ci)
            CI_MODE=true
            shift
            ;;
        *)
            BUILD_DIR="$1"
            shift
            ;;
    esac
done

BUILD_DIR="${BUILD_DIR:-build_coverage}"
REPORT_DIR="${BUILD_DIR}/coverage"

if [ "$CI_MODE" = false ]; then
    # Set up the build with coverage enabled (if not already configured)
    if [ ! -f "${BUILD_DIR}/build.ninja" ]; then
        meson setup "${BUILD_DIR}" -Db_coverage=true
    fi

    # Run tests (meson test automatically builds required test targets)
    meson test -C "${BUILD_DIR}" --print-errorlogs
fi

# Generate coverage report
mkdir -p "${REPORT_DIR}"

GCOVR_ARGS=(
    "${BUILD_DIR}"
    --root .
    --filter 'openrtx/src/'
    --filter 'openrtx/include/'
    --exclude '.*subprojects.*'
    --exclude '.*test.*'
    --cobertura "${REPORT_DIR}/cobertura.xml"
    --markdown "${REPORT_DIR}/coverage.md"
    --print-summary
)

if [ "$CI_MODE" = false ]; then
    GCOVR_ARGS+=(
        --html-details "${REPORT_DIR}/index.html"
        --decisions
        --calls
    )
fi

gcovr "${GCOVR_ARGS[@]}"

echo ""
echo "Cobertura XML: ${REPORT_DIR}/cobertura.xml"
if [ "$CI_MODE" = false ]; then
    echo "HTML report: ${REPORT_DIR}/index.html"
fi
