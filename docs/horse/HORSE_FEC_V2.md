# Horse FEC version 2 study

Date: 2026-10-05
Branch: `upstream-sync`
Status: implemented on `upstream-sync` (protocol version 2). Normative
rules in section 7b; `horse.md` describes version 2 as built.

Simulation: `tests/unit/horse_fec_v2_sim.cpp`,
`tests/unit/horse_fec_v2_codes.hpp`,
`tests/unit/horse_fec_v2_polar_q.inc`,
``scripts/horse_fec_v2_run.sh`. Results:
`tests/unit/horse_fec_v2_results.txt` and
`tests/unit/horse_fec_v2_round2.txt`. Commands to reproduce are in
section 19. Fixed seed `1` unless noted.
Impairment: the analog loopback of `horse_loopback.cpp` (gain 1.0,
additive uniform-amplitude noise in ADC counts, `HorseModulator` then
`HorseDemodulator` with `setSkipDcBlock(true)`).

## Recommendation (normative for implementation)

**Voice:** option C (M17 convolution, puncture, interleaver, decorrelator).
Polar list-4 is closed for MD-3x0 (section 12, >15 ms Ir estimate).

**LSF / late entry:** section 7b (A--E): three coded opening frames with
CRC-16; cycle-10 spare fragments (LSF + signature); majority combine;
voice-sync late entry; new sync words; version byte 2.

No compatibility path for version 1.

## 1. Version 1 facts checked against the code

C17. `ldpc_horse.c` encodes by duplicating each of
`LDPC_VOICE_PAYLOAD_BITS` (184) bits and decodes with
`in[2i] & in[2i+1]` (disagreement becomes 0). That detects some
errors and does not correct them. `HorseFrameEncoder::encodeVoiceFrameWithFn`
zeros a 23-byte buffer, writes FN (16 bits), codec (96 bits), tag
(32 bits) = 144 used bits, then 40 zero bits, then repeat-2 to 368
coded bits after the 16-bit sync. The 144-bit figure in the audit is
the used payload, not the repeat-2 input.

C20. `encodeLsf` copies 46 bytes with no CRC and no FEC. Decoder
accepts Hamming distance 2 on the LSF sync (acquire is Hamming 0).
The 3-frame intact LSF+voice+EOT table in `HORSE_AUDIT.md` comes from
`test_lsf_intact_under_noise` in `horse_loopback.cpp` (gain 1.0, 200
trials, seed `1000 + t*17 + sigma`): 200/200 at 2000 and 5000, 118/200
at 10000, 56/200 at 12500, 10/200 at 15000 (was 121/57/11 before the
tracking freeze; 3 trials worse at 10000). This study also ran a
different complete-TX metric (LSF + 6 signature frames + 300 voice +
EOT, seed 1, type match only, 200 trials) in round 2. The two tables
must not be mixed; the 3-frame payload-checked rates remain the C20
citation.

## 2. Soft decisions from HorseDemodulator

`HorseDemodulator::quantize` slices each correlator sample to `{+3,+1,-1,-3}`
with thresholds at `2/3` of the estimated outer deviation. There is no
per-bit confidence API. Each 4-FSK symbol is two bits
(`HorseUtils.hpp`: `00=+1`, `01=+3`, `10=-1`, `11=-3`).

To feed polar or Viterbi with real soft values the demodulator must
keep the symbol sample `y` and the outer amplitude `A` and form two
LLRs, for example `L(b0)` from the sign of `y` and `L(b1)` from
`|y|` versus `A` (mid-point between inner and outer). That change is
firmware and was not made. This campaign mapped demodulated hard bits
to LLR `±8`. `m17_soft` is therefore SoftViterbi on sliced bits, not
analog soft; it was slightly worse than `m17_hard` (0.030 % vs
0.010 % at noise 10000). Treat analog-soft curves as unmeasured.

## 3. Option A: CRC-aided polar (5G control)

Citation: ETSI TS 138 212 V16.2.0 (3GPP TS 38.212).

- CRC: CRC24C, clause 5.1, polynomial
  `D^24+D^23+D^21+D^20+D^17+D^15+D^13+D^12+D^8+D^4+D^3+D+1`
  (`0x1B2B117` with the `D^24` term). PDCCH-class; `n_PC = 0`.
- `K = 144 + 24 = 168`. `E = 368`. `K/E = 0.4565 > 7/16`, so
  shortening (clauses 5.4.1.1 and 5.4.1.2).
- Mother length: `ceil(log2(368)) = 9`, `E = 368 > (9/8)*256 = 288`,
  so `n = 9`, `N = 512` (clause 5.3.1). `n_min = 5`.
- Frozen set: 5G Polar sequence Table 5.3.1.2-1 (copied into
  `horse_fec_v2_polar_q.inc`; verified as a permutation of `0..1023`).
  Sub-block interleaver Table 5.4.1.1-1. Shortened positions
  `J(n)` for `n = E .. N-1` are frozen; `QI` is the `K` most reliable
  remaining indices; information bits are placed in increasing index
  order (clause 5.3.1.2).
- Rate matching: first `E` bits of the sub-block-interleaved mother
  codeword (shortening, 5.4.1.2). 144 mother bits not sent.
- List sizes simulated: 4 (10000 frames/point), 8 (2000), 16 (1000).
- The 32-bit voice tag is among the 144 information bits. List
  selection uses only CRC24C. Using the tag as the CRC-aided check
  was not done: an unauthenticated decoder that accepted a tag-matching
  list path would give a forger a `2^{-32}` guess per frame without
  the session `k_tag` if the tag were compared in the clear; CA-SCL
  must not substitute the MAC for CRC24C.

Bit budget (368-bit slot after sync): 144 info + 24 CRC + 0 tail + 144
shortened (not on air) + 200 frozen in the mother + 0 spare on air.

## 4. Option B: published short LDPC only

### 5G NR base graph 2, K = 144

Citation: ETSI TS 138 212 V16.2.0 clause 5.2.2 and Table 5.3.2-1.

- `B = 144 <= 192` => `Kb = 6`. One code block, `L = 0`, `K' = 144`.
- Minimum lifting `Z` in Table 5.3.2-1 with `Kb * Z >= 144` is
  `Zc = 24` (set index 1: `{3, 6, 12, 24, 48, 96, 192, 384}`).
  **Confirmed from the specification, not from the prompt.**
