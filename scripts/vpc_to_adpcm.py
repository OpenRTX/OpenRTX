#!/usr/bin/env python3
# SPDX-FileCopyrightText: Copyright 2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""Transcode OpenRTX codec2 .vpc voice prompts -> IMA-ADPCM.

The stock voiceprompts.vpc stores codec2 3200-mode bitstreams (8 bytes = one
160-sample/20 ms frame) indexed by a 350-entry TOC.  codec2 software decode is
too slow on the HD2's soft-float CK803S (proven 2026-06-10), so we transcode to
IMA-ADPCM (4-bit, integer decode, ~4 KB/s) on the host:

    .vpc prompt bytes --(c2dec 3200)--> s16 PCM 8 kHz --(2:1 decimate)-->
        s16 PCM 4 kHz --(IMA-ADPCM)--> nibbles

Clips are stored at 4 kHz (half the samples) so the FULL vocabulary fits the HD2
app-flash slot; the on-target player upsamples 2x back to the codec DAC's fixed
8 kHz.  Voice prompts are short spoken words, intelligible at a 2 kHz audio
bandwidth.

Modes:
  clip   <vpc> <c2dec> <idx> <out.h>   -- one prompt -> C header (Stage A test)
  pack   <vpc> <c2dec> <out.bin>       -- whole vocabulary -> ADPCM container

