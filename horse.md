## Horse digital voice mode (experimental)

Horse is an experimental encrypted digital voice mode in this fork for the
TYT MD-3x0 family. It reuses M17 DSP (RRC, correlator, clock recovery) but
defines its own framing and cryptography.

Horse is not a production security product. Without libsodium every crypto
call fails closed. There is no cleartext fallback.

### Why the name

4-FSK has four symbol levels, so four "legs".

---

## On-air waveform and framing

- Symbol rate: 4800 symbols/s
- Frame: 192 symbols (48 bytes)
- Sync words: LSF `{0x5A, 0xA7}`, voice `{0x7E, 0x9B}`, EOT `{0x3C, 0xD8}`
- 4-FSK LUT: `00=+1`, `01=+3`, `10=-1`, `11=-3`
- TX RRC at 48 kHz (`M17::rrc_48k`); RX RRC at 24 kHz
- Voice payload after 2-byte sync: 16-bit FN (MSB = last frame), 12-byte
  codec, 4-byte tag. FEC is repeat-2 in `ldpc_horse.c`, not an LDPC code.

### LSF (46 bytes after sync)

| Offset | Size | Field |
|--------|------|-------|
| 0 | 6 | Source (base-40, M17-compatible) |
| 6 | 6 | Destination |
| 12 | 32 | Ephemeral X25519 public key (always present) |
| 44 | 1 | Flags: `LSF_FLAG_ENCRYPTED=0x01`, `LSF_FLAG_SIGNED=0x02` |
| 45 | 1 | Protocol version (`LSF_PROTOCOL_VERSION` / `HORSE_LSF_VERSION` = 1) |

### Signature frames

When `sign_en` is set, the 64-byte Ed25519 signature of
`src||dst||eph_pk||flags||version` (46 bytes) is sent as six voice frames
`fn = 0x7000..0x7005`: five 12-byte chunks and one 4-byte chunk. Ordinary
voice FN is `0..0x6FFF`. RX requires strictly increasing FN; lost frames
are allowed as a gap, repeats and backward FN are not.

### Acquisition

LSF-only acquire, Hamming distance 0 (`HAMMING_ACQUIRE_MAX`). Tracking
allows Hamming 2. No late entry on voice or EOT. After lock, eight
completed frames without a valid voice tag drop the lock so a real LSF
can be acquired. Host loopback skips `dsp_dcBlockFilter` except
`test_layer_dc_block()` because of upstream `dsp.cpp:19` UB.

---

## Encryption, signing, and keying

Every mode (encrypt, sign, both) runs ephemeral X25519 ECDH. Session keys:

- Shared secret = `X25519(local_sk, remote_pk)`
- IKM = secret || src || dst || eph_pk || flags || version
- `k_enc` = BLAKE2b(IKM, key=`HORSE-KENC`)
- `k_tag` = BLAKE2b(IKM, key=`HORSE-KTAG`)

Any change to those LSF fields in transit changes the keys, so every
voice tag fails.

Voice: XChaCha20 with `k_enc` when the encrypted flag is set. Tag is
keyed BLAKE2b with `k_tag` over `dir || FN16 (no last-frame bit) ||
12-byte payload`. Comparison is `sodium_memcmp`.

Channel `encrypt_en` / `sign_en` must match the LSF flags. Both clear
still means encrypt. If the channel requires signing and the LSF does
not claim it, or the reverse, there is no audio. Same for encryption.

Identities: `horse_identity_keys_t` (Ed25519 32/64, X25519 32/32), wrapped
with Argon2id (`opslimit=2`, `memlimit=16384`) plus XChaCha20-Poly1305.
Passphrase is RAM-only. Linux files are
`$XDG_STATE_HOME/OpenRTX/horse_identity.bin` mode 0600. Peer public keys
are a sidecar table (`horse_peer_t`, 64 slots), not `contact_t`.
`horse_provision.py` writes both formats.

Codec: CODEC2 2400, two 20 ms blocks per 40 ms Horse frame.

---

## Security properties and limits

Properties the current code aims to provide, not a formal proof:

- No TX or RX audio without libsodium, an unlocked identity, and the
  peer keys required by the channel mode.
- Voice tags bind direction, FN, and payload to `k_tag`.
- `k_enc` and `k_tag` bind the ECDH secret and the LSF src, dst,
  ephemeral public key, flags, and version.
- Lost frames are tolerated; FN must increase.
- Signed sessions also require a verified 64-byte session signature
  before audio.
- A lock that never produces a valid tag is dropped after eight frames.

Limits:

- 32-bit tags are small; they reject accidental and casual forgery, not
  a high-budget attacker on a long recording.
- Argon2id 16 KiB is sized for 192 KiB SRAM and is weak against offline
  guessing of a bad passphrase from a flash dump. Unverified on MD-3x0.
- Repeat-2 FEC is not an LDPC code.
- No late entry: miss the LSF and the rest of the call is silent.
- Hamming-0 acquire uses a normalised correlator floor
  (`CORR_PEAK_MIN` Q12, 4076 ~= 0.995). Absolute 180000 blocked
  gain 0.25 and 0.5 (0/3). After the change, gain 0.25 and 0.5 are
  3/3; gain 2.0 clips (0/3). Ten minutes of open-FM Hamming-0 noise:
  false ncc max 4056 at gains 0.25-1.0, 4073 at 2.0; real LSF is 4096
  at 0.25-1.0 and 4067 at 2.0. 4076 sits between 4056 and 4096.
  `HORSE_FALSE_LOCK_LONG` repeats the 10-minute table.
- Until the first tagged frame, a new LSF replaces the current lock.
- C13 C5000 TX enable is implemented (`465707c4`) and untested on
  hardware.
- MD-3x0 flash, RTX stack, libsodium, and Argon2 heap: `/opt/arm-miosix-eabi`
  is not present on the host that last tried the cross build. Limiter
  from the linker script only: 848 KiB flash, 64 KiB CCM + 128 KiB SRAM.
  Argon2id 16 KiB is `crypto_pwhash` memlimit (heap inside libsodium).

---

## Codeplug

`horseInfo_t` on a Horse channel: `rxCan`/`txCan`, `encrypt_en`,
`sign_en`, `contact_index` into the peer sidecar. `contact_t` is the
upstream 39-byte layout.

---

## Tests

```bash
# Compile Linux and unit-test binaries by name, then:
meson test -C build_linux --no-rebuild
```

See `AGENTS.md` for the target list. `HORSE_FALSE_LOCK_LONG=1` extends
the loopback false-lock NCC table to ten minutes per gain.

Horse tests: Frame, Crypto, Info, Codec, Peers, Keystore, Host Interop,
Loopback (layers, DC-block, false-lock table, per-gain NCC, LSF replace
at 1/4/7 frames, three-mode modem plus negatives), Provision Pack,
settings.h vs upstream.

Sanitizer: `meson setup build_asan -Db_sanitize=address,undefined` with
`-fno-sanitize=shift` and `ASAN_OPTIONS=detect_leaks=0`.