- Code-block size `K = 10 * Zc = 240` (96 filler NULLs).
- Coded length before rate matching `N = 50 * Zc = 1200` (clause 5.3.2).
- Rate match to `E = 368` by clause 5.4.2.1 (bit selection from the
  circular buffer). Filler bits are not transmitted.

Table 5.3.2-3 (`H_BG` and `V_i,j` for BG2) did not extract as a complete
verified matrix from the ETSI PDF conversion (markdown-mangled cells).
**No BG2 min-sum FER is reported. No custom H was invented.**

### LDPC table files the owner must supply (exact)

Until these are present as machine-readable tables in-tree, LDPC stays
unevaluated:

1. **ETSI TS 138 212 V16.2.0 clause 5.3.2 Table 5.3.2-3** — complete
   BG2 base-graph shift coefficients `V_i,j` (and `H_BG` flags) for
   all rows/columns used at `Zc = 24` (set index 1). One CSV or
   plain integer matrix; PDF cell mangling is not enough.
2. **ETSI TS 138 212 V16.2.0 Table 5.3.2-2** — only if BG1 is also
   considered (not required for the K=144 BG2 plan above).
3. **CCSDS 230.1-G-3 Tables 4-2 and 4-3** (or 231.1-O-1 Tables 2-1
   through 2-3) — complete circulant `W` for `(512,256)` as eight
   64-bit hex columns without truncated rows, if that code is to be
   rate-matched; the books do not define 368-bit puncturing.

### CCSDS short codes

Citations: CCSDS 231.1-O-1 (Orange Book, historical) Tables 2-1, 2-2,
2-3 (generator `W` hex); CCSDS 231.0-B-4 clause 4.2 (`(128,64)` and
`(512,256)` in the Blue Book); CCSDS 230.1-G-3 Tables 4-2 and 4-3
(circulants).

- `(128,64)`: `k = 64 < 144`. Cannot carry a Horse voice payload in
  one codeword.
- `(256,128)`: in 231.1-O-1 only, not 231.0-B-4. `k = 128 < 144`.
- `(512,256)`: `n = 512 > 368`. Fitting the Horse slot requires
  puncturing or shortening that those books do not define for a
  368-bit 4-FSK frame.

The sim encoded `(512,256)` from Table 4-2 / 2-3 `W` with 112 zero
fillers and the first 368 coded bits. Min-sum on `H = [W^T | I]` is
dense (row weight ~128). A 10000-frame decode did not finish in six
minutes and was stopped. Sparse `H` figures in the PDFs did not
extract as a complete 4-by-8 circulant array (seven of eight columns
listed). **CCSDS voice FER is unverified. No custom H was used in a
completed 10000-frame run.**

## 5. Option C: M17 convolutional (baseline in tree)

Unchanged classes: `M17::ConvolutionalEncoder` (R=1/2, K=5, G1=0x19,
G2=0x17), `M17::DATA_PUNCTURE` / `LSF_PUNCTURE`, `M17::interleave`
(QPP `45x + 92x^2`), `M17::decorrelate`, `M17::HardViterbi`,
`M17::SoftViterbi`. Same construction as `FrameEncoder::encodeStreamFrame`
/ `encodeLsf`.

Voice bit budget: 144 info bits (18 bytes, same as `StreamFrame`) +
flush as in M17 (encoded[36] = truncated flush byte) -> puncture
11/12 -> 34 bytes. Remaining 96 bits of the 368-bit Horse slot are
repetition of the punctured stream (documented fill; the puncture
matrix is not modified). Then interleave and decorrelate.

LSF: Horse fields are 368 bits. M17 LSF coding is 30 bytes including
CRC16 into 368 bits in **one** frame. Horse cannot do that without
dropping the 32-byte ephemeral key. Version 2 LSF uses **two** frames
(see below).

## 6. Option D: mixed

Polar voice plus M17 LSF was not ranked. Polar list-4 cost is unmeasured
on MD-3x0 (section 12). LSF still needs a published short code that
fits 368 coded bits; this study only ran option C LSF in the harness.

## 7. LSF designs

Current fields: src 48, dst 48, eph 32*8, flags 8, version 8 = 368
bits. Version 2 adds a checksum. `crc_m17` (poly 0x5935, already in
tree) is 16 bits => 384 information bits.

| Option | Frames | Bit budget | Start delay | One frame lost | Late-entry extension |
|--------|--------|------------|-------------|----------------|----------------------|
| A polar | 3 voice-coded slots (3*144 = 432 >= 384; 48 pad) | 384 info+CRC / 1104 coded | 120 ms | Whole LSF fails unless the lost slot is treated as erasures on a single 3-frame mother code (not simulated) | Repeat the 3-frame LSF every 6 s: +5 % airtime |
| B BG2 | 1 frame if rate-matched 384->368 (too tight); 2 frames E=736 | 384 / 736 | 80 ms | Erasure of 368 of 736 | Same as A |
| C M17 | 3 frames of 18-byte chunks + CRC16, each DATA_PUNCTURE+interleave+decorrelate | 384 / 1104 | 120 ms | Whole LSF fails (no reconstruction from one chunk) | Repeat the 3-frame LSF every 6 s: +5 % airtime |
| v1 | 1 uncoded | 368 / 368, no CRC | 40 ms | Whole TX lost | None |

