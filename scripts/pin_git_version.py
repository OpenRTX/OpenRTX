#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
"""Rewrite meson.build to hard-code GIT_VERSION (upstream has no option)."""

from __future__ import annotations

import argparse
import pathlib
import re
import sys


def main() -> int:
    ap = argparse.ArgumentParser()
    ap.add_argument("version")
    ap.add_argument("meson_build", nargs="?", default="meson.build")
    args = ap.parse_args()
    path = pathlib.Path(args.meson_build)
    text = path.read_text()
    repl = f'git_version = \'"{args.version}"\''
    text2, n = re.subn(
        r"git_version\s*=\s*'\"'\s*\+\s*r\.stdout\(\)\.strip\(\)\s*\+\s*'\"'",
        repl,
        text,
        count=1,
    )
    if n != 1:
        print(
            "failed to pin GIT_VERSION in %s (matches=%u)" % (path, n),
            file=sys.stderr,
        )
        return 1
    path.write_text(text2)
    return 0


if __name__ == "__main__":
    sys.exit(main())
