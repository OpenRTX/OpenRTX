# Horse code audit

Date: 2026-10-05
Branch: `upstream-sync` (after merge of OpenRTX `v0.4.5`)
Spec: `horse.md` (the implementation claim)

This document is a read-of-the-path audit. Nothing in this file is a
security guarantee. Findings marked **confirmed** were traced end to end.
Fuzzing was not run (cancelled by the owner).

Linux Horse enablement is commit `7a2d775f`. This document is also the
work list. Finding status:

| Id | Status |
|----|--------|
| C1 voice MAC not session-keyed | fixed in `4e789cbf` / `16a5b677` (k_tag from ECDH) |
| C2 TX cleartext fallback | fixed in `c6bdad06` (refuse TX; UI `horseError`) |
| C3 demod correlator not Horse sync | fixed in `c6337821` / `4d301939` |
| C4 signature chunk overflow | fixed in `c3f68754`; packing `13ac0cef` (5x12+4) |
| C5 20 ms DMA vs 40 ms codec | fixed in `8b965e29` |
| C6 FN / nonce layout | fixed in `08e6c3cb` / `4e789cbf` |
| C7 Argon2 RAM / PBKDF2 | 16 KiB Argon2id heap; remaining largeram heap 89600 B vs 16+16+16 KiB |
| C8 passphrase in `settings_t` | fixed in `4015daa2` (RAM-only) |
| C9 `contact_t` growth | fixed in `1b79bbdb` (sidecar peers) |
| C10 Ed25519 seed padded to 64 | fixed in `80404ce2` |
| C11 keyring hex/JSON | fixed in `80404ce2` / `31495ec7` |
| C12 encrypted RX without session | fixed in `c6bdad06` |
| C13 MD-3x0 C5000 TX enable | implemented `465707c4`; untested on hardware |
| C14 RTX 512 B stack | ARM `txState` 128 B; sodium worker deepest Argon2 4120 B / Ed25519 4048 B |
| C15 keystore race | fixed in `fb86c4ed` |
| C16 `horseInfo_t` garbage on mode switch | fixed in `1ca16357` |
| C17 LDPC name vs repeat-2 | closed in v2: M17 option-C voice `0de39b89`; `ldpc_horse` removed |
| C18 tests vs claims | analog loopback; three-mode; late entry; NCC floor dropped `18a4d420`; LSF replace `34d9d292` |
| C19 `sodium_memzero` | fixed in `c6bdad06` / `fb86c4ed` |
| C20 LSF has no FEC and no checksum | closed in v2: three M17-coded opening frames + CRC-16 `4003bcf2`; fragments `d2e4405d` |
| M17 `dsp.cpp:19` UBSan | upstream; see `UPSTREAM_ISSUE_dsp.md` (do not patch in this fork) |
| `horse_keytool.py` | removed; identities are XDG files `31495ec7` |
| LSF bind / channel flags | `b1f207f5` |
| False-lock Hamming 0 + tag drop | `b9f6331c`; NCC floor removed `18a4d420` |
| Native meson test vs ARM firmware | reverted `a5fc2f08`; procedure in `AGENTS.md` (`--no-rebuild`) |
| Sanitizer Horse timeouts / `-fno-sanitize=shift` | `83e4a806` |
| ECIES dead code | `c18faa08` |
| Python/C identity interop | `3bb7439a` |

---

Linux Horse enablement (Task 2 compile-only) was committed as `7a2d775f`.

---

## Linux compile (previously never built on the host)

`CONFIG_HORSE` and `horse_src` were only on the MD-3x0 target. That
matches the known issue: the emulator did not compile Horse even though
`horse.md` and `README.md` describe emulator encrypt/sign, `/tmp/horse_output.raw`
and the provisioning FIFO.

Enabling Horse for Linux (`linux_def` `CONFIG_HORSE`, `linux_src += horse_src`)
produced these **errors** (gcc 13.3, then remaining after include fixes):