Recommended LSF is **3-frame M17 chunks + voice spare fragments**
(section 7b). Equal-airtime 3-frame opening results (200 trials, real
demod, seed 1): polar block 166/200 at noise 11000 vs M17 3-chunk
159/200 vs CRC copies 117/200. A fourth M17 opening frame is
160/200 at 11000 (one extra success). Polar LSF RAM is still large;
option C opening stays the cheap default now that polar voice is out.

## 7b. LSF and signature fragments (normative for v2)

Option C packs 34 punctured bytes (272 bits) then has **96 spare bits**
in the 368-bit slot. Version 2 uses those bits for a repeating
fragment cycle instead of repeating the punctured stream.

### A. Fragment cycle

| Mode | Cycle length | Contents |
|------|-------------:|----------|
| All modes | **10** frames | slots 0..3: LSF (48 B); slots 4..9: signature (64 B + 8 B zero pad) |

Cycle length is **always 10**, including encrypt-only. Late joiners
do not know the channel mode until the LSF CRC passes; a mode-dependent
length would be circular. Encrypt-only transmitters fill slots 4..9
with zeros. After LSF decode, if `LSF_FLAG_SIGNED` is clear, the
receiver ignores slots 4..9. If the channel requires signing, audio
waits until slots 4..9 rebuild the 64-byte signature and verification
succeeds.

**Fragment size:** 12 bytes (96 bits), uncoded in the spare.

**How FN identifies the slot:**

| Frame | FN range | Slot |
|-------|----------|------|
| Voice | `0 .. VOICE_FN_MAX` (0x6FFF) | `FN % 10` |
| Signature | `SIG_FRAME_BASE .. +5` (0x7000..0x7005) | `4 + (FN - SIG_FRAME_BASE)` (slots 4..9) |
| Opening LSF | (three LSF-sync frames, no FN) | not in the cycle |

Voice FN 0..9 therefore carries one full cycle (LSF then signature).
Signature frames refresh slots 4..9 once at the start of a signed TX.
Later voice frames keep refreshing the whole cycle for late joiners.

**Payload map:**

- Slot `s` in 0..3: bytes `[12*s .. 12*s+11]` of the 48-byte block
  `(46-byte LSF || crc_m17)`.
- Slot `s` in 4..9: bytes `[12*(s-4) ..]` of the 64-byte signature;
  slot 9 carries the last 4 signature bytes plus 8 zero pad bytes.

**Time to full recovery (ideal, no erasures):**

| Goal | Frames | Time (40 ms/frame) |
|------|-------:|-------------------:|
| LSF only (encrypt / unsigned) | 4 distinct slots 0..3 | 160 ms minimum; 400 ms if FN runs 0..9 |
| LSF + signature (sign / both) | 10 distinct slots 0..9 | **400 ms** minimum |

With erasures, majority combining (B) needs further repeats of each
slot; a practical budget for acceptance tests is **20 voice frames
(~800 ms)** after late-entry lock for LSF-only, and **30 frames
(~1.2 s)** when a signature is required.

### B. Fragment combining

For each slot the receiver keeps a **bit-wise majority** over the last
up to **three** hard copies (each copy is 96 bits from one frame).
Ties count as 0. After every update of slots 0..3, assemble 48 bytes
and accept the LSF only when `crc_m17` over the first 46 bytes matches
the last two. After LSF accept, if signing is required, assemble 64
bytes from slots 4..9 the same way and run the existing signature
check.

**Small code on fragments:** not worth the bits. The spare is only 96
bits; an inner code would shrink the LSF/signature payload or lengthen
the cycle. The outer `crc_m17` (LSF) and Ed25519 (signature) already
reject bad assemblies. Majority over repeats is the redundancy.

### C. Late-entry rules

1. Demodulator may **acquire on a voice sync word** with no prior LSF
   (Hamming-0 acquire, Hamming-2 track; same coast/clock rules).
2. **No audio** until, in order: LSF rebuilt and CRC-ok; session keys
   derived from the LSF fields; at least one voice/signature frame
   **tag** verified under `k_tag`; and, if the channel requires
   signing (`LSF_FLAG_SIGNED` or local policy), the **64-byte
   signature verified**.
3. **Frame numbers** must be **strictly increasing** from the first
   frame accepted for crypto/audio (same FN rules as today, including
   cap `VOICE_FN_MAX = 0x6FFF`). Frames at or behind the first accepted
   FN are dropped.
4. Opening three LSF-sync frames remain the fast path when present;
   fragments are the late-entry / erased-opening path.

### D. Version-1 signals and sync words

Version 1 has no deployed users: **replace it**. Unknown LSF version
bytes and version-1 frames produce **no audio** (fail closed).

**Sync words change in version 2.** Reasons: (1) a v1 radio locking on
a v2 LSF can pass `horse_crypto_lsf_version_ok` with probability
1/256 on a random version byte and then derive garbage keys; (2) a v2
radio must not acquire `{0x5A,0xA7}` / `{0x7E,0x9B}`; (3) new sync
words make mutual silence certain without relying on the version byte
alone. Chosen values (Hamming distance >= 6 from each other and from
v1): LSF `{0x15,0x57}`, voice `{0x45,0xFD}`, EOT `{0x77,0x74}`.
`LSF_PROTOCOL_VERSION` / `HORSE_LSF_VERSION` are **2**.

### E. DC-block off-by-one sample phase

**Evidence:** forcing `samplingPoint += 1` immediately after LSF
acquire zeroes voice on the firmware DC path (`setSkipDcBlock(false)`):
voice sync Hamming distance becomes 4. The same forced step with DC
skipped yields HD=1 and voice decodes. Cause: DC-block IIR still
settling at the LSF/voice boundary plus an off-by-one sample phase.

**Mitigation in tree (commit `69d97b54`):** `CLOCK_HOLD_FRAMES = 3`
before any TED apply, then sync-gated majority ±1 tracking. That stops
the failure mode that zeroed voice. Residual risk if software forces an
SP jump during settle remains noted in `HORSE_AUDIT.md` as mitigated,
not a separate open C-item, with this evidence. No further change is
required before v2 coding unless a new test shows hold=3 is insufficient.

