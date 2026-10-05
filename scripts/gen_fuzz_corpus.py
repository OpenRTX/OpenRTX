#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Generate minimal seed corpus for Horse libFuzzer targets.
#

import os
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CORPUS_BASE = os.path.join(SCRIPT_DIR, "..", "tests", "fuzz", "corpus")

LSF_SYNC = bytes([0x15, 0x57])
VOICE_SYNC = bytes([0x45, 0xFD])
EOT_SYNC = bytes([0x77, 0x74])


def write_horse_frame(base):
    d = os.path.join(base, "fuzz_horse_frame")
    os.makedirs(d, exist_ok=True)
    for name, sync in [("seed_lsf.bin", LSF_SYNC),
                       ("seed_voice.bin", VOICE_SYNC),
                       ("seed_eot.bin", EOT_SYNC)]:
        with open(os.path.join(d, name), "wb") as f:
            f.write(sync + bytes(46))
        print("Wrote", os.path.join(d, name))


def write_horse_voice(base):
    d = os.path.join(base, "fuzz_horse_voice")
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, "seed_46.bin"), "wb") as f:
        f.write(bytes(46))
    print("Wrote", os.path.join(d, "seed_46.bin"))


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else CORPUS_BASE
    os.makedirs(base, exist_ok=True)
    write_horse_frame(base)
    write_horse_voice(base)
    print("Corpus generation done.")


if __name__ == "__main__":
    main()