1. `HorseModulator.cpp:9` / `HorseDemodulator.cpp:8-9`:
   `#include "protocols/M17/M17DSP.hpp"` and `M17Utils.hpp` -- those
   headers were renamed upstream to `DSP.hpp` and `Utils.hpp`.
2. `HorseDemodulator.hpp` including `Synchronizer.hpp` without `<cmath>`:
   `std::abs` is not a member of `std` (error originates in
   `Synchronizer.hpp:75`).
3. `HorseDemodulator.cpp:22`:
   `streamSync({{+1, +3, ...}})` is an ambiguous `Synchronizer` constructor
   after the upstream API change (move-from `std::array` vs copy ctor).
4. `OpMode_Horse.cpp:315-316, 438-439`:
   `M17::decode_callsign` / `M17::encode_callsign` no longer exist; the
   API is `M17::Callsign`.
5. `OpMode_Horse.cpp` (rxState, originally 368/379/385/387):
   `break` not within a loop or switch. Ill-formed C++. Confirmed by the
   compiler. Replaced with a `drop_voice` flag so the file compiles; the
   intended skip-voice control flow is unchanged.
6. Link error: `HorseDemodulator::sfNum` / `sfDen` ODR-used as
   `static constexpr` members (C++14) without out-of-line definitions.

**Warnings** on the successful Linux link:

- `horse_keystore.c:89`: `-Wformat-truncation` (`%s/horse_identity.bin`
  into a 256-byte buffer).
- `OpMode_Horse.cpp` constructor: `-Wreorder` (`txSigSent` vs `rxSigChunks`).
- `OpMode_Horse.cpp:121,453`: `-Wmissing-field-initializers` on
  `contact_t contact = {0}`.
- Linker: missing `.note.GNU-stack` on `voicePromptData.S` (pre-existing
  on all Linux targets, not Horse-specific).

After those compile-only fixes, `meson compile -C build openrtx_linux`
succeeds and `OpMode_Horse.cpp` is part of the emulator binary.

---

## Confirmed bugs

### C1. Voice MAC is not keyed by the session key -- critical

- **Where:** `openrtx/src/protocols/horse/horse_crypto.c:191-198` (encrypt),
  `:225-231` (decrypt).
- **What is wrong:** BLAKE2b is keyed with the public string `"HVOICETAG"`,
  not `session_key`, and does not bind the frame number. Anyone can
  recompute a matching 32-bit tag over arbitrary ciphertext. Decrypt
  then runs XChaCha20.
- **Confirmed:** Live TX uses this tag as the only per-frame integrity in
  encrypt and combined modes (`OpMode_Horse::sendTxVoiceFrame`). Signed-only
  uses `horse_crypto_voice_auth_tag`, which does key BLAKE2b with `auth_key`.
- **Proposed fix:** Keyed MAC (or XChaCha20-Poly1305) over `FN || ciphertext`
  with a key derived from `session_key`. Do not treat a 32-bit tag as
  sufficient. Fail if `tag` is NULL.

### C2. TX fails open to cleartext -- critical

- **Where:** `OpMode_Horse.cpp:enable` (unlock return ignored, ~187);
  `txState` (~454-478).
- **What is wrong:** `horse.md` says fail closed without libsodium or a
  locked keystore. Code defaults `want_encrypt = true` when both channel
  flags are 0, then only sets `encryptTx` if crypto, contact X25519 and
  ECDH all succeed. Otherwise LSF flags stay 0 and `sendTxVoiceFrame`
  encodes plaintext with a zero tag.
- **Confirmed:** `horse_crypto_available()` is false without
  `HAVE_LIBSODIUM`. `horse_keystore_unlock` then returns false. `encryptTx`
  stays false; TX still proceeds.
- **Proposed fix:** If the channel requires encrypt or sign (including
  the default-encrypt case), refuse TX (`opStatus = OFF`) when crypto,
  keystore or contact is not ready. Never send voice without matching
  LSF flags.

