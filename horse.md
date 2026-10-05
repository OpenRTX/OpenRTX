## Horse digital voice mode (experimental)

Horse is an **experimental encrypted digital voice mode** implemented in this fork for the TYT MD‑3x0 family.  
It reuses large parts of the M17 DSP chain but defines its own framing and cryptography.

> Important: Horse is experimental and under active development.  
> Cryptographic implementations use libsodium where available (Linux builds and MD‑3x0 when cross‑compiled with sodium).  
> Without libsodium, crypto operations fail closed. This mode should not be relied upon for production secure communication until fully audited.

### Why is it named "Horse"?

Horse mode uses **4‑FSK (4‑level Frequency Shift Keying)** modulation, so it has "4 legs" — hence the name "Horse".

---

## On‑air waveform and framing

- **Symbol rate:** `4800` symbols/s (`SYMBOL_RATE` in `HorseConstants.hpp`).
- **Frame length:** `192` symbols per frame (`FRAME_SYMBOLS`), i.e. `48` bytes (`FRAME_BYTES`).
- **Sync words:**
  - Link Setup Frame (LSF): `LSF_SYNC_WORD = {0x5A, 0xA7}`
  - Voice frame: `VOICE_SYNC_WORD = {0x7E, 0x9B}`
  - End‑of‑Transmission (EOT): `EOT_SYNC_WORD = {0x3C, 0xD8}`
- **Voice payload structure (per frame):**
  - `16`‑bit frame number (`VOICE_FRAME_COUNTER_BITS`), MSB set on last voice frame
  - `96` bits voice codec data (`VOICE_MELPE_BITS`, 12 bytes on air)
  - `32`‑bit tag field (`VOICE_TAG_BITS`): encryption MAC and/or per‑frame auth tag

Voice frames use a **repeat‑2 placeholder** in `ldpc_horse.c` (each payload bit is
sent twice; decode is bitwise AND of the two copies). This is not an LDPC code.
The 18‑byte clear payload is 2‑byte FN + 12‑byte voice + 4‑byte tag.

### Link Setup Frame (LSF) layout

The 46‑byte LSF payload (after the 2‑byte sync word) is:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 6 | Source address (base‑40, M17‑compatible) |
| 6 | 6 | Destination address |
| 12 | 32 | Ephemeral X25519 public key (encrypt mode only; zero otherwise) |
| 44 | 1 | Flags (`LSF_FLAG_ENCRYPTED = 0x01`, `LSF_FLAG_SIGNED = 0x02`) |
| 45 | 1 | Reserved |

The ephemeral public key is sent in cleartext. Both peers derive the same
32‑byte session key via X25519 ECDH and BLAKE2b (`horse_crypto_derive_session_key()`).

### Signature transport frames

When signing is enabled, the 64‑byte Ed25519 session signature is split across
**six dedicated voice frames** before ordinary voice data:

- Frame numbers: `0x7000` … `0x7005` (`SIG_FRAME_BASE`, `SIG_FRAME_COUNT`)
- Chunks are 12 bytes except the last, which is 4 bytes (`sig_chunk_bytes()`).
  Final on‑air packing is a Step 2 design item; this is a bounds‑only layout.
- The session message signed at TX start is `src || dst || eph_pk` (44 bytes);
  `eph_pk` is included only when the encrypted flag is also set

Per‑frame authentication tags (signed‑only mode) use a key derived from the
verified session signature (`horse_crypto_derive_frame_auth_key()`).
Combined encrypt+sign mode uses the encryption MAC for voice integrity instead.

### Modulation details

Horse uses **4‑FSK (4‑level Frequency Shift Keying)** modulation:

- **Type:** 4‑FSK (4‑level FSK)
- **Symbol rate:** 4800 symbols/s
- **Symbol mapping:** Each byte maps to 4 symbols via a lookup table:
  - `00` → `+1`
  - `01` → `+3`
  - `10` → `-1`
  - `11` → `-3`
- **Filtering:** Root‑raised cosine (RRC) filter from M17 DSP (`M17::rrc_48k`) at 48 kHz sample rate
- **RF characteristics:**
  - Constant‑envelope 4‑FSK (same RF path as M17)
  - Occupies a standard 12.5 kHz channel
  - M17‑compatible RF characteristics

