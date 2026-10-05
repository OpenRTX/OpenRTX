# Horse FEC version 2 study

Date: 2026-10-05
Branch: `upstream-sync`
Status: proposal only. Firmware on-air format is unchanged. `horse.md`
still describes version 1 as built.

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

## Recommendation (three sentences)

The document supports option C (M17 convolution, puncture, interleaver,
decorrelator) as the version-2 coding choice for Horse on this radio.
LDPC (5G NR BG2 and CCSDS) was not evaluated: Table 5.3.2-3 was not a
usable matrix, the dense CCSDS min-sum never finished a 10000-frame
run, and `HorseDemodulator` still has no analog soft outputs, so this
study does not rank LDPC or polar against C. Polar CA-SCL list 4 is a
stronger code on known-timing hard bits (0 % FER through noise 12500
in this harness) but the host list decoder is about 11 ms/frame for
the reasons in section 12, MD-3x0 polar time and RAM figures in this
file are invalid until a reasonable implementation is measured, and
system FER on `HorseDemodulator` is lock-limited rather than
code-limited at the 1 % cliff.

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
trials, seed `1000 + t*17 + sigma`): 200/200 at 2000 and 5000, 121/200
at 10000, 57/200 at 12500, 11/200 at 15000. This study also ran a
different complete-TX metric (LSF + 6 signature frames + 300 voice +
EOT, seed 1, type match only, 200 trials): 200/200 at 2000 and 5000,
178/200 at 10000, 101/200 at 12500, 12/200 at 15000. The two tables
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

Recommended LSF: option C, **three** frames, CRC16 over the 46-byte
field image then 6 pad bytes (two 18-byte slots hold only 36 bytes,
so the earlier two-frame sketch cannot carry 46+CRC16). Receivers
that miss any chunk drop the session. Optional late entry: send the
same triple again after signature frames and every 150 voice frames
(6 s). Not required for v2. The complete-TX curve in section 9 used
this 3-chunk layout in the host harness only.

## 8. Voice FER: `HorseDemodulator` vs ideal frame timing

FER = `1 - ok/n` with undelivered frames counted as errors (`n = 10000`).
`got` is how many voice frames were decoded. Seed 1, gain 1.0.

**Real modem:** `HorseDemodulator` (clock recovery, Hamming-2 tracking,
`missedSyncs > 4` or EOT unlock). Same numbers as section 8 of the first
campaign for these three codecs.

| noise | repeat-2 FER (got) | polar L4 FER (got) | M17 hard FER (got) |
|------:|-------------------:|-------------------:|-------------------:|
| 0 | 0 (10000) | 0 (10000) | 0 (10000) |
| 6000 | 0 (10000) | 0 (10000) | 0 (10000) |
| 8000 | 0.0518 (10000) | 0 (10000) | 0 (10000) |
| 10000 | 0.5664 (10000) | 0 (10000) | 0.0001 (10000) |
| 11000 | 0.8275 (10000) | 0.5256 (4744, all correct) | 0.0030 (10000) |
| 12000 | 0.9994 (159) | 0.9841 (159) | 0.9842 (159) |
| 12500 | 0.9997 (159) | 0.9841 (159) | 0.9843 (159) |

**Ideal timing:** same analog capture; the harness slices at the known
RRC group delay (`RX_DELAY_24 + TX_DELAY_48/2` plus two preamble
frames) with a 90th-percentile outer amplitude. The receiver is told
where each frame starts. No correlator, no clock recovery.

| noise | repeat-2 FER (got) | polar L4 FER (got) | M17 hard FER (got) |
|------:|-------------------:|-------------------:|-------------------:|
| 0 | 0 (10000) | 0 (10000) | 0 (10000) |
| 6000 | 0 (10000) | 0 (10000) | 0 (10000) |
| 8000 | 0.0309 (10000) | 0 (10000) | 0 (10000) |
| 10000 | 0.3931 (10000) | 0 (10000) | 0.0001 (10000) |
| 11000 | 0.6458 (10000) | 0 (10000) | 0.0017 (10000) |
| 12000 | 0.8421 (10000) | 0 (10000) | 0.0048 (10000) |
| 12500 | 0.9002 (10000) | 0 (10000) | 0.0078 (10000) |