### C3. Demodulator acquisition sync is not a Horse sync word -- critical

- **Where:** `HorseDemodulator.hpp` in-class `streamSync` initializer
  `{+1, +3, +3, -1, -1, +1, -1, +3}` vs encoder `byteToSymbols` in
  `HorseUtils.hpp` and `HorseConstants.hpp`.
- **What is wrong:** Encoder LUT (`00->+1, 01->+3, 10->-1, 11->-3`, MSB
  dibit first) maps:
  - LSF `5A A7` to `+3,+3,-1,-1,-1,-1,+3,-3`
  - Voice `7E 9B` to `+3,-3,-3,-1,-1,+3,-1,-3`
  - EOT `3C D8` to `+1,-3,-3,+1,-3,+3,-1,+1`
  None match the correlator template. Later Hamming checks never run
  unless lock is entered by chance.
- **Confirmed:** Same LUT is used by the modulator (matches `horse.md`).
  M17 uses a different, matching template in `Demodulator.hpp`.
- **Proposed fix:** Initialise `streamSync` with LSF (and/or voice)
  symbols from `byteToSymbols`. Prefer separate correlators per sync.

### C4. Signature chunk 5 reads/writes past a 64-byte signature -- high
  (ASan: **critical** memory safety)

- **Where:** TX `OpMode_Horse.cpp` `encodeVoiceFrameWithFn(txSessionSig + 60, ...)`;
  RX `memcpy(rxSessionSig + chunk*12, melpe, 12)`; encoder always copies
  12 bytes (`HorseFrameEncoder.cpp:68-69`).
- **What is wrong:** 64-byte Ed25519 signature, six 12-byte slots:
  last slot starts at offset 60 and copies 12 bytes (indices 60-71).
- **Confirmed:** ASan on `Horse Frame Unit Test`:
  `stack-buffer-overflow` READ of size 12 in
  `encodeVoiceFrameWithFn` from `tests/unit/horse_frame.cpp:158`
  (`test_sig_frames_roundtrip`). `signature[64]` overflows at the last
  chunk. The test still passed without ASan.
- **Proposed fix:** Copy `min(12, 64 - chunk*12)` bytes and zero-pad.
  Use a 72-byte assembly buffer or an 8+4 split on frame 5.

### C5. Codec DMA is 20 ms; encode/decode assume 40 ms -- critical

- **Where:** `horse_codec.c:156-176, 197-238`; `melpe_horse.c` encode/decode.
- **What is wrong:** `audioStream_start(..., MELPE_HORSE_SAMPLES_40MS=320,
  BUF_CIRC_DOUBLE)` -- the existing codec path documents that
  `inputStream_getData` returns **half** the buffer (160 samples = 20 ms).
  Encode: short blocks become zero frames. Decode: `getIdleBuffer` is
  160 samples; `melpe_horse_decode` writes 320 `int16_t`s.
- **Confirmed:** Same double-buffer contract as `audio_codec.c`.
- **Proposed fix:** Start the stream with 640 samples, or accumulate two
  20 ms blocks; decode into a 320-sample staging buffer then copy halves.

### C6. Frame-number space collides; XChaCha20 nonce is only those 16 bits -- high

- **Where:** `HorseFrameEncoder.cpp:63-74`; `OpMode_Horse.cpp` `horse_is_sig_frame`;
  `horse_crypto.c:354-362` (`horse_crypto_voice_nonce_from_fn`).
- **What is wrong:** Voice FN is 15 bits (`& 0x7FFF`), wrap after 32768
  frames (~21.8 min at 40 ms). Last frame ORs `0x8000`. Signature frames
  reuse the voice FN field at `0x7000-0x7005`. After ~19 min ordinary
  voice FN enters that range; RX treats it as signature material.
  Nonce is 10 zero bytes plus FN. Wrap under one session key is
  XChaCha20 nonce reuse.
- **Confirmed:** Encoder increment, RX classifier, nonce helper write
  only bytes 10-11. Session key is per-call, so reuse is intra-call.
