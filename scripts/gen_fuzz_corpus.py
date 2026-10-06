#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Generate seed corpus for Horse libFuzzer targets (v2 sync + formats).
#

import os
import struct
import sys

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
CORPUS_BASE = os.path.join(SCRIPT_DIR, "..", "tests", "fuzz", "corpus")

LSF_SYNC = bytes([0x15, 0x57])
VOICE_SYNC = bytes([0x45, 0xFD])
EOT_SYNC = bytes([0x77, 0x74])
LSF_SYNC_V1 = bytes([0x5A, 0xA7])


def write_bin(path, data):
    os.makedirs(os.path.dirname(path), exist_ok=True)
    with open(path, "wb") as f:
        f.write(data)
    print("Wrote", path)


def write_horse_frame(base):
    d = os.path.join(base, "fuzz_horse_frame")
    # Single-frame structural seeds
    write_bin(os.path.join(d, "seed_lsf.bin"), LSF_SYNC + bytes(46))
    write_bin(os.path.join(d, "seed_voice.bin"), VOICE_SYNC + bytes(46))
    write_bin(os.path.join(d, "seed_eot.bin"), EOT_SYNC + bytes(46))
    # Three opening LSF sync frames (decoder chunk path)
    write_bin(os.path.join(d, "seed_lsf3.bin"),
              (LSF_SYNC + bytes(46)) * 3)
    # Voice then EOT (late-entry shape)
    write_bin(os.path.join(d, "seed_voice_eot.bin"),
              VOICE_SYNC + bytes(46) + EOT_SYNC + bytes(46))
    # Retired v1 sync must stay silent
    write_bin(os.path.join(d, "seed_v1_lsf.bin"), LSF_SYNC_V1 + bytes(46))
    # Ten voice frames (one fragment cycle worth of structure)
    write_bin(os.path.join(d, "seed_voice10.bin"),
              (VOICE_SYNC + bytes(46)) * 10)


def write_horse_voice(base):
    d = os.path.join(base, "fuzz_horse_voice")
    write_bin(os.path.join(d, "seed_46.bin"), bytes(46))
    # Alternating pattern in coded bytes
    pat = bytes([(i * 17) & 0xFF for i in range(46)])
    write_bin(os.path.join(d, "seed_pat.bin"), pat)


def main():
    base = sys.argv[1] if len(sys.argv) > 1 else CORPUS_BASE
    os.makedirs(base, exist_ok=True)
    write_horse_frame(base)
    write_horse_voice(base)
    print("Corpus generation done.")


if __name__ == "__main__":
    main()
