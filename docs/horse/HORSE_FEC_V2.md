# Horse FEC version 2 study

Date: 2026-10-05
Branch: `upstream-sync`
Status: proposal only. Firmware on-air format is unchanged. `horse.md`
still describes version 1 as built.

Simulation: `tests/unit/horse_fec_v2_sim.cpp`,
`tests/unit/horse_fec_v2_codes.hpp`,
`tests/unit/horse_fec_v2_polar_q.inc`,
``scripts/horse_fec_v2_run.sh`. Results:
`tests/unit/horse_fec_v2_results.txt`. Commands to reproduce are in
section 19. Fixed seed `1` unless noted.
Impairment: the analog loopback of `horse_loopback.cpp` (gain 1.0,
additive uniform-amplitude noise in ADC counts, `HorseModulator` then
`HorseDemodulator` with `setSkipDcBlock(true)`).

## Recommendation (three sentences)

Use option C for both voice and LSF: the M17 rate-1/2 K=5 convolutional
code, `DATA_PUNCTURE` / `LSF_PUNCTURE`, QPP interleaver and
decorrelator already in `openrtx/src/protocols/M17`, with soft-decision
Viterbi once the demodulator exports LLRs. On the real modem path,
repeat-2 hits 1 % voice-frame loss near noise 6400; M17 hard Viterbi
stays at 0.01 % at 10000 and 0.30 % at 11000, about 4.7 dB of
amplitude gain, and fits the 40 ms frame and the MD-3x0 memory
estimate. Polar CA-SCL list 4 is a stronger code when a frame is
demodulated (CRC24C rejected every wrong list in these runs) but it
measured 11 ms/frame on the host, list 16 measured 43 ms, estimated
M4 time is far above 40 ms, and at the 1 % operating point
`HorseDemodulator` lock loss dominates so polar is not better than M17
on the system FER; the extra complexity is not justified for this
radio.

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

Polar voice plus M17 LSF, or the reverse, was not better as a system:
polar list-4 is too expensive for the 40 ms voice period, and M17
already wins the 1 % voice FER including demod misses. Mixed polar-LSF
+ M17-voice would still need ~180 KiB-class SCL scratch (estimate) for
a one-shot LSF decode. Not recommended for MD-3x0.

## 7. LSF designs

Current fields: src 48, dst 48, eph 32*8, flags 8, version 8 = 368
bits. Version 2 adds a checksum. `crc_m17` (poly 0x5935, already in
tree) is 16 bits => 384 information bits.

| Option | Frames | Bit budget | Start delay | One frame lost | Late-entry extension |
|--------|--------|------------|-------------|----------------|----------------------|
| A polar | 3 voice-coded slots (3*144 = 432 >= 384; 48 pad) | 384 info+CRC / 1104 coded | 120 ms | Whole LSF fails unless the lost slot is treated as erasures on a single 3-frame mother code (not simulated) | Repeat the 3-frame LSF every 6 s: +5 % airtime |
| B BG2 | 1 frame if rate-matched 384->368 (too tight); 2 frames E=736 | 384 / 736 | 80 ms | Erasure of 368 of 736 | Same as A |
| C M17 | 2 frames of 18-byte chunks + CRC16, each DATA_PUNCTURE+interleave+decorrelate | 384 / 736 | 80 ms | Whole LSF fails (no reconstruction from one chunk) | Repeat the 2-frame LSF every 6 s: +3.3 % airtime |
| v1 | 1 uncoded | 368 / 368, no CRC | 40 ms | Whole TX lost | None |

Recommended LSF: option C, two frames, CRC16 over the 46-byte field
image before split. Receivers that miss either frame drop the
session (same as today if the single LSF is wrong), but both frames
are coded. Optional late entry: send the same pair again after
signature frames and every 150 voice frames (6 s). Not required for
v2.

## 8. Voice FER (hard decisions, 10000 frames, seed 1)

FER = `1 - ok/n` with misses counted as errors (`n = 10000` or as
noted). `got` is frames `HorseDemodulator` delivered.

| noise | repeat-2 | polar L4 | M17 hard | M17 "soft" | polar L8 (n=2000) |
|------:|---------:|---------:|---------:|-----------:|------------------:|
| 0 | 0 | 0 | 0 | 0 | 0 |
| 4000 | 0 | 0 | 0 | 0 | 0 |
| 6000 | 0 | 0 | 0 | 0 | 0 |
| 8000 | 0.0518 | 0 | 0 | 0 | 0 |
| 10000 | 0.5664 | 0 | 0.0001 | 0.0003 | 0 |
| 11000 | 0.8275 | 0.5256 (got 4744, all correct) | 0.0030 | 0.0029 | 0 (got 2000/2000) |
| 12000 | 0.9994 (got 159) | 0.9841 (got 159) | 0.9842 | 0.9842 | 0.9205 |
| 12500 | 0.9997 | 0.9841 | 0.9843 | 0.9843 | 0.9205 |
| 14000 | 1.0 (got 36) | 0.9964 | 0.9965 | 0.9965 | 0.9820 |
| 15000 | 1.0 | 0.9964 | 0.9967 | 0.9967 | 0.9820 |
| 16000 | 1.0 | 0.9964 | 0.9973 | 0.9972 | 0.9820 |
| 18000 | 1.0 (got 23) | 0.9983 | 0.9991 | 0.9992 | 0.9895 |

Polar L16: 0/1000 at 10000 (43 ms/frame); at 12500/15000 FER follows
`got`, same lock cliff. Repeat-2 `undet` equals the error count
because AND-decode has no CRC and always returns a word. Polar
`undet = 0` in every completed run (CRC24C rejected wrong lists).
M17 `undet` is a wrong 18-byte word; there is no extra CRC on the
voice payload besides the 32-bit tag, which this FER test treated as
data, not as a MAC.

Above ~12000 the demodulator delivers tens of frames; FEC barely
matters. That is the same cliff C20 saw on uncoded LSF, now on voice
tracking.

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

Version 2 complete TX through coded LSF was **not** run (would need
the v2 layout on the air). Independent approximation: 300 M17 voice
frames at FER 0.0001 give `(0.9999)^300 ≈ 0.97` before LSF. Coded
two-frame LSF is required to beat the uncoded LSF limit.

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

## 11. Gain at 1 % voice FER (including demod misses)

Linear interpolation in noise amplitude. `G = 20 log10(N_x / N_y)`.

| | N at 1 % FER | vs repeat-2 | vs M17 |
|--|-------------:|------------:|-------:|
| repeat-2 | ~6386 (between 6000 and 8000) | 0 dB | -- |
| M17 hard | ~11007 (0.30 % at 11000, cliff at 12000) | **+4.7 dB** | 0 dB |
| polar L4 | ~10019 (0 % at 10000, 52.6 % at 11000) | +3.9 dB | **-0.8 dB** |

Polar is worse than M17 at the 1 % **system** point because lock
dropped on the 10000-frame polar stream at 11000 while M17 stayed
locked. Conditional on a delivered frame, polar L4 was error-free
at 11000 (4744/4744). Do not invert that into a polar win on the
demodulated path the product actually uses.

Complete-TX 1 % (198/200) for v1 seed-1 type match interpolates to
about noise 5450, but 3-frame C20 is harsher. Version 2 must be
measured on 200 full sessions after LSF coding exists.

## 12. Decoder cost

Host times from the noiseless 10000-frame runs (seed 1). MD-3x0
figures are **estimates**: scale host microseconds by 24 for 168 MHz
versus a ~4 GHz host; multiply polar floats by 4 for no-FPU
soft-float. RTX stack is 512 bytes; static buffers required.

| Decoder | Host us/frame | Peak RAM (host, order) | MD-3x0 time (est.) | MD-3x0 RAM (est.) | vs 40 ms |
|---------|--------------:|----------------------:|-------------------:|------------------:|----------|
| repeat-2 | 1.6 | 368 B scratch | 0.04 ms | static 46+23 B | yes |
| M17 hard Viterbi | 79--92 | < 1 KiB (`history` 244 * 16) | 2 ms | < 1 KiB static | yes |
| M17 SoftViterbi (sliced) | 100--110 | < 2 KiB | 2.5 ms | < 2 KiB | yes |
| polar CA-SCL L=4 | 10900--11500 | SCL path copies, ~90 KiB class | 1.0 s | ~90 KiB | **no** |
| polar L=8 | 21000--24000 | ~180 KiB class | 2 s | ~180 KiB | **no** |
| polar L=16 | 42000--43000 | ~360 KiB class | 4 s | ~360 KiB | **no** (exceeds host 40 ms) |
| CCSDS min-sum dense H | unfinished at 10000 | `W` 64 KiB + messages | n/a | 64 KiB+ | **unverified** |

MD-3x0 has 64 KiB CCM + 128 KiB SRAM total for the whole firmware.
Polar L=4 scratch near 90 KiB is an estimate and already crowds
CODEC2. Polar L=16 on the host already exceeds the frame period.

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
- LSF: two 40 ms frames, M17-coded, CRC16 over src||dst||eph||flags||
  version (version = 2), then split. Signature of the 46-byte field
  image is unchanged except version=2.
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
   200 complete transmissions per noise (LSF pair + 6 signature + 300
   voice + EOT), fixed seed schedule `1000 + t*17 + (unsigned)noise`
   to match C20. Version 2 must reach **>= 198/200** at noise
   **10000** (99 %). Repeat-2 v1 is 121/200 on the 3-frame test and
   178/200 on the 308-frame type-only test at that point; 198/200 at
   10000 is the coded-LSF gate.
2. Per-frame decode time: **<= 5 ms** on the host binary used here,
   and **<= 15 ms estimated** on 168 MHz Cortex-M4 without double
   FPU, static buffers, not on the 512-byte RTX stack. Polar L=4
   already fails (11 ms host, ~1 s estimated M4).

## 17. Licences (simulation only)

- OpenRTX M17 headers and `ldpc_horse.c`: GPL-3.0-or-later.
- Polar Q sequence: transcribed from ETSI TS 138 212 V16.2.0 Table
  5.3.1.2-1 (specification text, not third-party code).
- CCSDS `W` hex: CCSDS 231.1-O-1 / 231.0-B-4 tables (specification
  text). No third-party decoder library was linked.

## 18. Estimated or unverified

- All MD-3x0 times and RAM.
- Analog soft LLRs (demod has none).
- 5G NR BG2 FER (Table 5.3.2-3 not verified from the PDF dump).
- CCSDS FER and sparse H (dense min-sum not completed; 8th circulant
  column not read from the figure).
- Version 2 complete-TX 200-trial curve (layout not on the air).
- Polar L=8/16 used 2000/1000 frames, not 10000, except L=4.
- 1 % FER noise is interpolated; no point was tuned.
- `m17_soft` is sliced SoftViterbi, not analog soft.
- Polar SCL RAM is an order-of-magnitude estimate from path copies.
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

Logged output: `tests/unit/horse_fec_v2_results.txt`.
`scripts/horse_fec_v2_run.sh` is the original campaign (includes a
CCSDS 10000-frame step that does not finish; skip id 6).

## 20. Tests and commits

Firmware and on-air format are unchanged. Native `meson test --no-rebuild`
and the sanitizer build must pass. No merge. Draft PR stays draft.
`horse.md` was not edited. The owner must sign off commits before a PR
is considered ready.