- **Proposed fix:** Dedicated frame type (or a reserved high range never
  used for voice). 96-bit nonce = `session_id || FN32`. Stop TX before wrap.

### C7. Argon2id moderate (~256 MiB) vs STM32F405 -- high

- **Where:** `horse_crypto.c` `crypto_pwhash`.
- **What is wrong:** Early code used
  `crypto_pwhash_OPSLIMIT_MODERATE` / `MEMLIMIT_MODERATE`. F405 has
  192 KiB RAM. Current memlimit is 16 KiB (`crypto_pwhash` heap).
  Without libsodium every crypto call fails closed; there is no
  PBKDF2 / `crypto_utils` fallback.
- **Confirmed:** `horse_crypto.c` `#else` returns false; no PBKDF2.
- **Proposed fix:** F405-safe Argon2id parameters, same on the provision
  PC. Do not claim PBKDF2 unless implemented and never mixed with
  Argon2id blobs.

### C8. Passphrase stored in plaintext `settings_t` -- high

- **Where:** `openrtx/include/core/settings.h:54`; persisted via
  `nvm_writeSettingsAndVfo` (`state.c`).
- **What is wrong:** `char horse_passphrase[33]` is a normal settings
  field. Shutdown writes the whole struct to NVM. No UI editor; unlock
  and provision read it (`OpMode_Horse::enable`, `horse_provision.c`).
- **Confirmed:** Packed `settings_t` includes the field; NVM write uses
  `sizeof(settings_t)`.
- **Proposed fix:** Do not persist the passphrase. Prompt at unlock,
  keep in RAM, `sodium_memzero` on lock. If a settings field is
  required, store a verifier only.

### C9. `contact_t` / `settings_t` layout change -- high (Linux CPS) / medium (MCU settings)

- **Where:** `cps.h` `horseContact_t` (70 B) grows `contact_t` from 39 B
  to 103 B; `settings.h` grows `settings_t` by 33 B. `CPS_VERSION`
  remains 0.1.
- **What is wrong:** Linux `cps_io_libc.c` uses `sizeof(contact_t)` as
  array stride -- old `.rtxc` files mis-parse. `horseInfo_t` (4 B) does
  **not** grow `channel_t` (union already sized by `m17Info_t` 5 B).
  Native MD-3x0 CPS is vendor records, not `sizeof(channel_t)`; Horse
  fields are not stored in the TYT contact table (`cps_readContact`
  sets DMR). MDx/Module17 settings CRC uses `sizeof(settings_t)`; old
  blocks fail CRC and load defaults (wipe, not silent corruption).
  CS7000 settings are not CRC-gated the same way; trailing bytes can
  be garbage.
- **Confirmed:** Packed struct comments vs native CPS translators vs
  `cps_io_libc.c` stride.
- **Proposed fix:** Bump `CPS_VERSION` with a versioned reader; do not
  persist passphrase in `settings_t`; migrate settings by version.

### C10. Provisioning packs a 32-byte Ed25519 seed into a 64-byte field -- critical (sign path)

- **Where:** `scripts/horse_provision.py:91-92, 116-124`; firmware
  `horse_identity_keys_t.ed25519_sk[64]`; `crypto_sign_detached`
  (`horse_crypto.c`).
- **What is wrong:** PyNaCl `bytes(signing_key)` is the 32-byte seed.
  `struct.pack("...64s...")` null-pads to 64. Libsodium's 64-byte sk is
  `seed || pk`. Signing with `sk[32:64]==0` does not match `ed25519_pk`.
- **Confirmed:** Pack format vs libsodium sk layout; TX
  `horse_crypto_sign(id->ed25519_sk, ...)`.
- **Proposed fix:** Pack `bytes(signing_key) + bytes(verify_key)`.
  Validate sk/pk consistency on store.

### C11. Kernel keyring round-trip is broken; `show` prints secret material -- high / medium