### Interactions (unchanged security)

**Key derivation:** only after LSF CRC. IKM stays
`secret||src||dst||eph||flags||version` with version=2.

**Tag:** `dir||FN16||payload` under `k_tag` on every frame; no
cleartext fallback.

**Signature frames:** payload still carries the six signature chunks
at `SIG_FRAME_BASE`; spare bits concurrently refresh cycle slots 4..9
for late joiners who missed the opening SIG burst.

### LSF success curves (200 trials, seed 1, real demod)

Opening only (`lsf_cmp` kind=0):

| noise | 3-frame ok | 4-frame ok |
|------:|-----------:|-----------:|
| 8000 | 198/200 | 198/200 |
| 10000 | 178/200 | 179/200 |
| 11000 | 159/200 | 160/200 |
| 12000 | 141/200 | 143/200 |

Opening plus fragments (`lsf_frag`, erase_open=0). Success =
opening CRC or fragment rebuild; `mean_start_fr` is frames from TX
start until LSF valid (0 if opening worked). Study used cycle length 4
(LSF-only); v2 implements cycle length 10 per A:

| noise | nfr | open | frag_or_open | mean_start_fr |
|------:|----:|-----:|-------------:|--------------:|
| 8000 | 3 | 198/200 | 200/200 | 0 |
| 8000 | 4 | 198/200 | 200/200 | 0 |
| 10000 | 3 | 178/200 | 196/200 | 1 |
| 10000 | 4 | 179/200 | 195/200 | 1 |
| 11000 | 3 | 159/200 | 183/200 | 2 |
| 11000 | 4 | 160/200 | 184/200 | 2 |
| 12000 | 3 | 141/200 | 159/200 | 3 |
| 12000 | 4 | 143/200 | 162/200 | 3 |

Opening payload erased, sync kept (`erase_open=1`):

| noise | nfr | frag ok | mean_start_fr | start time |
|------:|----:|--------:|--------------:|-----------:|
| 8000 | 3 | 197/200 | 7 | ~280 ms |
| 8000 | 4 | 198/200 | 8 | ~320 ms |
| 10000 | 3 | 179/200 | 11 | ~440 ms |
| 10000 | 4 | 180/200 | 13 | ~520 ms |
| 11000 | 3 | 147/200 | 16 | ~640 ms |
| 11000 | 4 | 148/200 | 16 | ~640 ms |
| 12000 | 3 | 84/200 | 20 | ~800 ms |
| 12000 | 4 | 88/200 | 21 | ~840 ms |

**Recommended LSF design (implementation):** 3-frame M17 opening +
cycle-10 spare fragments (A--D). Drop a fourth opening frame.

**Acceptance (unchanged gate):** with fragments enabled, host analog
loopback at noise 10000 reaches **>= 198/200** LSF-valid (opening or
rebuild) within the recovery budget in A, and version-2 complete TX
meets section 16 item 1. Report figures whether or not the gate is
met; do not lower the gate.

## 8. Voice FER: `HorseDemodulator` vs ideal frame timing

FER = `1 - ok/n` with undelivered frames counted as errors (`n = 10000`).
`got` is how many voice frames were decoded. Seed 1, gain 1.0.
Lock is authenticated on the LSF (`noteValidTag`). Every code uses
the M17 randomiser once (M17 already has it; polar and repeat-2 XOR
the same sequence). Payload bytes 0-1 are the frame index so a
missing frame does not shift the reference.

**Real modem after tracking freeze:** `HorseDemodulator` keeps the
acquire sampling point (M17 TED is computed, not applied). Coast
N=4. Hamming-2 tracking. EOT still unlocks at once.

| noise | repeat-2 FER (got) | polar L4 FER (got) | polar L8 FER (got) | M17 hard FER (got) |
|------:|-------------------:|-------------------:|-------------------:|-------------------:|
| 8000 | 0.0215 (10000) | 0 (10000) | 0 (10000) | 0 (10000) |
| 10000 | 0.3757 (10000) | 0 (10000) | 0 (10000) | 0.0006 (10000) |
| 11000 | 0.6511 (10000) | 0 (10000) | 0 (10000) | 0.0044 (10000) |
| 12000 | 0.8441 (9980) | 0.0017 (9983) | 0.0017 (9983) | 0.0137 (9982) |
| 12500 | 0.8987 (9961) | 0.0035 (9965) | 0.0035 (9965) | 0.0208 (9964) |
| 13000 | 0.9435 (9961) | 0.0059 (9941) | 0.0059 (9941) | 0.0327 (9943) |
| 15000 | 1.000 (83) | 0.5933 (4069) | 0.5932 (4069) | 0.1794 (9736) |

**Ideal timing:** known RRC delay, 90th-percentile outer.

| noise | repeat-2 FER (got) | polar L4 FER (got) | polar L8 FER (got) | M17 hard FER (got) |
|------:|-------------------:|-------------------:|-------------------:|-------------------:|
| 8000 | 0.0194 (10000) | 0 (10000) | 0 (10000) | 0 (10000) |
| 10000 | 0.3272 (10000) | 0 (10000) | 0 (10000) | 0.0006 (10000) |
| 11000 | 0.5797 (10000) | 0 (10000) | 0 (10000) | 0.0034 (10000) |
| 12000 | 0.7870 (10000) | 0 (10000) | 0 (10000) | 0.0086 (10000) |
| 12500 | 0.8553 (10000) | 0 (10000) | 0 (10000) | 0.0132 (10000) |
| 13000 | 0.9082 (10000) | 0 (10000) | 0 (10000) | 0.0197 (10000) |
| 15000 | 0.9881 (10000) | 0.0001 (10000) | 0 (10000) | 0.1013 (10000) |
| 20000 | 1.000 (10000) | 0.1245 (10000) | 0.0748 (10000) | 0.7528 (10000) |

