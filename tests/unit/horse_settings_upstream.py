#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
"""Compare settings.h to upstream/master. Extra meson test args are ignored."""

import subprocess
import sys

MESON_SKIP = 77


def main():
    probe = subprocess.run(
        ["git", "rev-parse", "--verify", "upstream/master"],
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
        check=False,
    )
    if probe.returncode != 0:
        print("horse_settings_upstream: skip (no upstream/master)")
        return MESON_SKIP
    diff = subprocess.run(
        [
            "git",
            "diff",
            "--exit-code",
            "upstream/master",
            "--",
            "openrtx/include/core/settings.h",
        ],
        check=False,
    )
    return diff.returncode


if __name__ == "__main__":
    sys.exit(main())