- **Where:** `scripts/horse_provision.py` store/load (`payload.hex()`
  then `json.loads` without `unhexlify`); `show` prints truncated
  secret-key hex.
- **Proposed fix:** Store raw JSON or hex with matching decode. Never
  print `ed25519_sk` / `x25519_sk`.

### C12. Encrypted RX without a session still starts audio -- high

- **Where:** `OpMode_Horse.cpp` rxState LSF + voice.
- **What is wrong:** Session key is derived only if the keystore is
  unlocked. If LSF has `LSF_FLAG_ENCRYPTED` but ECDH fails, `encryptRx`
  stays false and the payload is pushed to the codec (ciphertext as
  "voice"). Combined+locked may drop via the sign branch.
- **Proposed fix:** If LSF encrypted and `!sessionValid`, do not open
  RX audio and do not push frames.

### C13. Horse TX never enables RF TX on MD-3x0 -- high

- **Where:** `OpMode_Horse.cpp` has `radio_enableRx()` only; no
  `radio_enableTx()`. `radio_MD3x0.cpp:254-280`: `C5000.startAnalogTx`
  is FM and M17 only; `OPMODE_HORSE` hits `default` and skips analog TX.
  `radio_setOpmode` does have a Horse case (12.5 kHz).
- **Confirmed:** Grep of `OpMode_Horse.cpp` for `radio_enableTx` is empty.
  M17 calls `radio_enableTx()`.
- **Proposed fix:** Call `radio_enableTx()` in Horse `txState` like M17;
  handle `OPMODE_HORSE` the same as M17 LINE_IN (with the intended BW).

### C14. RTX stack 512 B vs LDPC scratch; Horse codec thread has no 16 kB stack -- critical on MD-3x0

- **Where:** `threads.h:16-17` `RTX_THREAD_STKSIZE 512`;
  `ldpc_horse.c:30-31` `in_bits[184]`, `out_bits[368]` on the RTX
  thread; `horse_codec.c` `pthread_create` without
  `CODEC2_THREAD_STKSIZE`.
- **Confirmed:** Encoder/decoder are called from `txState`/`rxState`.
  `audio_codec.c` sets 16 kB for codec2; Horse does not.
- **Proposed fix:** Static LDPC scratch. Set codec thread stack to
  `CODEC2_THREAD_STKSIZE`. Increase RTX stack if Horse stays there.

### C15. Provision parser and keystore have no mutex; `hdr->len` vs 320-byte buffer -- high / medium

- **Where:** `horse_provision.c`; `horse_keystore.c` static identity;
  `threads.c` poll on the main thread vs RTX `enable`/`txState`.
- **What is wrong:** If `len` exceeds the remaining buffer the parser
  resets at 320 and never completes. `MSG_SEND_IDENTITY` is
  unauthenticated. `store_plaintext` can replace identity while RTX
  holds `horse_keystore_get_identity()`. RTX also reads
  `state.channel.horse.*` and `state.settings.horse_passphrase` without
  `state_mutex`.
- **Proposed fix:** Cap `hdr->len`; mutex around the keystore; copy Horse
  flags into `rtxStatus` under `rtx_mutex`.

### C16. Mode cycle leaves garbage in `horseInfo_t` -- medium

- **Where:** `openrtx/src/ui/default/ui.c` macro 5 only changes
  `channel.mode`.
- **What is wrong:** The tagged union still holds FM/M17 bytes, so
  `encrypt_en` / `contact_index` are leftover bits. Combined with C2
  this can TX cleartext or the wrong contact. Native CPS never fills
  Horse fields.
- **Proposed fix:** On switch to Horse, zero `channel.horse` and require
  a provisioned Horse contact.

### C17. LDPC / MELPe names vs reality -- closed (protocol v2)