Acceptance up to 12500: delivered counts are within 0.4 % of ideal
(9961--9965 vs 10000). Polar FER is within 0.35 points of ideal
(undelivered only). M17 FER is 2.08 % vs 1.32 % (0.76 points).
Repeat-2 is already past 10 % loss by noise 10000; demod FER sits
about 4--6 points above ideal because slicing is worse, not because
lock drops. 10 % loss on the real demodulator: repeat-2 between 8000
(2.15 %) and 10000 (37.6 %); M17 by 15000 (17.9 %); polar L4/L8 when
lock collapses (~15000, 59 %), while ideal polar L4 crosses 10 %
near 20000.

Log: `tests/unit/horse_fec_v2_round3.txt`.

## 8b. Clock recovery cliff (cause, then fix)

`clktrace` at noise 12000 (400 voice + LSF + EOT, seed 1): sampling
point stayed 0, TED proposed at most `|d|=1` on the first voice,
outer levels stayed at the acquire peak. The run still delivered
402 frames after the fix.

**Cause, before the fix:** not a wandering Gardner loop. A Hamming-fail
LSF search during an unauthenticated lock wrote 8 symbols into
`demodFrame` and left `frameIndex=8`. The next 192-symbol boundary
never lined up, so `takeFrame` stopped (`got=162`, `locked=1`,
`frameIndex=184` on the old clktrace). Known-phase slicing of the
same samples still decoded.

**DC-block cause (sampling-point move after LSF):** forcing
`samplingPoint += 1` immediately after LSF acquire zeroes voice on the
firmware DC-block path (`setSkipDcBlock(false)`): voice Hamming
distance to the sync word rises to 4 so the type is not VOICE. The
same forced step with DC skipped yields HD=1 and voice still
decodes. Root cause is the DC-block IIR still settling at the LSF
boundary plus an off-by-one sample phase: the first voice frame is
sliced on the wrong 5-sample grid. Unbounded TED apply after acquire
was therefore rejected.

**Fix (frame index):** snapshot `frameIndex`, `demodFrame`, and
`samplingPoint` on LSF acquire; restore if Hamming fails. After
`noteValidTag()`, `tryAcquireLsf` is not entered.

**Fix (clock):** coast `COAST_MISS_UNLOCK=4`. After LSF, hold the
acquire sampling point for `CLOCK_HOLD_FRAMES=3`, then apply only
sync-gated, majority-agreed `±1` TED steps (`CLOCK_AGREE_FRAMES=3`)
with a one-frame hold after each apply (`setClockTracking(true)`).
`setClockTracking(false)` freezes SP for diagnostics. This replaces
the permanent freeze while protecting the DC-block path.

`test_clock_lsf_tracking` / `test_coast_and_sp_protect` require
302-frame streams at 12000 unauth/auth and 12500 auth (pre-fix
stall was 162/302).

### Long-transmission clock tables (real demodulator)

Rendered once per `nvoice`, then impaired. Delivery = `got/(nvoice+2)`.
Acceptance: tracking delivers >= 99 % of frames up to 20 ppm.

**Frozen (`tracking=0`), nvoice=3000:**

| noise | ppm | got | ok | FER | deliv |
|------:|----:|----:|---:|----:|------:|
| 0 | 0 | 3002 | 3000 | 0 | 1.000 |
| 0 | 2 | 785 | 778 | 0.741 | 0.262 |
| 0 | 20 | 82 | 75 | 0.975 | 0.027 |
| 8000 | 0 | 3002 | 2911 | 0.030 | 1.000 |
| 8000 | 20 | 100 | 30 | 0.990 | 0.033 |

**Tracking (`tracking=1`), nvoice=3000:** every ppm in
{0,2,5,10,20,50} at noise 0 and 8000 delivered **3002/3002**
(deliv=1.000). Clean FER=0. Noise-8000 FER rises with ppm (0.030 at
0 ppm to 0.462 at 50 ppm) but frames are still delivered.

**Frozen, nvoice=28000:** same cliff as 3000 (deliv ~0.028 at 2 ppm).

**Tracking, nvoice=28000:** deliv=1.000 through 20 ppm at both noises;
at 50 ppm + noise 8000, deliv=0.964 (below 99 %, outside the 20 ppm
gate). Clean FER=0 through 50 ppm.

### Impairment baselines after tracking

Unchanged vs freeze: `first_noise_fail=13000`, `first_ppm_fail=650`,
gain/DC/false-lock paths pass. Short 3-frame ppm sweep still fails
near 650 ppm before many agree/hold cycles; long streams are what
tracking fixes.

C20 multi-seed intact at sigma=10000 (200 trials each): 118, 124,
120, 123, 121 (mean 121.2). Prior 118 vs 121 is run-to-run seed
variation inside that range.

## 8c. Authenticated lock and coasting (firmware)

`COAST_MISS_UNLOCK = 4` in `HorseConstants.hpp`. Idle zeros after the
last voice, no EOT, real demod:

| N | extra frames | time to release |
|--:|-------------:|----------------:|
| 2 | 3 | 120 ms |
| 4 | 5 | 200 ms |
| 8 | 9 | 360 ms |

N=4: two missed syncs still hold the grid; a missed EOT frees the
channel in 200 ms. EOT Hamming-2 still ends at once.

## 8d. Why polar lost lock at noise 11000 (old tables)

Those 159-frame cliffs were unauthenticated `tryAcquireLsf` plus the
`frameIndex` clobber, not polar versus M17. After auth-on-LSF and
the snapshot, polar and M17 both hold ~10000 frames through 12500.

Version 1, 200 trials, seed 1, LSF+6 sig+300 voice+EOT, types only:

| noise | ok/200 |
|------:|-------:|
| 2000 | 200 |
| 5000 | 200 |
| 10000 | 178 |
| 12500 | 101 |
| 15000 | 12 |

C20 3-frame payload-checked rates (audit): 200, 200, 121, 57, 11.