At noise 12000 the code is still usable (M17 0.48 %, polar 0 %) if
timing is held; `HorseDemodulator` delivers 159 frames and FER is
dominated by lock. Repeat-2 remains code-limited even with ideal
timing.

Log: `tests/unit/horse_fec_v2_round2.txt`. Command:
`./build_linux/horse_fec_v2_sim study <id> <noise> 10000 1` with
id 0/1/4.

## 8b. Why polar lost lock at noise 11000

Same demodulator class, same seed `1`, same impair LCG
(`rng = rng*1103515245+12345`), fresh `HorseDemodulator` each run
(no leftover state). Polar does **not** apply the M17 `decorrelate`
sequence; M17 voice does. Payload 4-FSK histograms on 10000 noiseless
codewords (seed 1):

| codec | +3 | +1 | -1 | -3 |
|-------|---:|---:|---:|---:|
| repeat-2 | 0 | 1119026 | 0 | 720974 |
| polar L4 | 460210 | 459322 | 459816 | 460652 |
| M17 hard | 459320 | 459468 | 461238 | 459974 |

Polar and M17 are both balanced. Repeat-2 never emits +3 or -1
(zeros in the unused 40 bits plus AND-friendly structure).

At noise 11000, polar **code** is not the failure: ideal timing
decoded 10000/10000. `HorseDemodulator` delivered 4744 voice frames,
all correct. The last delivered frame had Hamming distance 7 to the
voice sync, 5 to LSF, **10 to EOT** -- not a Hamming-2 false EOT.
The demod output shows 15 lock starts, last lock length 17, 106
voice-sync misses among delivered frames, then **no more frames**
(sampling point no longer completing 192 symbols). M17 at the same
noise had `miss_ev=1` and held until a true EOT (last frame HD to
EOT = 0 on the EOT burst).

Cause: after a missed voice sync, `lockedState` calls `tryAcquireLsf`
whenever no tag has been verified. A polar payload can correlator-match
LSF and **move `samplingPoint`**. Clock recovery then walks off the
true 40 ms grid. M17's decorrelated stream at this seed did not
trigger that path often enough to drop the 10000-frame lock. The
codes do not share the analog waveform, so this is payload-dependent
demod behaviour, not a polar decoder bug.

## 8c. Lock loss by itself

`lockstat` uses version-1 encoded voice (any payload) plus LSF and
EOT, 2000 frames, seed 1. Firmware rule: unlock on Hamming-2 EOT or
`missedSyncs > 4`. Ideal slice uses the same Hamming rule on known
timing.

| noise | demod delivered | demod lock length | demod end | ideal delivered | ideal end |
|------:|----------------:|------------------:|-----------|----------------:|-----------|
| 0--11000 | 2002 | 2002 | true EOT (HD 0) | 2002 | true EOT |
| 12000 | 164 | 164 | no EOT; last HD_e=7; sampling stops (`drop=none` on the delivered list, 4 Hamming misses) | 2002 | true EOT |
| 12500 | 164 | 164 | same as 12000 | 2002 | true EOT |
| 15000 | 42 | 42 | 5 Hamming misses (`drop=miss`) | 252 | Hamming misses before EOT |

Below the cliff, lock lasts the whole burst and ends on the EOT
sync. At 12000 and above, `HorseDemodulator` stops emitting frames
while the known-phase slicer still sees an EOT: that is **clock
recovery / sampling**, not Hamming-2 EOT. Hamming-2 miss (`missedSyncs
> 4`) appears at 15000 on both paths.

## 8d. Coasting (harness only, not firmware)

