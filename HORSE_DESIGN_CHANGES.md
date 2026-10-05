# Horse design changes (proposal only)

Status: **awaiting owner approval**. None of the items below have been
implemented. Step 3 / Step 4 work does not depend on these decisions
except where a temporary bound is noted.

Each item: options, recommendation, effect on the **on-air format** and
on **stored data**.

---

## 1. Voice frame authentication

**Problem:** The 32-bit "MAC" is BLAKE2b keyed with the public string
`"HVOICETAG"`, not the session key, and it does not bind FN or direction.

**Options:**

| Id | Construction | Tag bytes | Voice payload left |
|----|--------------|-----------|--------------------|
| A | Keep 32-bit tag, key with session key, bind `dir \|\| FN \|\| ciphertext` | 4 | 12 B codec (unchanged) |
| B | 64-bit keyed BLAKE2b / truncated Poly1305, same AAD | 8 | 8 B codec (breaks CODEC2 2400 x2) |
| C | XChaCha20-Poly1305 (16 B tag) | 16 | 0 B in-frame; tag in extra frames |
| D | 32-bit tag now (A) with a version bit in LSF to grow later | 4 | 12 B |

The air voice payload is 18 bytes before LDPC (2 FN + 12 codec + 4 tag).
Growing the tag steals codec bits or adds frames.

**Recommendation:** **A** for this experimental mode: keyed 32-bit tag over
`dir (1 byte) \|\| FN (2, without last-frame bit) \|\| ciphertext (12)`.
Reject missing tags. Document that 32 bits is a traffic-analysis /
brute-force budget, not a long-term MAC. Do not ship B/C until a frame
version exists.

**On-air:** Compatible with today's 48-byte frame if AAD is not sent
(only verified). Incompatible with today's **tag value** (receivers with
the old `"HVOICETAG"` MAC will fail closed, which is intended).

**Stored data:** none.

---

## 2. Nonce and frame-number layout

**Problem:** FN is 15 data bits plus last-frame MSB. Signature frames
reuse `0x7000-0x7005`. Nonce is 10 zero bytes + FN. Wrap at ~21.8 min
reuses XChaCha20 nonces under one session key. Voice FN eventually
collides with the signature range.

**Options:**

| Id | FN / nonce | Max TX | New session |
|----|------------|--------|-------------|
| A | 15-bit voice FN; reserve `0x7000-0x7FFF` forever; stop TX before 0x7000; nonce = `session_id[10] \|\| FN` | ~19 min | Every PTT; also on wrap |
| B | Separate 3-bit type nibble; 13-bit counter; 96-bit nonce = `eph_pk[10:22] \|\| counter32` | hours | Every PTT |
| C | 32-bit FN in LSF continuation; voice FN stays 16-bit but nonce uses LSF random + 32-bit counter in state only | length of PTT | Every PTT; never reuse nonce |

**Recommendation:** **A** as a no-layout-break stopgap if we must stay on
the current 16-bit field: never emit voice FN in `0x7000-0x7FFF`, refuse
TX at 0x6FFF, last-frame bit unchanged, nonce =
`session_key-derived 10 bytes \|\| FN16`. Prefer **B** if a one-time
on-air break is acceptable (cleaner type vs counter).

**On-air:** A: same layout; shorter max call; different nonce (old
radios decrypt garbage). B: **breaks** FN meaning.

**Stored data:** none. Session key remains per PTT (new ephemeral).

---

## 3. Signature transport (64 bytes, 12-byte slots)

**Problem:** `6 * 12 = 72`. Chunk 5 reads/writes past 64.

**Options:**

| Id | Chunking | Padding |
|----|----------|---------|
| A | 5 x 12 + 1 x 4; remaining 8 voice bytes zero | Last frame short |
| B | 6 x 11 with 2 bytes unused in the 12-byte field (66 > 64 still awkward) | |
| C | 8-byte slots, 8 frames (`0x7000-0x7007`) | Changes FN count |
| D | Put 64-byte sig in LSF extension / extra LSF | New frame type |

**Recommendation:** **A**. Keep `SIG_FRAME_COUNT = 6` and `SIG_FRAME_BASE`.
Copy `min(12, 64 - 12*i)` bytes; zero-pad the rest of the 12-byte field.
RX copies the same counts into a 64-byte buffer. A Step 3 bound-only
patch implements A as the safe default; changing to C/D needs a bump of
`SIG_FRAME_COUNT` and is **on-air incompatible**.

**On-air:** A is what a careful encoder should have been. Receivers that
memcpy 12 into a 64-byte buffer still overflow until they use the same
lengths.

**Stored data:** none.

---

## 4. Passphrase and keystore on MD-3x0

**Problem:** Argon2id `MEMLIMIT_MODERATE` (~256 MiB) cannot run in 192 KiB
SRAM. PBKDF2 is a missing symbol, not a fallback. `settings_t.horse_passphrase[33]`
is stored in the clear next to other settings.

**Options:**

| Id | KDF | Passphrase storage |
|----|-----|-------------------|
| A | `crypto_pwhash_MEMLIMIT_MIN` + `OPSLIMIT_INTERACTIVE` (measure on F405) | RAM only; UI prompt; never NVM |
| B | Same KDF; store Argon2id **hash verifier** in settings, wrap key from prompt | Settings grow by 16-32 B verifier, not the password |
| C | Hardware unique key + no user passphrase | Breaks current provision tool |