Version 2 complete TX (harness only): option C voice, 3-chunk M17 LSF
with `crc_m17`, 6 signature frames, 300 voice, EOT. 200 trials per
point, analog rendered once per (mode, noise), impair seed
`1 + t*17`. Success = LSF CRC plus all 300 voice info bytes. Ideal
slice with miss_limit N. Modes: encrypt (random payload), sign
(0x11-pattern), both (random). Coast N=2/8 matched N=4 at every
measured point because failures were LSF CRC, not tracking.

| noise | encrypt ok/200 | sign ok/200 | both ok/200 | voice_ok encrypt / 60000 |
|------:|---------------:|------------:|------------:|-------------------------:|
| 5000 | 200 | 200 | 200 | 60000 |
| 8000 | 200 | 200 | 200 | 60000 |
| 10000 | 189 | 189 | 189 | 59988 |
| 11000 | 132 | 159 | 132 | 59619 |
| 12500 | 18 | 15 | 18 | 54762 |
| 15000 | 0 | 0 | 0 | 29745 |

The 198/200 gate at noise 10000 is **not** met (189/200); the 11
failures are coded-LSF CRC, not the 300 voice frames. Firmware was
not changed.

## 9. Complete transmissions and LSF comparison

Version 1 type-only (round 2, ideal/demod mix): 178/200 at 10000,
101/200 at 12500. C20 3-frame payload-checked: 200, 200, 118, 56, 10.

Version 2 complete on the **real demodulator**, M17 3-chunk LSF plus
M17 voice, 200 trials, miss_limit 4, seed `1 + t*17`:

| noise | encrypt ok/200 | sign ok/200 | both ok/200 |
|------:|---------------:|------------:|------------:|
| 10000 | 144 | 152 | 144 |
| 11000 | 97 | 104 | 97 |
| 12500 | 20 | 18 | 20 |

Last round's ideal-slice encrypt 132 vs sign 159 at 11000 was not
crypto. Sign voice is a constant `0x11` pattern with FN in the first
two bytes; encrypt and both use a random 18-byte payload. The LSF
flag byte also differs, so the three M17-coded LSF chunks are
different codewords. Voice_ok at 11000 is 47513 (encrypt) vs 47502
(sign) of 60000, so the TX-success gap is LSF CRC, not the 300
voice frames. The same pattern remains on the real demod (97 vs 104).

The best two **codes** on this round are polar L4 voice (tied with
L8 through 13000 on the demod) and M17 hard. The best two **LSF**
layouts at 3 frames are polar block then M17 3-chunk. Complete TX
of polar-voice plus polar-LSF was not run (unverified).

### 9a. LSF at equal airtime (3 frames) and at 4 frames

200 trials, real demod, seed 1. Loss = `1 - ok/200`. RAM is decoder
working set as coded.

3 frames:

| noise | M17 3-chunk loss (us, B) | CRC copies loss (us, B) | polar block loss (us, B) |
|------:|-------------------------:|------------------------:|-------------------------:|
| 8000 | 0.010 (237, 790) | 0.010 (0.7, 46) | 0.010 (4472, 43040) |
| 10000 | 0.110 (249, 790) | 0.135 (1.0, 46) | 0.100 (29323, 43040) |
| 11000 | 0.205 (224, 790) | 0.415 (1.4, 46) | 0.170 (3898, 43040) |
| 12000 | 0.295 (313, 790) | 0.720 (1.5, 46) | 0.240 (5021, 43040) |
| 12500 | 0.355 (254, 790) | 0.860 (1.6, 46) | 0.305 (3361, 43040) |
| 15000 | 0.685 (248, 790) | 1.000 (1.3, 46) | 0.550 (2750, 43040) |

4 frames (M17 repeats chunk 0; CRC sends a fourth copy; polar repeats
the N=512 mother):

| noise | M17 3-chunk | CRC copies | polar block |
|------:|------------:|-----------:|------------:|
| 8000 | 0.010 | 0.010 | 0.010 |
| 10000 | 0.105 | 0.115 | 0.100 |
| 11000 | 0.200 | 0.330 | 0.180 |
| 12000 | 0.285 | 0.635 | 0.240 |
| 12500 | 0.350 | 0.790 | 0.295 |
| 15000 | 0.705 | 1.000 | 0.505 |

Item (d) LDPC: **stopped**. ETSI TS 138 212 V16.2.0 clause 5.3.2
Table 5.3.2-3 (BG2 `V_i,j`) is not a complete verified matrix in
this tree. CCSDS 231.0-B-4 `(512,256)` does not rate-match to a
Horse 368-bit slot. No H was constructed.

Polar LSF is one CA-SCL over 384 info bits plus CRC24C, mother
N=512 (5G Q table, sub-block interleaver), then repetition to 3 or
4 frames. N=1024 SCL in this host decoder did not round-trip; it
was not used.

## 10. Burst errors (1000 frames, hard bits, no analog)

Contiguous corrupted 4-FSK symbols in the 46-byte payload. `intl=1`
applies an extra M17 QPP on top of the codec.

| nsym | intl | repeat-2 | polar L4 | polar L8 | M17 hard |
|-----:|-----:|---------:|---------:|---------:|---------:|
| 4 | 0 | 57 | 1000 | 1000 | 1000 |
| 4 | 1 | 1000 | 1000 | 1000 | 940 |
| 8 | 0 | 3 | 1000 | 1000 | 1000 |
| 8 | 1 | 1000 | 1000 | 1000 | 44 |
| 16 | 0 | 0 | 1000 | 1000 | 1000 |
| 16 | 1 | 1000 | 1000 | 1000 | 0 |
| 32 | 0 | 0 | 620 | 705 | 351 |
| 32 | 1 | 1000 | 549 | 698 | 0 |

Repeat-2 needs an interleaver; it has none today. M17 already
interleaves; a second QPP destroys it. Polar's 5G sub-block
interleaver handled 16-symbol bursts; 32 symbols are past the code.