Modulation is generated in `HorseModulator`:

- Symbol stream is produced as small integer levels (4‑level symbols) and written into a 48 kHz baseband buffer.
- A **root‑raised cosine (RRC)** filter from the existing M17 DSP (`M17::rrc_48k`) shapes the symbols.
- On RF, the same constant‑envelope 4‑FSK path used by M17 is reused, so Horse occupies a standard 12.5 kHz channel with M17‑compatible RF characteristics.

For Linux builds, baseband samples are written to `/tmp/horse_output.raw` for analysis.

---

## Encryption, signing, and keying

Horse uses modern cryptographic primitives with per‑session keys and optional signing:

- **Long‑term identity keys:** Ed25519/X25519 keypairs stored in `horse_identity_keys_t` structure:
  - **Ed25519** (32‑byte public, 64‑byte secret): used for digital signatures
  - **X25519** (32‑byte public, 32‑byte secret): used for key exchange
  - Keys are provisioned to the radio via `horse_provision.py` and stored encrypted with a user passphrase
- **Session keys:** 32‑byte symmetric keys derived per call (`HORSE_SESSION_KEY_BYTES`).
- **On‑air key agreement (encrypt mode):**
  - TX generates an ephemeral X25519 keypair and places the public key in the LSF.
  - TX derives `session_key = BLAKE2b(ECDH(eph_sk, contact.x25519_pk))`.
  - RX derives the same key with `BLAKE2b(ECDH(local_x25519_sk, eph_pk))`.
  - Implemented in `horse_crypto_derive_session_key()`.
- **ECIES session key wrapping (API only):**
  - `horse_crypto_ecies_encrypt_session_key()` / `horse_crypto_ecies_decrypt_session_key()`
    wrap an arbitrary session key for storage or tooling; the live Horse TX/RX path
    uses the LSF ephemeral public key scheme above instead.
- **Voice encryption (XChaCha20 + BLAKE2b MAC):**
  - API in `horse_crypto_voice_encrypt(...)` / `horse_crypto_voice_decrypt(...)`.
  - Algorithm: **XChaCha20 stream cipher** with a 96‑bit nonce from the 16‑bit
    frame number and a **32‑bit BLAKE2b tag** keyed with the public string
    `"HVOICETAG"` (not the session key). That MAC is **not** a session‑bound
    authenticator; a replacement is proposed in `HORSE_DESIGN_CHANGES.md`.
- **Digital signatures (Ed25519):**
  - API in `horse_crypto.h`:
    - `horse_crypto_sign()` / `horse_crypto_verify()` — session‑level Ed25519 signature
    - `horse_crypto_build_session_message()` — assemble the 44‑byte signed payload
    - `horse_crypto_derive_frame_auth_key()` — derive per‑call frame auth key from signature
    - `horse_crypto_voice_auth_tag()` / `horse_crypto_voice_auth_verify()` — cleartext per‑frame auth
  - On TX, the session signature is sent in six frames (`fn = 0x7000` … `0x7005`).
  - On RX, frames are reassembled and verified against the contact's `ed25519_pk`.
- **Identity storage and unlock:**
  - `horse_keystore.c` stores the provisioned identity encrypted at rest (Argon2id + XChaCha20‑Poly1305).
  - `settings_t.horse_passphrase` unlocks the keystore at mode enable (`OpMode_Horse::enable()`).
  - Without a valid passphrase or libsodium backend, crypto operations fail closed.
- **Passphrase‑based key derivation (Argon2id):**
  - API in `horse_crypto_argon2id_derive(...)`.
  - Implemented via libsodium `crypto_pwhash` Argon2id only. There is no PBKDF2
    path. Without libsodium the derive call returns false.
  - Parameters are currently `OPSLIMIT_MODERATE` / `MEMLIMIT_MODERATE` (~256 MiB).
    That will not fit STM32F405 SRAM; see `HORSE_DESIGN_CHANGES.md`.

**Implementation status:**