- **Was:** `ldpc_horse.c` repeat-2 misnamed as LDPC; fragile voice FEC.
- **Fix:** Option-C M17 convolutional coding via existing
  `ConvolutionalEncoder` / `DATA_PUNCTURE` / interleaver / decorrelator /
  `HardViterbi` (`0de39b89`). `ldpc_horse.*` removed. Codec remains
  CODEC2 2400 (`melpe_horse.c` name kept for path continuity).
- **Normative:** `docs/horse/HORSE_FEC_V2.md` option C / section 7b.

### C20. LSF has no FEC and no checksum -- closed (protocol v2)

- **Was:** single uncoded 46-byte LSF; reception limit on analog
  loopback (121/200 intact at sigma 10000).
- **Fix:** three M17-coded opening frames covering 46 B + `crc_m17`
  (`4003bcf2`); cycle-10 spare fragments with majority combine
  (`d2e4405d`); late entry on voice sync (`b1ceb6b0`); version byte 2
  and new sync words (`dc8c906a`).
- **Normative:** `docs/horse/HORSE_FEC_V2.md` section 7b.

### C18. Tests do not cover the claims -- medium (quality)

Host analog loopback (`tests/unit/horse_loopback.cpp`) now covers:

- Layer a: `byteToSymbols` / `setSymbol` inverses and the horse.md dibit table.
- Layer b: RRC TX 48 kHz / RX 24 kHz at group delay 40 (24 kHz), SER 0.
- Layer c: demod timing recovery of LSF, voice and EOT.
- Layer d: preamble, LSF, six signature frames, 300 voice frames, EOT.
- Firmware DC-block path (`test_layer_dc_block`): passes in the unsanitized
  meson suite. Under UBSan it aborts on the known `dsp.cpp:19` shift.
- **DC settle + SP jump (mitigated):** forcing `samplingPoint += 1` right
  after LSF acquire made voice sync HD=4 with DC on (no voice); HD=1
  with DC skipped. Mitigated by `CLOCK_HOLD_FRAMES=3` then bounded TED
  (`69d97b54`). Not tracked as a separate open C-item.

### Cost figures (host, protocol v2)

| Item | Value |
|------|------:|
| Hard Viterbi decode (m17_hard, noise 0, n=5000) | 82.4 us/frame |
| Hard Viterbi decode (noise 10000, n=2000) | 82.2 us/frame |
| `sizeof(HorseFrameDecoder)` | 454 B (fragCopy 360 B of that) |
| `sizeof(HorseFrameEncoder)` | 116 B |
| `sizeof(HorseDemodulator)` | 416 B |
| `sizeof(M17::HardViterbi)` | 2032 B |
| `voice_decode` stack (`-Wstack-usage`, host g++) | 2208 B |

Gate for decode time is <= 5 ms/frame (section 16); host meets it.
Static RAM added vs pre-fragment decoder is dominated by `fragCopy`
(~360 B) plus small counters. On MD-3x0, `voice_decode` is 16 B
(`-Wstack-usage`); `SoftViterbi::decodePunctured` 88 B; `txState`
128 B on the 512 B RTX stack. libsodium runs on the 16 KiB worker.

### MD-3x0 link with libsodium 1.0.20 (estimates marked)

Seed tarball sha256
`ebb65ef6ca439333c2bb41a0c1990587288da07f6c7fd07cb3a18cc18d30ce19`.
Autotools: `--disable-shared --enable-static --disable-ssp --disable-pie
--disable-asm --disable-blocking-random`, CFLAGS `-Os` Cortex-M4 hard
float, `RANDOMBYTES_CUSTOM_IMPLEMENTATION`. Not `--enable-minimal`
(drops `crypto_stream_xchacha20`). Unused objects dropped by
`--gc-sections`. Randomness: STM32F405 HASH_RNG (PLL48 / PLL_Q=7),
`rng_init`/`rng_get`, registered with `randombytes_set_implementation`
before `sodium_init`. Not a software CSPRNG.