## 11. Gain at 1 % voice FER

Linear interpolation in noise amplitude. `G = 20 log10(N_x / N_y)`.
**System** (demod misses = errors) versus **code** (ideal timing).
Invalid lock-cliff interpolations from round 2 are replaced.

| | N at 1 % (demod) | N at 1 % (ideal) |
|--|-----------------:|-----------------:|
| repeat-2 | ~7500 (2.15 % at 8000, 37.6 % at 10000) | ~7600 |
| M17 hard | ~11700 (0.44 % at 11000, 1.37 % at 12000) | ~12300 |
| polar L4 | ~12800 (0.35 % at 12500, 0.59 % at 13000) | ~19800 (12.45 % at 20000) |

Polar versus C on the real modem at 12500 is 0.35 % vs 2.08 % FER
(1.73 points) with delivered counts 9965 vs 9964.

## 12. Decoder cost

Host `polar_prof` (`-O2` release build, noiseless), path LLRs reduced
to successive-cancellation size (`int16 llr[2*N]`, 2568 B/path; was
`yst[10][512]` = 10760 B):

| L | us/frame | path_copy/frame | fcomb/frame | sizeof_path |
|--:|---------:|----------------:|------------:|------------:|
| 4 | 554.4 | 1334 | 7074 | 2568 |
| 8 | 1092.9 | 2654 | 13282 | 2568 |

MD-3x0 estimate uses instruction counts, not a host/CPU clock ratio.
`callgrind` on `polar_prof 5` recorded 61.5e6 Ir for 5×L4 + 5×L8;
attributing by host time share gives **~3.74e6 Ir/frame (L4)** and
**~7.37e6 Ir/frame (L8)**. At 168 MHz with IPC 1.0 that is **22.2 ms
(L4)** and **43.9 ms (L8)**; at IPC 0.8, **27.8 ms / 54.8 ms**. Soft
float for path metrics on the M4 is not in the host Ir the same way,
so the real radio would be no faster. **L4 > 15 ms => option C is the
version-2 voice code; polar voice is closed.**

RAM at SC path size: L4 = 4 * 2568 ≈ 10 KiB; L8 ≈ 20 KiB; plus 2 KiB
Q table.

| Decoder | Host -O2 us/frame | MD-3x0 (Ir @168 MHz) |
|---------|------------------:|----------------------|
| polar CA-SCL L=4 | 554 | ~22--28 ms, ~10 KiB |
| polar CA-SCL L=8 | 1093 | ~44--55 ms, ~20 KiB |
| option C hard Viterbi | ~80--90 (-O0 era) | fits 40 ms (in tree) |

## 12b. Soft decisions (not implemented)

`HorseDemodulator` would need, per 4-FSK symbol sample `y` and outer
amplitude `A`: two LLRs, for example `L(b0)` from the sign of `y`
and `L(b1)` from `|y|` versus `A/3` and `A` (inner vs outer). Those
values must be emitted with the frame, not only `{+3,+1,-1,-3}`.
Literature: unquantized soft Viterbi is about **2 dB** better than
hard decisions on AWGN (Proakis, *Digital Communications*, 4th ed.,
Ch. 8). Polar CA-SCL with Gaussian LLRs similarly sits well above
hard-mapped `±8` as used here. No firmware change was made.

## 13. Code size and reuse

- Option C: no new encoder/decoder classes. Reuse
  `ConvolutionalEncoder`, `Viterbi.hpp`, `CodePuncturing.hpp`,
  `Interleaver.hpp`, `Decorrelator.hpp` unchanged. Glue in Horse
  frame pack/unpack only. Estimate +1 KiB of Horse-side glue.
- Option A: Polar sequence table 1024 * 2 B = 2 KiB plus SCL.
  Firmware not written. Estimate several KiB plus the RAM above.
- Option B: BG2 Table 5.3.2-3 is large; CCSDS `W` is 8 KiB of hex.
  Not for MD-3x0 until a sparse decoder exists.

## 14. Undetected errors and the 32-bit tag

Repeat-2: every residual error is an output word. The 32-bit tag is
the only session check (`k_tag` over dir||FN||payload). Random
collision `2^{-32}` per accepted frame if the decoder is ignored.

M17: Viterbi always emits 18 bytes. Undetected = wrong payload that
still matches the tag. Same `2^{-32}` after a wrong decode, plus the
need to forge `k_tag`.

Polar: CRC24C list selection. Wrong path passing CRC24C is about
`2^{-24}` per frame in the usual model; those runs saw zero. The tag
is still applied after decode. Combined false accept ~ `2^{-24}` then
`2^{-32}` if CRC and MAC are independent. **Do not use the tag as the
SCL selection CRC** (forgery would be a 32-bit guess in the decoder
without `k_tag` if someone compared tags inside SCL on the
ciphertext/tag bits).

## 15. Protocol answers

Version 2 on-air (proposed, not implemented):

- New LSF, voice and EOT sync words (16 bits each). Version-1
  receivers acquire only Hamming-0 LSF of `{0x5A,0xA7}` and stay
  silent. Version-2 receivers ignore v1 sync. Same Hamming-0 acquire
  on the new LSF word.
- LSF: three 40 ms frames, M17-coded, CRC16 over src||dst||eph||flags||
  version (version = 2), then split into 18-byte chunks. Signature of
  the 46-byte field image is unchanged except version=2.
- Signature frames: same coding as voice (they already ride the voice
  encoder with FN `0x7000..0x7005`).
- Voice: 144 bits as today, M17-coded into 368, sync = new voice
  word. FN and last-frame bit unchanged (`0..0x6FFF`).
- EOT: new EOT sync + zeros, uncoded (detection is the sync).

Randomiser: yes in signed-only mode. Codec2 2400 can emit long runs;
M17 `decorrelate` is already in option C. Polar 5G coded-bit
interleaver (`I_BIL`) was not enabled (`I_BIL = 0` in this sim).