Proposal: once LSF acquire (Hamming 0) has succeeded, keep the 40 ms
frame grid for N consecutive non-matches of LSF/voice before dropping
lock. EOT Hamming-2 still ends the burst immediately. Unlock when
`missedSyncs > N` (firmware today is N=4). Simulated on the **ideal
grid** for N=2, 4, 8. `HorseDemodulator` was not patched.

Through noise 11000, voice Hamming almost never misses on the ideal
grid, so N does not change got/FER (same as the ideal table). At 12500
M17 had 21 Hamming misses but still ended on EOT for N=2,4,8.

End of transmission: when the EOT word is recognized, extra frames
after the true EOT index is 0 for N=2,4,8 (noise 0 and 11000,
20-voice burst). If EOT is missed, idle frames fail LSF/voice match
and lock drops after N+1 frames (40 ms each): 120 ms (N=2), 200 ms
(N=4, firmware), 360 ms (N=8). At noise 15000 the 20-frame burst never
acquired (Hamming-0 LSF failed), so N did not apply.

Coasting on the real demodulator would also need to **inhibit
`tryAcquireLsf` while coasting**, or polar-like payloads will steal
the sampling point after the first missed voice sync. That is not
implemented.

## 9. Complete transmissions

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

| | N at 1 % (demod) | N at 1 % (ideal) |
|--|-----------------:|-----------------:|
| repeat-2 | ~6386 | ~7200 (3.09 % at 8000) |
| M17 hard | ~11007 | >12500 (0.78 % at 12500) |
| polar L4 | ~10019 (lock) | >12500 (0 % at 12500) |

This study does not convert those numbers into a ranking of polar
versus C. Demod 1 % for polar is lock, not CRC24C.

## 12. Decoder cost

Host `polar_prof` (50 noiseless L=4 frames): **11302 us/frame**,
1334 full `SclPath` copies, 1795 `encode_n`, 7074 `fcomb`, 1796 leaf
visits. Each info-bit fork copies `u[512]` plus the LLR stack
(`vector<vector<float>>` of sizes 512+256+...+1). That is thousands
of heap allocations and O(N) memcpy per list path, not arithmetic
throughput. Recursive SCL with eager path copy is why the host needs
~11 ms.

MD-3x0 polar time and RAM columns below are **invalid**. They were
host-us scaled by 24 and a ~90 KiB guess. Do not use them until a
decoder that fits CCM/SRAM and avoids per-path heap copies is
measured on the M4.

| Decoder | Host us/frame | vs 40 ms host | MD-3x0 |
|---------|--------------:|---------------|--------|
| repeat-2 | 1.6--5 | yes | not re-measured here |
| M17 hard Viterbi | 79--113 | yes | not re-measured here |
| polar CA-SCL L=4 (this sim) | 10900--16000 | no (11 ms) | **invalid / unmeasured** |
| polar L=8 | 21000--24000 | no | **invalid / unmeasured** |
| polar L=16 | 42000--43000 | no | **invalid / unmeasured** |
| CCSDS min-sum dense H | unfinished | n/a | not evaluated |

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

- All MD-3x0 polar times and RAM (marked invalid).
- Analog soft LLRs (demod has none).
- 5G NR BG2 FER and CCSDS FER (not evaluated).
- Polar L=8/16 used 2000/1000 frames, not 10000, except L=4.
- 1 % FER noise is interpolated; no point was tuned.
- `m17_soft` is sliced SoftViterbi, not analog soft.
- CRC24C bit-order matches 5.1 systematic form as implemented in the
  sim; a second independent encoder was not compared.

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
`tests/unit/horse_fec_v2_round2.txt` (ideal timing, lock, coast, v2
complete). `scripts/horse_fec_v2_run.sh` is the original campaign
(includes a CCSDS 10000-frame step that does not finish; skip id 6).

## 20. Tests and commits

Firmware and on-air format are unchanged. Native meson test 25/25 and
the sanitizer build 25/25. No merge. Draft PR stays draft. `horse.md`
was not edited. The owner must sign off commits before a PR is
considered ready. Do not push until asked.
