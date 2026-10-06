# Horse design changes

Status: **approved** (owner, 2026-10-05). Implementation follows the
decisions below, one commit per numbered item. Item 0 is an addition
found while checking signed-only key agreement.

Original options are kept for history. **Decision** is what is being
built.

---

## 0. Key agreement in every mode (new)

**Finding:** In signed-only TX, `OpMode_Horse` does not send an ephemeral
X25519 public key. The per-frame auth key is
`BLAKE2b(session_signature, key="HORSE-FAUTH")` via
`horse_crypto_derive_frame_auth_key`. The 64-byte Ed25519 signature is
sent in the clear and is constant for a given `src || dst` (and empty
eph). Anyone with a recording and public keys can recompute the same
tag key and forge new voice that RX accepts.

**Decision:**

- TX always generates an ephemeral X25519 key and puts it in the LSF
  (encrypted, signed, and combined).
- Session secret = ECDH(ephemeral, contact X25519). KDF with distinct
  labels yields `k_enc` and `k_tag`.
- Signed message is always `src || dst || eph_pk || flags || version`.
- Frame tags in every mode are keyed with `k_tag`. Remove
  `horse_crypto_derive_frame_auth_key`.
- LSF byte 45 (was reserved) is protocol version = 1. RX rejects unknown
  versions with no audio.

**On-air:** LSF always carries eph_pk; version byte is 1; signed payload
grows by flags+version. Old recordings will not verify.

**Stored data:** none.

---

## 1. Voice frame authentication

**Problem:** The 32-bit "MAC" is BLAKE2b keyed with the public string
`"HVOICETAG"`, not the session key, and it does not bind FN or direction.

**Options:** (A–D as originally proposed.)

**Decision:** **A, amended.** Tag = truncated keyed BLAKE2b with `k_tag`
over `dir || FN` (without the last-frame bit) || payload (ciphertext, or
cleartext in signed-only). Constant-time comparison. Missing or wrong
tag: frame dropped, no audio.

**On-air:** Same 4-byte tag field; incompatible tag value.

**Stored data:** none.

---

## 2. Nonce and frame-number layout

**Problem:** FN is 15 data bits plus last-frame MSB. Signature frames
reuse `0x7000-0x7005`. Nonce is 10 zero bytes + FN. Wrap reuses
XChaCha20 nonces. Voice FN collides with the signature range.

**Options:** (A–C as originally proposed; D–F were LSF-acquisition
additions.)

**Decision:** **A, amended.**

- Voice FN never enters `0x7000-0x7FFF`. At `0x6FFF` TX sends a proper
  EOT, stops, and tells the user. A new PTT starts a new session.
- RX accepts only strictly increasing FN within a session (document
  the gap tolerated for lost frames); repeats and backward FN are
  dropped.
- LSF acquisition: tolerate a small Hamming distance on the sync word.
  Choose the value from measured false-lock rate on noise-only input
  and report the measurement.
- Late entry (joining after a missed LSF) is **not supported**.

**On-air:** Same FN field; shorter max call; RX lock may accept a few
sync-bit errors.

**Stored data:** none.

---

## 3. Signature transport (64 bytes, 12-byte slots)

**Problem:** `6 * 12 = 72`. Chunk 5 reads/writes past 64.

**Options:** (A–D as originally proposed.)

**Decision:** **A** (already bounded). Make it final and tested:
`5 x 12 + 1 x 4`; remaining voice bytes zero.

**On-air:** Last signature frame carries 4 payload bytes.

**Stored data:** none.

---

## 4. Passphrase and keystore on MD-3x0

**Problem:** Argon2id moderate (~256 MiB) cannot run in 192 KiB SRAM.
PBKDF2 is a missing symbol. `settings_t.horse_passphrase[33]` is stored
in the clear.

**Options:** (A–C as originally proposed.)

**Decision:** **A, amended.**