Version-1 hearing version-2: without new sync words, a v1 radio can
lock on a v2 LSF, read a random version byte, and pass
`horse_crypto_lsf_version_ok` with probability 1/256, then derive
garbage keys and drop on tags. New sync words make both sides stay
silent. Reverse: v2 must not acquire `{0x5A,0xA7}`.

KDF / tag / FN: keep the current IKM
`secret||src||dst||eph||flags||version` (`horse_crypto.c`). Set
version to 2 so v1 keys cannot be reused on a v2 LSF. Tag input
(dir||FN16||12-byte payload) and nonce-from-FN stay. CRC16 on the LSF
is not bound into the KDF beyond the fields already copied; bind the
CRC only as a decode check, not as extra IKM, unless a later crypto
review asks to hash the whole coded LSF.

## 16. Acceptance test (implementation round)

1. Host analog loopback, gain 1.0, `setSkipDcBlock(true)`, at least
   200 complete transmissions per noise (3-chunk LSF + 6 signature + 300
   voice + EOT), fixed seed schedule `1000 + t*17 + (unsigned)noise`
   to match C20. Version 2 must reach **>= 198/200** at noise
   **10000** (99 %). Repeat-2 v1 is 121/200 on the 3-frame test and
   178/200 on the 308-frame type-only test at that point; 198/200 at
   10000 is the coded-LSF gate. The harness v2 curve reached 189/200
   at 10000 (LSF CRC limited).
2. Per-frame decode time: **<= 5 ms** on the host binary used here.
   Polar L=4 already fails (11 ms host). MD-3x0 polar estimates in
   this document are invalid until a reasonable implementation is
   measured.

## 17. Licences (simulation only)

- OpenRTX M17 headers and `ldpc_horse.c`: GPL-3.0-or-later.
- Polar Q sequence: transcribed from ETSI TS 138 212 V16.2.0 Table
  5.3.1.2-1 (specification text, not third-party code).
- CCSDS `W` hex: CCSDS 231.1-O-1 / 231.0-B-4 tables (specification
  text). No third-party decoder library was linked.

## 18. Estimated or unverified

- Soft-float cost of polar path metrics on MD-3x0 (Ir estimate is a
  lower bound).
- Analog soft LLRs (demod has none).
- 5G NR BG2 FER and CCSDS FER (tables listed in section 4; not in tree).
- Polar L=8/16 used 2000/1000 frames, not 10000, except L=4.
- 1 % FER noise is interpolated; no point was tuned.
- `m17_soft` is sliced SoftViterbi, not analog soft.
- CRC24C bit-order matches 5.1 systematic form as implemented in the
  sim; a second independent encoder was not compared.
- Late entry on voice sync (no opening LSF) is implemented in firmware
  and covered by loopback three-mode / DROP_LSF tests; FEC-sim
  `lsf_frag erase_open=1` covers erased opening payload with sync kept.
- Complete TX of polar-voice plus polar-LSF was not run.
- `perf` hardware counters were unavailable (`perf_event_paranoid=4`).

## 19. Reproduce the tables

Fixed seeds: voice FER and polar timing use seed `1` (payload RNG and
`impair_48k`). C20 3-frame rates use `1000 + t*17 + (unsigned)sigma`.
Complete-TX type-only v1 used seed `1` plus `t*17` per trial.

Build and noiseless check:

```
meson compile -C build_linux horse_fec_v2_sim
./build_linux/horse_fec_v2_sim selftest
```

Voice FER through `HorseDemodulator` (id 0=repeat-2, 1=polar L4,
4=M17 hard, 5=M17 sliced-soft), 10000 frames, seed 1:

```
./build_linux/horse_fec_v2_sim voice <id> <noise> 10000 1
```

Noises in the table: 0 4000 6000 8000 10000 11000 12000 12500 14000
15000 16000 18000. Polar L8: id 2, 2000 frames. Polar L16: id 3, 1000
frames at 10000 12500 15000.

Burst (1000 frames, no analog):

```
./build_linux/horse_fec_v2_sim burst <nsym> <intl> 1000
```

nsym in 4 8 16 32, intl 0 or 1.

v1 complete type-only:

```
./build_linux/horse_fec_v2_sim complete_v1 <noise> 200 1
```

v2 complete (mode 1=encrypt 2=sign 3=both; miss_limit 2,4,8):

```
./build_linux/horse_fec_v2_sim complete_v2 <noise> 200 1 <miss> <mode>
```

Ideal vs demod plus coast N=2,4,8 (id 0/1/4):

```
./build_linux/horse_fec_v2_sim study <id> <noise> 10000 1
```

Lock characterisation, EOT delay, polar profile, symbol histograms:

```
./build_linux/horse_fec_v2_sim lockstat <noise> 2000 1
./build_linux/horse_fec_v2_sim eot_notice <noise> <miss_limit> 20 1
./build_linux/horse_fec_v2_sim polar_prof 50
./build_linux/horse_fec_v2_sim hist 10000 1
```

Logged output: `tests/unit/horse_fec_v2_results.txt` (first campaign),
`tests/unit/horse_fec_v2_round2.txt` (old lock-limited tables),
`tests/unit/horse_fec_v2_round3.txt` (authenticated demod, polar SCL
rewrite, LSF a--c, complete on real demod).

```
./build_linux/horse_fec_v2_sim lsf_cmp <kind 0-3> <frames> <noise> 200 1
./build_linux/horse_fec_v2_sim clktrace <noise> 400 1 <auth>
```

## 20. Tests and commits

Native meson test 25/25. Address-sanitizer Horse/M17 subset 12/12
including loopback and FEC selftest. Firmware tracking is protocol
version 1 (no on-air format change). Draft PR stays draft. `horse.md`
was not edited. The owner must sign off commits; this round's
demodulator and study files are uncommitted until asked.

Pushed earlier on `upstream-sync`: `3b79a791` (FEC host sim),
`77ccf6de` (ideal timing / lock study).
