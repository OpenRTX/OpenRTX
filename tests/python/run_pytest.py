#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Run pytest, ignoring Catch2 --reporter args appended by meson test."""

from __future__ import annotations

import sys

import pytest


def strip_reporter(argv):
    out = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--reporter" and i + 1 < len(argv):
            i += 2
            continue
        if a.startswith("--reporter="):
            i += 1
            continue
        out.append(a)
        i += 1
    return out


if __name__ == "__main__":
    sys.exit(pytest.main(strip_reporter(sys.argv[1:])))
