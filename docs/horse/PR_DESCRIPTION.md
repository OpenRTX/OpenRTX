# Horse on upstream-sync

Draft PR notes for branch `upstream-sync` against OpenRTX `upstream/master`
(v0.4.5). Do not merge to `master` from this text. Keep the PR draft.

## What changed

Horse is an experimental encrypted 4-FSK voice mode for the TYT MD-3x0
family, built on M17 RRC/correlator/clock recovery with its own frames
and libsodium crypto. **Protocol version 2** replaces version 1 (no
deployed users; no compatibility path).

This branch:

- Enables Horse on the Linux emulator and keeps it on MD-3x0 only among
  the radio targets (`CONFIG_HORSE`).
- Frames (v2): three M17-coded opening LSF frames with CRC-16; voice and
  signature frames use M17 option-C coding; 12-byte spare fragment cycle
  (LSF + signature); EOT; sync words
  `{0x15,0x57}` / `{0x45,0xFD}` / `{0x77,0x74}`.
- Crypto: X25519 ECDH, XChaCha20, 32-bit keyed tags, Ed25519 session
  signatures, Argon2id 16 KiB via `crypto_pwhash`, LSF-bound KDF
  (version 2). Fail closed. No cleartext fallback.
- Demod: Hamming-0 LSF or voice acquire, Hamming-2 tracking, HD<=4
  flywheel after a voice frame, tag-decided voice after authentication,
  coast, bounded TED after `CLOCK_HOLD_FRAMES`, late entry via fragments.
- Crypto worker: 16 KiB thread; RTX never runs libsodium; TX not keyed
  until key agreement finishes; PTT cancel. TX encrypt failure sends
  EOT, unkeys, and shows `Horse: TX crypto` (not zeros).
- MD-3x0 links pinned libsodium 1.0.20 (sha256
  `ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19`)
  via `scripts/build_libsodium_cm4.sh`. If that prebuild fails (no Miosix
toolchain), `meson setup` for `build_cm4` still succeeds and MD-3x0
Horse stays fail-closed without `HAVE_LIBSODIUM`. `randombytes` is the
STM32F405 HASH_RNG. Linux still uses the host package.
- HASH_RNG: seed/clock flags every read, discard first word after
  enable, reject consecutive identical words, mutex for the crypto
  worker. Failure is `HORSE_ERR_RNG` (`Horse: RNG`); no TX. Host mock:
  `horse_randombytes_test`.
- Hardware steps: `docs/horse/HARDWARE_TEST_PLAN.md`.
- Soft Viterbi on the host path; fragment accumulator `int16_t`.
- Tests: analog loopback, three-mode + late entry, long-clock, cps
  layout vs upstream, frame/crypto/worker/info/codec/peers/keystore/host
  interop, RNG mock, provision selftest (ignores extra meson args),
  settings.h vs `upstream/master` (skip if that ref is missing),
  `horse_fec_v2_sim` floors and owner targets. Crypto tests skip
  (meson 77) without libsodium. Keystore/peers use `/tmp` mkdtemp.
- Horse libFuzzer: `fuzz_horse_frame`, `fuzz_horse_voice` (`FUZZING.md`).
- Docs: `horse.md` (root); audit/design/DSP/PR/FEC under `docs/horse/`.

`horseInfo_t` (4 B) is added to the existing `channel_t` union. Packed
`sizeof`/`offsetof` of every `cps.h` struct match upstream/master.
`contact_t` is unchanged (39 B).

C17/C20 closed: option-C voice (`0de39b89`), coded LSF (`4003bcf2`),
fragments (`d2e4405d`), late entry (`b1ceb6b0`), version/sync
(`dc8c906a`), OpMode (`8191a605`).

## Verified on host

Native meson procedure (`AGENTS.md`): compile `linux` and the listed
unit-test binaries by name, then `meson test --no-rebuild`. Address
sanitizer: `-Dasan=true`, `ASAN_OPTIONS=detect_leaks=0`.

Host analog loopback (gain 1.0 unless noted), after v2:

| Check | Result |
|-------|--------|
| Clean LSF+voice+EOT | 3/3 (5 frames with 3-frame opening) |
| Noise | first_noise_fail=17000 (gate >=13000) |
| Clock offset | first_ppm_fail=450 (gate >=300) |
| Polarity invert / invert flag | 0/3 without, 3/3 with |
| DC 500 | 3/3 (unsanitized); `dsp.cpp:19` is upstream UB |
| Gain 0.25 / 0.5 | 3/3 |
| Gain 2.0 | fails; samples clip |
| Late entry | voice acquire + fragment LSF rebuild |
| Three-mode modem | encrypt, sign, both; late-entry DROP_LSF; negatives |
| CPS layout vs upstream | compile-time `static_assert` |
| complete_v2 @10000 (200/mode, real demod) | 95--107/200 (gate >=198; not met) |
| lsf_frag erase_open=0/1 @10000 | 186/200 / 137/200 (gate >=198; not met) |
| Viterbi host | ~82 us/frame; `voice_decode` stack 2208 B host |
| Soft campaign 200 TX | 8000 strict 192-196 usable 198-200; 10000 strict 156-159 usable 186-191 |
| Owner targets (soft, both seeds) | 8000: 190/195; 10000: 150/180 (meson) |
| Floors (not targets) | 8000 strict 180; 10000 usable 165 |
| Streamloss nloss @10000 soft+track | 33 before tag-decided, 30 after |
| Host sodium stack delta | derive 616, verify 1616, Argon2id 5000; RTX 512 B |

`dsp.cpp` is not patched (`docs/horse/UPSTREAM_ISSUE_dsp.md`).

## Unverified without a radio

One list. None of these is claimed done:

- On-air TX/RX, RF, UI, and C5000 key/unkey on a handset (plan:
  `docs/horse/HARDWARE_TEST_PLAN.md`)
- DWT/ITM cycle counts; PTT-to-key times above are estimates
- Runtime heap high-water vs 16+16+16 KiB on the device
- Identity NVM offset versus the MD-3x0 partition map
- Provisioning FIFO on device

MD-3x0 map with real sodium (gcc 9.2.0-mp3.2): flash 350000 B / 848 KiB
(40.31%); CCM 30800 B / 64 KiB (47.00%); largeram framebuffer 40 KiB;
heap 89600 B. RTX `txState` 128 B; sodium worker deepest 4120 B
(Argon2id). Non-Horse `openrtx_mduv3x0` / `openrtx_gd77` rebuilt with
`-Dgit_version=v0.4.5` against `upstream/master` at `v0.4.5`: flash
images (`objcopy -O binary`) are byte-identical. Object files may
still differ in DWARF compile paths.

Passphrase minimum length is 8 bytes (`HORSE_PASSPHRASE_MIN`); empty
and shorter strings are rejected in firmware unlock/store and in
`horse_provision.py`.

## Known limits

- 32-bit tags: casual integrity, not a high-budget attacker.
- Argon2id 16 KiB is weak against offline guessing of a bad passphrase.
- False Hamming-0 locks are expected; no audio without LSF CRC and tag.
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
          dsp_oversampling_test gfx_text_test m17_replay_test cps_layout_test \
          horse_fec_v2_sim horse_crypto_worker_test horse_tx_fail_test \
          horse_randombytes_test
meson test -C build_linux --no-rebuild

meson setup build_linux_address -Dasan=true
# same compile list, then:
ASAN_OPTIONS=detect_leaks=0 meson test -C build_linux_address --no-rebuild
```

The GitHub unit-test job also passes `--test-args '--reporter junit'`.
Horse provision and settings wrappers ignore unknown extra arguments.
`horse_randombytes_test` is built with `b_coverage=false` so gcovr does
not merge two copies of `horse_randombytes.c`.

`HORSE_FALSE_LOCK_LONG=1` extends the loopback Hamming-0 noise table
to ten minutes. Do not run `ninja -C <dir>` with no target if that
directory also builds Miosix firmware and the ARM toolchain is missing.
