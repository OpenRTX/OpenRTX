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

LSF_SYNC = bytes([0x5A, 0xA7])
VOICE_SYNC = bytes([0x7E, 0x9B])
EOT_SYNC = bytes([0x3C, 0xD8])


def write_horse_frame(base):
    d = os.path.join(base, "fuzz_horse_frame")
    os.makedirs(d, exist_ok=True)
    for name, sync in [("seed_lsf.bin", LSF_SYNC),
                       ("seed_voice.bin", VOICE_SYNC),
                       ("seed_eot.bin", EOT_SYNC)]:
        with open(os.path.join(d, name), "wb") as f:
            f.write(sync + bytes(46))
        print("Wrote", os.path.join(d, name))


def write_ldpc_horse(base):
    d = os.path.join(base, "fuzz_ldpc_horse")
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, "seed_46.bin"), "wb") as f:
        f.write(bytes(46))
    print("Wrote", os.path.join(d, "seed_46.bin"))


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else CORPUS_BASE
    os.makedirs(base, exist_ok=True)
    write_horse_frame(base)
    write_ldpc_horse(base)
    print("Corpus generation done.")


if __name__ == "__main__":
    main()