| Region | Before sodium | After sodium |
|--------|--------------:|-------------:|
| flash (848 KiB) | 240896 B (27.74%) | 350000 B (40.31%) |
| CCM smallram (64 KiB) | 29328 B (44.75%) | 30800 B (47.00%) |
| largeram (130560 B) | 40 KiB framebuffer | 40 KiB framebuffer |
| ELF text/data/bss | 239120 / 1776 / 68512 | 347096 / 2904 / 68856 |
| heap (`_end` 0x2000a200 to `_heap_end` 0x20020000) | 89600 B | 89600 B |

Heap 89600 B vs Argon2id 16384 + worker stack 16384 + codec stack 16384
= 49152 B. Remainder ~40 KiB for other mallocs. **Runtime heap HWM
unverified on the radio.**

ARM `-Wstack-usage` / `.su` (bytes, static):

| Path | Stack |
|------|------:|
| `OpMode_Horse::txState` (RTX) | 128 |
| `OpMode_Horse::rxState` | 72 |
| `sendTxVoiceFrame` | 72 |
| `SoftViterbi::decodePunctured` voice | 88 |
| `melpe_horse_encode_20ms` (codec thread) | 664 |
| worker `generate_addresses` (Argon2id) | 4120 |
| worker `ge25519_scalarmult` | 4048 |
| worker `ge25519_double_scalarmult_vartime` (verify) | 2344 |
| worker `crypto_scalarmult_curve25519_ref10` | 440 |
| `sodium_stackzero` | unbounded warning; Horse does not call it |

Timing **estimates** (not DWT traces). 168 MHz. C ref10, `-Os`, no asm.
Published Cortex-M4 C curve25519/ed25519 figures scaled to this clock;
Viterbi from host 82 us scaled by ~3 GHz/168 MHz.

| Op | Estimate |
|----|----------|
| X25519 scalarmult or keypair | ~8e6 cycles, ~48 ms |
| Ed25519 sign | ~6e6 cycles, ~36 ms |
| Ed25519 verify | ~10e6 cycles, ~60 ms |
| Argon2id m=16384 t=2 (unlock, not PTT) | ~15 ms |
| Soft Viterbi / voice frame | ~1.5 ms (host 82 us) |
| PTT to `radio_enableTx`, encrypt | ~96 ms (keypair + derive) |
| PTT to key, signed or both | ~132 ms (keypair + sign + derive) |

TX encrypt failure now encodes EOT, unkeys, `HORSE_ERR_TX_CRYPTO`
(host test `horse_tx_fail_test`).

Non-Horse `openrtx_mduv3x0` / `openrtx_gd77` vs `upstream/master`
v0.4.5: not byte-identical. Flash +64 B / +32 B. Causes: (1)
`GIT_VERSION` (`v0.4.5` vs `v0.4.5-95-g…-dirty`); (2) `rtxStatus_t`
Horse overlay 0x8c to 0xa4 (+24 B BSS) compiled on every radio even
without `CONFIG_HORSE`. No Horse UI strings in those binaries. Sodium
is not linked.

First failing layer before the demod fix was **c**. Commits:

| Commit | Change |
|--------|--------|
| `b425dbf3` | RRC always on host; `setSymbol` inverse; TX RRC reset |
| `2e3bbe98` | `CORR_SYNC_SCALE` 18 for mixed `±1`/`±3` sync words |
| `f9cac93d` | lock at correlator peak, not M17 falling edge |
| `09ff4e7f` | peak-abs slicing so LSF inner symbols stay inner |
| `12dea76f` | correlator window aligned with `convolve()`; loopback first pass; `should_fail` removed |
| `4d301939` | acquire/track LSF, voice and EOT; Hamming 0 lock / 2 track; `readyFrame` after swap |
| `af1704cf` | DC-block loopback variant |

Acquisition uses Hamming 0. Tracking uses Hamming 2 and checks
`readyFrame` after the swap (M17 still inspects the emptied `demodFrame`;
see `UPSTREAM_ISSUE_dsp.md`).

**Baseband regression baselines** (payload must match; not RF sensitivity):