- **Linux emulator / unit tests:** libsodium backend. Frame, crypto, keystore,
  provision packing, and modulator/demodulator loopback tests. `OpMode_Horse`
  refuses TX when crypto, keystore, or peer keys are missing (no cleartext).
  Encrypted or signed RX without a valid session produces no audio.
- **Voice codec:** CODEC2 2400 (two 20 ms frames per 40 ms Horse frame) via `melpe_horse.c`; true MELPe‑2400 is not yet integrated. DMA vs 40 ms framing is an open design item.
- **Embedded targets (MD‑3x0, etc.):** Crypto fails closed when libsodium is not linked (`horse_crypto_available()` returns false). Cross‑compiled libsodium is required for on‑device encrypt/sign. RTX stack, Argon2 RAM, and C5000 TX enable are **unverified** without the cross toolchain.
- **LDPC:** Not LDPC. Repeat‑2 only, until a real code is merged.
- **Demodulator:** Acquisition correlator uses Horse LSF symbols from
  `byteToSymbols(LSF_SYNC_WORD)` (`+3,+3,-1,-1,-1,-1,+3,-3`).

Run unit tests:

```bash
meson setup build_linux
meson test -C build_linux
```

Horse‑specific tests: `Horse Frame Unit Test`, `Horse Crypto Unit Test`.

---

## Codeplug integration and GnuPG key handling

Horse adds minimal, forward‑compatible fields to the existing codeplug structures in `openrtx/include/core/cps.h`.

### Horse channels

Horse‑specific per‑channel information:

- `horseInfo_t`:
  - `rxCan` / `txCan` (4‑bit each): logical **channel IDs** for receive/transmit.
  - `encrypt_en` (1‑bit): enable encrypted voice on this channel.
  - `sign_en` (1‑bit): enable signed voice on this channel.
  - `contact_index` (16‑bit): index into the global contact table for the peer's public keys.

If both `encrypt_en` and `sign_en` are zero, firmware defaults to encrypt when
crypto is available (backward compatible with earlier always‑encrypt behaviour).

Each `channel_t` in the codeplug contains a `mode` (FM/DMR/M17/Horse) and a tagged union of mode‑specific data.  
For Horse, the `horse` field of that union is populated with `horseInfo_t`.

### Horse contacts

Horse extends the generic `contact_t` structure with a Horse‑specific view:

- `contact_t`:
  - `name[32]`: human‑readable contact name.
  - `mode`: which mode the contact is for (DMR, M17, Horse, …).
  - `info.horse` (`horseContact_t` when `mode == OPMODE_HORSE`):
    - `address[6]`: Horse address encoded in the same base‑40 scheme used for M17 callsigns.
    - `x25519_pk[32]`: peer public key for encrypted sessions.
    - `ed25519_pk[32]`: peer public key for signature verification.

Each Horse channel references one contact via `contact_index`. That contact must
carry the peer keys required by the selected mode (X25519 for encrypt,
Ed25519 for sign, both for combined).

### How identity keys fit into this design

Horse identity keys use **Ed25519/X25519** (not GnuPG/OpenPGP directly). The workflow is:

1. **Key generation:** Use `horse_provision.py generate <label>` on a desktop system:
   - Generates Ed25519/X25519 keypairs using libsodium (PyNaCl).
   - Stores the identity in the Linux kernel keyring for secure intermediate storage.
2. **Provisioning to radio:** Use `horse_provision.py provision <label> [--port DEVICE]`:
   - Exports the identity from the kernel keyring.
   - Sends it to the radio over USB‑CDC serial.
   - Radio stores it encrypted with a user passphrase (via `horse_crypto_argon2id_derive`).
3. **At runtime:**
   - Radio unlocks stored identity keys using the user's passphrase.
   - For encrypted transmission: uses X25519 public key from contact to wrap session keys.
   - For signed transmission: uses Ed25519 secret key to sign voice frames or control messages.
   - Contact public keys are stored in the codeplug or a separate key store, referenced via `contact_index`.

**Key storage model:**

