## Horse digital voice mode (experimental)

Horse is an experimental encrypted digital voice mode in this fork for the
TYT MD-3x0 family. It reuses M17 DSP (RRC, correlator, clock recovery) and
the M17 convolutional encoder / DATA_PUNCTURE / interleaver / decorrelator
/ Viterbi path for voice and LSF coding. Framing and cryptography are
Horse-specific.

Horse is not a production security product. Without libsodium every crypto
call fails closed. There is no cleartext fallback. Protocol version is **2**;
version 1 has no deployed users and is not accepted on the air.

### Why the name

4-FSK has four symbol levels, so four "legs".

---

## On-air waveform and framing

- Symbol rate: 4800 symbols/s
- Frame: 192 symbols (48 bytes)
- Sync words (v2): LSF `{0x15, 0x57}`, voice `{0x45, 0xFD}`, EOT
  `{0x77, 0x74}` (Hamming distance >= 6 from each other and from retired
  v1 `{0x5A,0xA7}` / `{0x7E,0x9B}` / `{0x3C,0xD8}`)
- 4-FSK LUT: `00=+1`, `01=+3`, `10=-1`, `11=-3`
- TX RRC at 48 kHz (`M17::rrc_48k`); RX RRC at 24 kHz
- Voice after 2-byte sync: 46 coded bytes = M17 DATA_PUNCTURE of 18 info
  bytes (FN16 + 12-byte codec + 4-byte tag) plus 12 spare bytes used for
  the fragment cycle (section below)

### Opening LSF

Three LSF-sync frames. Each carries an 18-byte M17-coded chunk of the
48-byte block `(46-byte LSF || crc_m17)`. The receiver accepts the LSF
only when all three chunks assemble and the CRC matches.

| Offset | Size | Field |
|--------|------|-------|
| 0 | 6 | Source (base-40, M17-compatible) |
| 6 | 6 | Destination |
| 12 | 32 | Ephemeral X25519 public key (always present) |
| 44 | 1 | Flags: `LSF_FLAG_ENCRYPTED=0x01`, `LSF_FLAG_SIGNED=0x02` |
| 45 | 1 | Protocol version (`LSF_PROTOCOL_VERSION` = 2) |
| 46 | 2 | `crc_m17` over the first 46 bytes |

### Fragment cycle (spare bits)

Every voice and signature frame carries a 12-byte fragment in the option-C
spare. Cycle length is always **10** (mode-independent until LSF CRC):

| Slot | Content |
|------|---------|
| 0..3 | bytes of the 48-byte LSF\|\|CRC block |
| 4..9 | 64-byte Ed25519 signature (encrypt-only TX fills zeros; RX ignores if unsigned) |

Slot index: voice `FN % 10`; signature frames
`4 + (FN - 0x7000)`. The receiver bit-majority combines up to three hard
copies per slot (ties as 0) and accepts the LSF only on CRC pass. A small
inner code on the fragments is not used (outer CRC and Ed25519 already
reject bad assemblies). Full LSF+signature recovery under ideal
conditions needs 10 distinct slots (400 ms); with erasures, plan on about
20 voice frames (LSF-only) or 30 (signed).

### Signature frames and EOT

When signing, the 64-byte Ed25519 signature of
`src||dst||eph_pk||flags||version` is also sent as six voice-coded frames
`fn = 0x7000..0x7005`. Ordinary voice FN is `0..0x6FFF`. EOT uses the v2
EOT sync word. RX requires strictly increasing FN from the first accepted
frame; lost frames are gaps, repeats and backward FN are not.

### Acquisition and late entry

- Acquire on LSF or voice sync at Hamming 0; track at Hamming 2.
- Late entry: lock on voice without an opening LSF; rebuild LSF (and
  signature if required) from fragments. No audio until, in order: LSF
  CRC-ok, session keys derived, at least one voice tag verified under
  `k_tag`, and (if the channel requires signing) the 64-byte signature
  verified.
- After lock, twelve completed frames without `noteValidTag()` drop the
  lock (OpMode notes a tag when LSF CRC passes and when a voice tag
  verifies).
- After authentication, every frame slot is offered to the voice
  decoder unless the sync is within Hamming 2 of EOT; the tag decides
  acceptance. Before authentication the Hamming-2 / flywheel-4 rule
  remains.
- Host loopback skips `dsp_dcBlockFilter` except `test_layer_dc_block()`
  because of upstream `dsp.cpp:19` UB. DC-block off-by-one at the
  LSF/voice boundary is mitigated by `CLOCK_HOLD_FRAMES` (see
  `docs/horse/HORSE_FEC_V2.md`).

---

## Encryption, signing, and keying