- Remove `horse_passphrase` from `settings_t` entirely. After this,
  `openrtx/include/core/settings.h` must be identical to
  `upstream/master`.
- Passphrase is entered when Horse mode is enabled, held in RAM only,
  and wiped with the derived key on mode exit.
- Argon2id parameters: one shared constant used by firmware and
  `horse_provision.py`, stored in a versioned header of the identity
  blob. Largest memory cost justified for 192 KiB SRAM; mark unverified
  until measured on the device.
- No PBKDF2 path.
- `horse.md` must state that this gives little resistance to offline
  guessing of a weak passphrase from a flash dump.

**On-air:** none.

**Stored data:** new identity blob header; existing blobs need
re-provision. `sizeof(settings_t)` matches upstream (MDx CRC).

**MD-3x0:** Argon2id time/memory unverified without the device.

---

## 5. Contact storage (`contact_t` 39 -> 103 B)

**Problem:** `horseContact_t` (70 B) in the `contact_t` union breaks
Linux `.rtxc` stride.

**Options:** (A–C as originally proposed.)

**Decision:** **A, amended.** `contact_t` returns to its upstream size
and layout. Horse peer keys go in a separate table `(address, x25519_pk,
ed25519_pk)` indexed by `horseInfo_t.contact_index`: sidecar file on
Linux, distinct flash region on the radio. No converter for the old
103-byte layout. Test that an upstream-format codeplug loads unchanged.

**On-air:** none.

**Stored data:** restore 39 B `contact_t`. New peer table. Fork `.rtxc`
with 103 B stride is not migrated.

---

## 6. Audio framing (20 ms DMA vs 40 ms codec)

**Problem:** Double buffer of 320 samples yields 160-sample (20 ms)
blocks. Encode emits zeros. Decode writes 320 samples into a 160-sample
half.

**Options:** (A–C as originally proposed.)

**Decision:** **B.** Accumulate two 20 ms blocks in `horse_codec` before
encode/decode. Test with real 20 ms blocks: encode never emits zero
frames; decode never writes past its buffer.

**On-air:** none.

**Stored data:** none.

**MD-3x0:** unverified until the audio driver is exercised on hardware.

---

## 7. Stack (512 B RTX vs LDPC/crypto)

**Problem:** `ldpc_horse_encode/decode` use large stack arrays on the
RTX thread. Horse codec thread is created without
`CODEC2_THREAD_STKSIZE`.

**Options:** (A–C as originally proposed.)

**Decision:** **A, amended.** FEC and crypto scratch buffers are static
and used only from the RTX thread, without a mutex. Horse codec thread
gets `CODEC2_THREAD_STKSIZE`. Build with `-Wstack-usage` and report the
largest frames in Horse code.

**On-air / stored data:** none.

**MD-3x0:** unverified until linked with the Cortex-M4 toolchain.

---

## 8. `horse_keytool.py` and keyring hex/JSON

**Problem:** Keyring hex/JSON round-trip is broken. `show` prints secret
key material. `horse_keytool.py` exports OpenPGP, not Horse contacts.

**Options:** (A–C as originally proposed.)

**Decision:** **B and C, amended.** Delete `horse_keytool.py` and the
kernel keyring code. Identities are files under
`XDG_STATE_HOME/OpenRTX/`, mode 0600, encrypted with the passphrase in
the same blob format as the radio. No command prints secret key
material. Add a command that exports a contact's public keys in the
peer-table format of item 5.

**On-air:** none.

**Stored data:** host-only. Existing keyring entries are discarded.

---

## After all items

- Loopback through the modem in encrypt, signed, and combined modes with
  the new key agreement, plus negatives: wrong contact key, flipped tag
  bit, replayed frame, reordered frames, mismatched flags, unknown
  version, missing LSF, incomplete signature.
- Rewrite `horse.md`, including **Security properties and limits**.
- Update `HORSE_AUDIT.md` with commit hashes.
