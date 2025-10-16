#!/bin/bash
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later

# Script to verify that pre-generated locale files are up-to-date
# Used in CI to ensure developers regenerate files after editing PO files

set -e

echo "Checking if locale files are up-to-date..."

# Ensure Python venv exists
if [ ! -d ".venv" ]; then
    echo "Error: Python virtual environment not found. Run:"
    echo "  python3 -m venv .venv"
    echo "  source .venv/bin/activate"
    echo "  pip install -r requirements.txt"
    exit 1
fi

LOCALES="cs de en_US es fr it_IT ja ko mk nl pl_PL pt_BR sv th uk_UA zh_Hans"

# Regenerate all locale files
for locale in $LOCALES; do
    echo "Regenerating $locale..."
    .venv/bin/python3 scripts/gen_locale.py \
        --struct openrtx/include/ui/ui_strings.h \
        --po openrtx/src/locale/${locale}.po \
        --output openrtx/src/core/locale_generated/ui_strings_${locale}.c
done

# Check if any files changed
if ! git diff --exit-code openrtx/src/core/locale_generated/*.c > /dev/null; then
    echo ""
    echo "ERROR: Pre-generated locale files are out of date!"
    echo ""
    echo "The following files have changed:"
    git diff --name-only openrtx/src/core/locale_generated/*.c
    echo ""
    echo "Please regenerate the locale files by running:"
    echo "  source .venv/bin/activate  # or .venv/Scripts/activate on Windows"
    echo "  meson compile -C build_linux regenerate-locales"
    echo ""
    echo "Then commit the updated files."
    exit 1
fi

echo "✓ All locale files are up-to-date"
