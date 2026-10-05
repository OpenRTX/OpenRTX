# Horse on upstream-sync

Draft PR notes for branch `upstream-sync` against OpenRTX `upstream/master`
(v0.4.5). Do not merge to `master` from this text.

## What changed

Horse is an experimental encrypted 4-FSK voice mode for the TYT MD-3x0
family, built on M17 RRC/correlator/clock recovery with its own frames
and libsodium crypto.

This branch:

- Enables Horse on the Linux emulator and keeps it on MD-3x0 only among
  the radio targets (`CONFIG_HORSE`).
- Frames: LSF, signed-session chunks, voice (CODEC2 stand-in), EOT.
- Crypto: X25519 ECDH, XChaCha20, 32-bit keyed tags, Ed25519 session
  signatures, Argon2id 16 KiB via `crypto_pwhash`, LSF-bound KDF.
  No ECIES on the live path. No cleartext fallback. No `crypto_utils`
  PBKDF2 stub.
- Demod: Hamming-0 LSF acquire at the `convolve()` phase, Hamming-2
  tracking, normalised correlator (Q12) with extra floor 0 (real LSF
  ncc overlaps Hamming-0 noise). An unauthenticated lock is replaced
  by a new LSF after a missed sync; eight frames without a valid tag
  drop the lock.
- C13: C5000 TX key/unkey on every MD-3x0 TX path.
- Tests: analog loopback, three-mode, cps packed layout vs upstream,
  frame/crypto/info/codec/peers/keystore/host interop.
- Horse libFuzzer: `fuzz_horse_frame`, `fuzz_ldpc_horse` (`FUZZING.md`).
- Docs: `horse.md` (root); audit/design/DSP/PR under `docs/horse/`.

`horseInfo_t` (4 B) is added to the existing `channel_t` union. Packed
`sizeof`/`offsetof` of every `cps.h` struct match upstream/master
(`tests/unit/cps_layout.c`). `contact_t` is unchanged (39 B).

Reverted: treating the 46-byte LSF as a repeat-2 codeword. The LSF is
uncoded (C20, open, protocol v2 with C17).

## Verified on host

Native meson procedure (`AGENTS.md`): compile `linux` and the listed
unit-test binaries by name, then `meson test --no-rebuild`. Address
sanitizer: `-Dasan=true`, `ASAN_OPTIONS=detect_leaks=0`, project
`-fno-sanitize=shift`.

Host analog loopback (gain 1.0 unless noted):

| Check | Result |
|-------|--------|
| Clean LSF+voice+EOT | 3/3 |
| Noise | pass 12500, fail 13000 |
| Clock offset | fail 300 ppm |
| Polarity invert / invert flag | 0/3 without, 3/3 with |
| DC 500 | 3/3 (unsanitized); `dsp.cpp:19` is upstream UB |
| Gain 0.25 / 0.5 | 3/3 |
| Gain 2.0 | fails; samples clip |
| Hamming-0 false LSF | ~5.80 decoded LSF/min over 10 min of open-FM noise |
| Intact LSF+voice+EOT / 200 | 200/200 at 2000 and 5000; 121/200 at 10000; 57/200 at 12500; 11/200 at 15000 |
| LSF replace | 1, 4, 7 frames into a false lock |
| Three-mode modem | encrypt, sign, both; negatives |
| CPS layout vs upstream | compile-time `static_assert` |

`dsp.cpp` is not patched (`docs/horse/UPSTREAM_ISSUE_dsp.md`).

## Unverified on MD-3x0

- On-air TX/RX, C5000 keying (`465707c4`), RF, and UI.
- Flash/RAM: `/opt/arm-miosix-eabi` was missing on the last host, so
  no linker map. Script limit only: 848 KiB flash, 64 KiB CCM + 128 KiB
  SRAM.
- RTX 512 B stack and Argon2id 16 KiB heap under libsodium on-device.

## Known limits

- 32-bit tags: casual integrity, not a high-budget attacker.
- Argon2id 16 KiB is weak against offline guessing of a bad passphrase.
- Voice FEC is repeat-2, not LDPC (C17, open).
- LSF has no FEC and no checksum (C20, open). Reception limit.
- No late entry without the LSF.
- False Hamming-0 LSF locks are expected; no audio without a valid tag.
- Linux and MD-3x0 only for `CONFIG_HORSE`.

## How to run the tests

```bash
meson setup build_linux
meson compile -C build_linux linux \
  m17_golay_test m17_viterbi_test m17_callsign_test m17_metatext_test \
  m17_demodulator_test m17_rrc_test cps_test minmea_conversion_test \
  horse_frame_test horse_crypto_test horse_info_test horse_codec_test \
  horse_peers_test horse_keystore_test horse_host_interop_test \
  horse_loopback_test ui_check_standby_test m17_packet_test \
  dsp_oversampling_test gfx_text_test m17_replay_test cps_layout_test
meson test -C build_linux --no-rebuild

meson setup build_linux_address -Dasan=true
meson compile -C build_linux_address linux \
  m17_golay_test m17_viterbi_test m17_callsign_test m17_metatext_test \
  m17_demodulator_test m17_rrc_test cps_test minmea_conversion_test \
  horse_frame_test horse_crypto_test horse_info_test horse_codec_test \
  horse_peers_test horse_keystore_test horse_host_interop_test \
  horse_loopback_test ui_check_standby_test m17_packet_test \
  dsp_oversampling_test gfx_text_test m17_replay_test cps_layout_test
ASAN_OPTIONS=detect_leaks=0 meson test -C build_linux_address --no-rebuild
```

`HORSE_FALSE_LOCK_LONG=1` extends the loopback Hamming-0 noise table
to ten minutes. Do not run `ninja -C <dir>` with no target if that
directory also builds Miosix firmware and the ARM toolchain is missing.