Every mode (encrypt, sign, both) runs ephemeral X25519 ECDH. Session keys:

- Shared secret = `X25519(local_sk, remote_pk)`
- IKM = secret || src || dst || eph_pk || flags || version
- `k_enc` = BLAKE2b(IKM, key=`HORSE-KENC`)
- `k_tag` = BLAKE2b(IKM, key=`HORSE-KTAG`)

Any change to those LSF fields in transit changes the keys, so every
voice tag fails. Version must be 2.

Voice: XChaCha20 with `k_enc` when the encrypted flag is set. Tag is
keyed BLAKE2b with `k_tag` over `dir || FN16 (no last-frame bit) ||
12-byte payload`. Comparison is `sodium_memcmp`.

Channel `encrypt_en` / `sign_en` must match the LSF flags. Both clear
still means encrypt. If the channel requires signing and the LSF does
not claim it, or the reverse, there is no audio. Same for encryption.

Identities: `horse_identity_keys_t` (Ed25519 32/64, X25519 32/32), wrapped
with Argon2id (`opslimit=2`, `memlimit=16384`) plus XChaCha20-Poly1305.
The passphrase must be 8 to 32 bytes inclusive (`HORSE_PASSPHRASE_MIN` /
`HORSE_PASSPHRASE_MAX`). Empty and shorter strings are rejected in
`horse_provision.py` and in firmware unlock/store. 8 bytes is the
chosen keypad-feasible floor, not a high-entropy policy.
Passphrase is RAM-only. Linux files are
`$XDG_STATE_HOME/OpenRTX/horse_identity.bin` mode 0600. Peer public keys
are a sidecar table (`horse_peer_t`, 64 slots), not `contact_t`.
`horse_provision.py` writes both formats.

All libsodium work (X25519, Ed25519, BLAKE2b, XChaCha20, Argon2id) runs
on a dedicated 16 KiB worker thread. RTX posts a job under a mutex,
waits on a condition variable (no busy-wait), and reads the result.
Secrets in the job are wiped on take, cancel, and terminate. TX must
not key the transmitter until key agreement has finished; PTT release
cancels the pending job. Host stack watermarks are in
`docs/horse/HORSE_FEC_V2.md`; Cortex-M4 figures will differ.

Codec: CODEC2 2400, two 20 ms blocks per 40 ms Horse frame.

---

## Security properties and limits

Properties the current code aims to provide, not a formal proof:

- No TX or RX audio without libsodium, an unlocked identity, and the
  peer keys required by the channel mode.
- Voice tags bind direction, FN, and payload to `k_tag`.
- `k_enc` and `k_tag` bind the ECDH secret and the LSF src, dst,
  ephemeral public key, flags, and version (version 2).
- Lost frames are tolerated; FN must increase from the first accepted
  frame, capped at `VOICE_FN_MAX = 0x6FFF`.
- Signed sessions also require a verified 64-byte session signature
  before audio (dedicated frames and/or fragment rebuild).
- Unknown or version-1 LSF bytes produce no audio. v1 sync words are
  not acquired.
- A lock that never produces a valid tag is dropped after twelve frames.

Limits:

- 32-bit tags are small; they reject accidental and casual forgery, not
  a high-budget attacker on a long recording.
- Argon2id 16 KiB is sized for 192 KiB SRAM and is weak against offline
  guessing of a bad passphrase from a flash dump. Unverified on MD-3x0.
- Hamming-0 acquire; `CORR_PEAK_MIN` = 0 (real LSF ncc under noise
  overlaps false locks).
- False LSF/voice locks on open FM are expected at the measured rate.
  They are harmless: no audio without a valid tag and LSF CRC.
- Gain 2.0 fails in the impairment test because the samples clip.
- C13 C5000 TX enable is implemented and untested on hardware.
- MD-3x0 flash, RTX stack, libsodium, Argon2 heap, and C5000 keying:
  see the unverified list in `docs/horse/PR_DESCRIPTION.md`. Host
  decoder BSS is 8872 B; fragment soft accumulator is int16.

Normative FEC, gates, and measured stack: `docs/horse/HORSE_FEC_V2.md`.
Audit: `docs/horse/HORSE_AUDIT.md`.

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
the loopback Hamming-0 false-lock table to ten minutes.

Horse tests: Frame, Crypto, Crypto Worker (stack HWM + PTT cancel),
Info, Codec, Peers, Keystore, Host Interop, Loopback (including late
entry, three-mode, long-clock), CPS layout vs upstream, Provision Pack.
FEC binary: `horse_fec_v2_sim` (floors, owner targets, streamloss).

Sanitizer: `meson setup build_linux_address -Dasan=true` with
`ASAN_OPTIONS=detect_leaks=0`.