- **Desktop side:** Keys stored in Linux kernel keyring (`@user` keyring) with label `openrtx:horse:<label>`.
- **Radio side:** Private keys are stored encrypted in flash, wrapped with a key derived from the user's Horse passphrase.
- **Codeplug:** Stores contact public keys (Ed25519/X25519) or references to them, not private keys.

**Current implementation status:**

- `horse_provision.py` provides key generation and provisioning (USB serial and Linux FIFO).
- `horse_keystore.c` and `horse_provision.c` are integrated in firmware init and the main thread poll loop.
- `OpMode_Horse` applies encrypt/sign/combined modes at runtime when libsodium is available.
- Embedded targets without libsodium fail closed (no cleartext crypto stubs).

---

## Provisioning and key management tools

This fork includes two tools for Horse key management:

### `horse_provision.py` – identity generation and provisioning

- **Location:** `scripts/horse_provision.py`  
- **Purpose:** Generate Ed25519/X25519 identities and provision them to radios.
- **Dependencies:** `pynacl`, `pyserial`, `keyutils` (for kernel keyring access).

**Commands:**

- `generate <label>` — Generate a new Horse identity and store it in the kernel keyring.
- `list` — List all stored identities in the keyring.
- `show <label>` — Display identity details (public keys, fingerprints).
- `provision <label> [--port DEVICE] [--fifo PATH]` — Send identity to radio over USB‑CDC serial or a Linux FIFO (`/tmp/openrtx_horse_prov.fifo` in the emulator).

**Example workflow:**

```bash
# Generate identity for operator "M0ABC"
python3 scripts/horse_provision.py generate M0ABC

# List stored identities
python3 scripts/horse_provision.py list

# Provision identity to radio (auto-detects USB serial port)
python3 scripts/horse_provision.py provision M0ABC

# Or specify port manually
# Or use the Linux emulator FIFO
python3 scripts/horse_provision.py provision M0ABC --fifo /tmp/openrtx_horse_prov.fifo
```

Set the radio passphrase in codeplug settings (`horse_passphrase`, max 32 characters)
before transmitting or receiving encrypted/signed traffic.

### `horse_keytool.py` – collecting Horse public keys from GnuPG

- **Location:** `scripts/horse_keytool.py`  
- **Purpose:** Collect public keys for Horse contacts from:
  - OpenPGP key files (`.asc`, `.pgp`), and
  - keys already present in the user’s GnuPG keyring (including smartcards / Nitrokey),
  and write them into a single JSON mapping file for later use by codeplug tooling.

### Inputs

The tool is a command‑line program that accepts one or more `--contact` arguments:

- `--contact NAME:KEYREF`
  - **NAME**: human‑readable contact name or callsign (e.g. `M0ABC`).
  - **KEYREF**:
    - a path to an ASCII‑armored `.asc` or binary `.pgp` key file, **or**
    - any key reference that `gpg --export` understands (fingerprint, key ID, email, etc.).

Because it uses the standard `gpg` command, keys backed by **Nitrokey** or other smartcards, and keys cached in the **kernel keyring** via GnuPG, are handled transparently as long as GnuPG can export them.

### Output

- `--output / -o PATH` selects an output file, typically `horse_keys.json`.
- The JSON file has the structure:
  - `version`: schema version (integer).
  - `generated_at`: ISO‑8601 UTC timestamp.
  - `entries`: array of objects, each with:
    - `name`: contact name/callsign.
    - `key_ref`: original reference (file path or GnuPG spec).
    - `source`: `"file"` or `"gpg"`.
    - `format`: `"ascii"`, `"text"`, or `"binary"`.
    - `public_key`: the exported public key:
      - ASCII‑armored text for keys coming from GnuPG or `.asc` files.
      - Hex‑encoded binary for `.pgp` files.

### Example usage

From the repository root:

```bash
python3 scripts/horse_keytool.py \
  --output horse_keys.json \
  --contact M0ABC:/path/to/m0abc.asc \
  --contact M0DEF:/path/to/m0def.pgp \
  --contact M0GHI:0xDEADBEEFCAFEBABE
```

This command will:

- Read the `.asc` and `.pgp` files directly.
- Ask `gpg --export --armor 0xDEADBEEFCAFEBABE` for the third contact.
- Write a combined `horse_keys.json` containing public‑key material for all three Horse contacts.