IMA-ADPCM here is mono, no block headers: each clip is a flat nibble stream
decoded from a fixed start state (predictor=0, index=0).  Low nibble of each byte
is the earlier sample.  Matches the on-target decoder in lib/codec/adpcm_ima.h,
driven by core/voicePrompts_adpcm.c.
"""
import struct
import subprocess
import sys

# --- IMA-ADPCM tables (standard) ---
STEP_TAB = [
    7, 8, 9, 10, 11, 12, 13, 14, 16, 17, 19, 21, 23, 25, 28, 31, 34, 37, 41,
    45, 50, 55, 60, 66, 73, 80, 88, 97, 107, 118, 130, 143, 157, 173, 190, 209,
    230, 253, 279, 307, 337, 371, 408, 449, 494, 544, 598, 658, 724, 796, 876,
    963, 1060, 1166, 1282, 1411, 1552, 1707, 1878, 2066, 2272, 2499, 2749, 3024,
    3327, 3660, 4026, 4428, 4871, 5358, 5894, 6484, 7132, 7845, 8630, 9493,
    10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086,
    29794, 32767]
INDEX_TAB = [-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8]


def ima_encode(pcm):
    """list[int16] -> bytes of packed 4-bit nibbles (low nibble first)."""
    pred = 0
    index = 0
    out = bytearray()
    nibbles = []
    for sample in pcm:
        step = STEP_TAB[index]
        diff = sample - pred
        code = 0
        if diff < 0:
            code = 8
            diff = -diff
        if diff >= step:
            code |= 4
            diff -= step
        if diff >= step >> 1:
            code |= 2
            diff -= step >> 1
        if diff >= step >> 2:
            code |= 1
        # mirror the decoder to keep predictor in lockstep
        diffq = step >> 3
        if code & 4:
            diffq += step
        if code & 2:
            diffq += step >> 1
        if code & 1:
            diffq += step >> 2
        if code & 8:
            pred -= diffq
        else:
            pred += diffq
        pred = max(-32768, min(32767, pred))
        index += INDEX_TAB[code]
        index = max(0, min(88, index))
        nibbles.append(code & 0xf)
    for i in range(0, len(nibbles), 2):
        lo = nibbles[i]
        hi = nibbles[i + 1] if i + 1 < len(nibbles) else 0
        out.append(lo | (hi << 4))
    return bytes(out)


# The codec2 data region starts AFTER a 7-byte codec2 header (CODEC2_HEADER_SIZE
# in voicePrompts.c::fetchCodec2Data: base = sizeof(header)+sizeof(TOC)+7).
# TOC offsets are relative to that post-header base.  Skipping these 7 bytes is
# essential -- feeding them to c2dec misaligns every frame (garbage/squeal).
CODEC2_HEADER_SIZE = 7


def read_vpc(path):
    d = open(path, "rb").read()
    magic, ver = struct.unpack_from("<II", d, 0)
    assert magic == 0x5056, f"bad magic {magic:#x}"
    toc = struct.unpack_from("<350I", d, 8)
    data = d[8 + 350 * 4 + CODEC2_HEADER_SIZE:]
    return toc, data


def c2_to_pcm(c2dec, c2bytes):
    """Run c2dec 3200 over a codec2 bitstream -> list[int16] PCM (8 kHz)."""
    p = subprocess.run([c2dec, "3200", "-", "-"], input=c2bytes,
                       capture_output=True, check=True)
    return list(struct.unpack(f"<{len(p.stdout)//2}h", p.stdout))


import math


def _lowpass_taps(fs, fc, ntaps):
    """Hann-windowed-sinc low-pass FIR, normalized to unity DC gain."""
    m = ntaps - 1
    h = []
    for n in range(ntaps):
        x = n - m / 2.0
        s = 2 * fc / fs if x == 0 else math.sin(2 * math.pi * fc * x / fs) / (math.pi * x)
        w = 0.5 - 0.5 * math.cos(2 * math.pi * n / m)   # Hann
        h.append(s * w)
    g = sum(h)
    return [t / g for t in h]


# ~1.8 kHz cutoff, 23-tap: a real anti-alias filter (the old 2-tap box left
# 2-3 kHz content to alias into the passband as garbage, muddying the speech).
_AA = _lowpass_taps(8000.0, 1800.0, 23)


def decimate2x(pcm):
    """8 kHz -> 4 kHz, halving the sample count.  Low-pass below the 2 kHz new
    Nyquist with a windowed-sinc FIR, THEN drop every other sample.  Proper
    anti-aliasing keeps short speech prompts intelligible; the on-target player
    reverses this with a 2x linear-interpolation upsample."""
    n = len(pcm)
    m = len(_AA)
    half = m // 2
    out = []
    for i in range(0, n - 1, 2):
        acc = 0.0
        for k in range(m):
            j = i + k - half
            if 0 <= j < n:
                acc += pcm[j] * _AA[k]
        v = int(round(acc))
        out.append(max(-32768, min(32767, v)))
    return out


def prompt_bytes(toc, data, idx):
    start = toc[idx]
    # The last TOC entry has no successor; its clip runs to end-of-data.
    end = toc[idx + 1] if idx + 1 < len(toc) else len(data)
    length = (end - start) // 8 * 8   # whole 3200 frames only
    return data[start:start + length]


def cmd_clip(vpc, c2dec, idx, out_h):
    idx = int(idx)
    toc, data = read_vpc(vpc)
    pcm = decimate2x(c2_to_pcm(c2dec, prompt_bytes(toc, data, idx)))
    adpcm = ima_encode(pcm)
    with open(out_h, "w") as f:
        f.write("/* Auto-generated by scripts/vpc_to_adpcm.py -- do not edit. */\n")
        f.write(f"/* .vpc prompt idx {idx}: {len(pcm)} samples "
                f"({len(pcm)*1000//8000} ms) -> {len(adpcm)} ADPCM bytes */\n")
        f.write("#include <stdint.h>\n")
        f.write(f"const unsigned hd2_adpcm_sample_count = {len(pcm)};\n")
        f.write(f"const unsigned char hd2_adpcm_sample[{len(adpcm)}] = {{\n")
        for i in range(0, len(adpcm), 16):
            f.write("  " + ",".join(str(b) for b in adpcm[i:i+16]) + ",\n")
        f.write("};\n")
    print(f"wrote {out_h}: {len(pcm)} samples -> {len(adpcm)} bytes "
          f"({len(adpcm)*8000//len(pcm)} B/s effective)")


def cmd_pack(vpc, c2dec, out_bin, include=None):
    """include: optional set of TOC indices to embed; others -> empty (the
    on-target player treats an empty clip as silence).  Default = all."""
    toc, data = read_vpc(vpc)
    clips = []
    for idx in range(350):
        if (include is not None) and (idx not in include):
            clips.append(b"")
            continue
        b = prompt_bytes(toc, data, idx) if toc[idx] or idx == 0 else b""
        if not b:
            clips.append(b"")
            continue
        clips.append(ima_encode(decimate2x(c2_to_pcm(c2dec, b))))
    # container: magic 'VPA1', count, TOC[count+1] u32 byte offsets, data (4 kHz)
    blob = bytearray()
    header = struct.pack("<II", 0x31415056, 350)  # 'VPA1', 350 entries
    offsets = []
    cur = 0
    for c in clips:
        offsets.append(cur)
        cur += len(c)
    offsets.append(cur)
    toc_bytes = struct.pack(f"<{len(offsets)}I", *offsets)
    for c in clips:
        blob += c
    out = header + toc_bytes + bytes(blob)
    open(out_bin, "wb").write(out)
    nonzero = sum(1 for c in clips if c)
    print(f"wrote {out_bin}: {nonzero} clips, {len(blob)} ADPCM bytes, "
          f"{len(out)} total")


if __name__ == "__main__":
    cmd = sys.argv[1] if len(sys.argv) > 1 else ""
    if cmd == "clip":
        cmd_clip(*sys.argv[2:6])
    elif cmd == "pack":
        cmd_pack(*sys.argv[2:5])
    elif cmd == "subset":
        # subset <vpc> <c2dec> <out.bin> [lo-hi,lo-hi,...]  (default 0-107:
        # silence,digits,A-Z,phonetics,units,directions,nouns,point/plus/minus)
        spec = sys.argv[5] if len(sys.argv) > 5 else "0-107"
        inc = set()
        for part in spec.split(","):
            if "-" in part:
                a, b = part.split("-")
                inc.update(range(int(a), int(b) + 1))
            else:
                inc.add(int(part))
        cmd_pack(sys.argv[2], sys.argv[3], sys.argv[4], include=inc)
    else:
        print(__doc__)
        sys.exit(1)