| Impairment | Pass | Fail |
|------------|------|------|
| Uniform noise amplitude vs outer ~21861 | 12500 | 17000 (v2 sync E2=64 + 3-frame opening; gate >=13000); post-v2 remeasure `first_noise_fail=17000` |
| Sample-rate offset | 400 ppm | 450 ppm; post-v2 `first_ppm_fail=450` |
| Polarity invert without `invertPhase` | 0/3 frames | -- |
| Polarity invert with `invertPhase` | 3/3 | -- |
| DC offset 500 | 3/3 | -- |
| Gain 0.25 | 3/3 | -- |
| Drop first TX frame | 3/3 | -- |

Still not covered: `OpMode_Horse` TX/RX on the air, MD-3x0, and a
session-keyed voice MAC (C1).

- `horse_frame.cpp`: LSF/voice/EOT round-trip on bytes.
- `horse_crypto.cpp`: Encrypt round-trip needs sodium. Negative tests
  pass without sodium because decrypt returns false. Sign test can use
  all-zero keys. No check that the MAC is session-keyed (a forged
  `"HVOICETAG"` tag would still pass).
- No unit test of `OpMode_Horse` TX/RX, so `horse.md`'s "exercised in
  OpMode_Horse and unit tests" is false for the opmode.

### C19. Keys not wiped with `sodium_memzero` -- medium

- **Where:** `horse_keystore.c` wrap key left on the stack after unlock;
  `OpMode_Horse.cpp` `memset` on `sessionKey` / `eph_sk` (compiler may
  drop `memset` on secrets).
- **Proposed fix:** `sodium_memzero` on wrap keys, eph_sk, session_key,
  identity copies.

---

## Suspicions (not filed as confirmed bugs)

- `HORSE_IDENTITY_NVM_OFFSET 0x00FE0000` on 16 MB W25Qx: not traced
  against every MD-3x0 partition map.
- FIFO `serial.Serial("/tmp/openrtx_horse_prov.fifo")` vs firmware
  `open(O_RDWR|O_NONBLOCK)`: not executed.
- Host `dependency('libsodium')` applied to **md3x0_opts**: native
  sodium on an ARM link is likely wrong; cross builds may compile
  without `HAVE_LIBSODIUM`.
- Empty passphrase unlock if identity was stored with `""`.
- Fingerprint hashes the entire identity including secrets
  (`horse_crypto.c`); CONFIRM sends only the hash.
- MDx `data[1024]` overlay vs 128 KB sector 11: likely pre-existing;
  passphrase makes blocks larger.
- `horse_identity_store_t` is not packed: Linux vs ARM padding if both
  used for the same blob.
- `horse_keytool.py` dumps OpenPGP blobs; it does not produce
  `horseContact_t` X25519/Ed25519 fields.

---

## ASan / UBSan

Built with `meson setup build_asan -Db_sanitize=address,undefined` and
`meson test -C build_asan`.

| Test | Result |
|------|--------|
| Horse Crypto Unit Test | PASS |
| Horse Frame Unit Test | FAIL -- C4 stack-buffer-overflow |
| 12 other tests | PASS |
| M17 Demodulator Test | FAIL -- UBSan `dsp_dcBlockFilter` left shift of negative value (`dsp.cpp:19`). Upstream DSP, not Horse. |

Fuzz targets were not run.

Later (C18): unsanitized `meson test -C build_linux` is 19/19, including
Horse Loopback. Horse Frame no longer overflows (C4). Address/UBSan of
the DC-block loopback path still hits `dsp.cpp:19`.

---

## What matches `horse.md`

Frame size 192 symbols / 48 bytes, sync **bytes**, LSF 46-byte layout,
flags `0x01`/`0x02`, TX order LSF then six signature frames then voice
then EOT (when `signTx` is set). Symbol LUT on TX. Linux dump path
`/tmp/horse_output.raw`. ECIES wrap APIs exist and are unused on the
live path, as documented.