---

## Developer reference

- **Protocol types and framing:**
  - `openrtx/include/protocols/horse/HorseDatatypes.hpp`
  - `openrtx/include/protocols/horse/HorseConstants.hpp`
- **DSP / modulation path:**
  - `openrtx/src/protocols/horse/HorseModulator.cpp`
- **Runtime integration:**
  - `openrtx/src/rtx/OpMode_Horse.cpp` — TX/RX state machine, encrypt/sign/combined paths
  - `openrtx/src/protocols/horse/horse_keystore.c` — encrypted identity storage
  - `openrtx/src/protocols/horse/horse_provision.c` — USB/FIFO provisioning handler
- **Cryptography API:**
  - `openrtx/include/protocols/horse/horse_crypto.h` — API definitions
  - `openrtx/src/protocols/horse/horse_crypto.c` — libsodium backend (fail‑closed without it)
  - Functions:
    - `horse_crypto_derive_session_key()` — X25519 ECDH session key from LSF eph_pk
    - `horse_crypto_voice_encrypt()` / `horse_crypto_voice_decrypt()` — Voice frame encryption (XChaCha20 + BLAKE2b MAC)
    - `horse_crypto_sign()` / `horse_crypto_verify()` — Session Ed25519 signature
    - `horse_crypto_build_session_message()` / `horse_crypto_derive_frame_auth_key()` — Signed‑mode helpers
    - `horse_crypto_voice_auth_tag()` / `horse_crypto_voice_auth_verify()` — Cleartext per‑frame auth
    - `horse_crypto_argon2id_derive()` — Passphrase‑based key derivation
- **Unit tests:**
  - `tests/unit/horse_frame.cpp` — LSF/voice/EOT round‑trip, LSF crypto fields, signature frame transport
  - `tests/unit/horse_crypto.cpp` — Voice encrypt/decrypt, Ed25519, session auth round‑trip
- **Codeplug integration:**
  - `openrtx/include/core/cps.h` (`horseInfo_t`, `horseContact_t`, and `channel_t` / `contact_t` unions)

## Operational modes

Horse supports three channel configurations via `horseInfo_t.encrypt_en` and
`horseInfo_t.sign_en`. All modes require an unlocked identity keystore and a
valid `contact_index` with the peer's public keys.

### 1. Encrypted mode (`encrypt_en=1`, `sign_en=0`)

- LSF carries ephemeral X25519 public key and `LSF_FLAG_ENCRYPTED`.
- Voice payload is encrypted with XChaCha20; 32‑bit MAC in the tag field.
- Provides confidentiality and per‑frame integrity.

### 2. Signed mode (`encrypt_en=0`, `sign_en=1`)

- LSF sets `LSF_FLAG_SIGNED`; no ephemeral key in LSF.
- TX signs `src || dst` (44 bytes, `eph_pk` zeroed) and sends the 64‑byte
  Ed25519 signature in six frames (`fn = 0x7000` … `0x7005`).
- Voice is cleartext; each frame carries a 32‑bit auth tag derived from the
  verified session signature.
- RX drops voice until the signature is reassembled and verified against
  `contact.ed25519_pk`.

### 3. Combined mode (`encrypt_en=1`, `sign_en=1`)

- LSF sets both flags; ephemeral key is included in the signed session message.
- Session signature frames are sent after the LSF, then encrypted voice.
- Per‑frame integrity comes from the encryption MAC (no separate auth tag).

### TX/RX sequence (signed or combined)

1. Link Setup Frame (src, dst, flags, optional eph_pk)
2. Six signature transport frames (if `sign_en`)
3. Voice frames (encrypted, signed‑only, or both per mode)
4. End‑of‑Transmission frame

Implementation: `OpMode_Horse::txState()` / `rxState()` in
`openrtx/src/rtx/OpMode_Horse.cpp`.

---

This document describes the **current implementation** of Horse as implemented in this fork.  
Horse is experimental and under active development. Cryptographic implementations use audited libraries (libsodium) where available, but should be reviewed before production use.