**Recommendation:** **A** for the radio: interactive/min memlimit, same
parameters in `horse_provision.py`. Remove the PBKDF2 `#else` (fail
closed). Stop persisting `horse_passphrase`. Linux emulator can keep an
env/`stdin` prompt.

**On-air:** none.

**Stored data:** **breaks** existing encrypted identity blobs if memlimit
changes (re-provision). Removing `horse_passphrase` from `settings_t`
**changes** `sizeof(settings_t)` again (MDx CRC miss -> defaults). Prefer
a settings version byte or leave the field zeroed unused until a
migration (owner choice). Existing identity flash layout unchanged
except KDF parameters inside the blob.

**MD-3x0:** unverified until cross-built; time `crypto_pwhash` on device.

---

## 5. Contact storage (`contact_t` 39 -> 103 B)

**Problem:** `horseContact_t` (70 B) in the `contact_t` union breaks Linux
`.rtxc` stride. Native TYT CPS cannot store Horse keys anyway.

**Options:**

| Id | Layout |
|----|--------|
| A | Revert `contact_t` to 39 B; `horse_keystore` / file `horse_contacts.bin` keyed by `contact_index` or callsign |
| B | Bump `CPS_VERSION`; versioned reader; keep 103 B in-memory only with conversion |
| C | Store peer keys only in the identity/provision channel, not in CPS |

**Recommendation:** **A**. `horseInfo_t.contact_index` indexes a separate
packed table (`horse_peer_t { address[6], x25519_pk[32], ed25519_pk[32] }`).
Linux: a sidecar file next to `.rtxc`. Radio: SPI flash region distinct
from vendor CPS.

**On-air:** none.

**Stored data:** restore 39 B `contact_t` (**fixes** old `.rtxc`). Horse
peer keys live in a new file/NVM object. Existing fork `.rtxc` written
with 103 B stride would need a one-shot converter.

---

## 6. Audio framing (20 ms DMA vs 40 ms codec)

**Problem:** Double buffer of 320 samples yields 160-sample (20 ms)
blocks. Encode sees short buffers and emits zeros. Decode writes 320
samples into a 160-sample idle half.

**Options:**

| Id | Approach |
|----|----------|
| A | `audioStream_start` with 640 samples (two 40 ms halves) |
| B | Accumulate two 20 ms blocks in `horse_codec` before encode/decode |
| C | Switch Horse to 20 ms CODEC2 frames (one 6-byte frame; wastes air bits) |

**Recommendation:** **B**. It matches the documented `BUF_CIRC_DOUBLE`
contract used by M17/codec2 and does not depend on driver buffer-size
interpretation. A is acceptable if Linux and MD-3x0 both treat the
argument as the **full** ring size.

**On-air:** none if the 12-byte / 40 ms payload stays. C would **change**
payload.

**Stored data:** none.

**MD-3x0:** unverified until the audio driver is exercised on hardware.

---

## 7. Stack (512 B RTX vs LDPC/crypto)

**Problem:** `ldpc_horse_encode/decode` use ~552 B of stack arrays on the
RTX thread (`RTX_THREAD_STKSIZE` 512). Horse codec thread is created
without `CODEC2_THREAD_STKSIZE`.

**Options:**

| Id | Approach |
|----|----------|
| A | Static/global LDPC scratch (mutex if RTX and tests share it); `pthread_attr_setstacksize(..., CODEC2_THREAD_STKSIZE)` for Horse codec |
| B | Raise `RTX_THREAD_STKSIZE` to 2048 for all radios |
| C | Move Horse FEC to the codec thread |

**Recommendation:** **A** plus a modest RTX bump only if Horse still
allocates `contact_t` (103 B) on RTX (see item 5). Do not silently grow
every target's RTX stack if A suffices.

**On-air / stored data:** none.

**MD-3x0:** unverified until linked with the Cortex-M4 toolchain and
checked with `-Wstack-usage` / Map file.

---

## 8. `horse_keytool.py` and keyring hex/JSON

**Problem:** `generate` stores `payload.hex()`; `show`/`provision` do
`json.loads` on that string. `show` prints secret key material.
`horse_keytool.py` exports OpenPGP blobs, not X25519/Ed25519 contacts.

**Options:**

| Id | Tools |
|----|-------|
| A | Fix keyring: store UTF-8 JSON; never print secrets; keep `horse_keytool.py` as optional GPG helper with a documented converter |
| B | Remove kernel keyring; identities as `0600` files under `XDG_STATE_HOME/OpenRTX/` |
| C | Delete `horse_keytool.py`; provision only via `horse_provision.py generate` |

**Recommendation:** **B + C**. Kernel keyring adds a host dependency
(`keyutils`) that is already broken. File-backed identities match the
radio blob. Drop `horse_keytool.py` until someone maps OpenPGP to
`horse_peer_t`.

**On-air:** none.

**Stored data:** host-only. Existing keyring entries would need
`generate` again.

---

## Approval gate

Please mark each item A/B/C (or a variant) before implementation of
sections 1-2, 4-8. Section 3 option A is the Step 3 overflow bound.
