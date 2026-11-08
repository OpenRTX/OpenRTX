<!--
SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
-->

# Horse identity provisioning

Host-side tools that create and load Horse identities. Secrets never
belong on argv in normal use; prefer `--passphrase-fd` or a TTY prompt.
`HORSE_PASSPHRASE` is for automated tests only.

| Tool | Role |
|------|------|
| [`scripts/horse_provision.py`](../../scripts/horse_provision.py) | Generate / show / provision identities; peer export; OpenPGP commands |
| [`scripts/horse_openpgp.py`](../../scripts/horse_openpgp.py) | OpenPGP helpers imported by `horse_provision.py` (not a CLI by itself) |
| [`tools/horse-openpgp/`](../../tools/horse-openpgp/) | Rust sequoia helper (`inspect-cert`, `extract-public`, `extract-secret`) |

Protocol and store formats: [horse.md](../../horse.md). Hardware steps:
[HARDWARE_TEST_PLAN.md](./HARDWARE_TEST_PLAN.md). Test snapshot:
[TEST_RESULTS.md](./TEST_RESULTS.md).

## Dependencies

```bash
uv venv && uv pip install -r requirements.txt
# OpenPGP path also needs: gpg and/or sq, plus the helper binary
(cd tools/horse-openpgp && cargo build --release --locked)
export HORSE_OPENPGP_HELPER="$PWD/tools/horse-openpgp/target/release/horse-openpgp"
```

If `CARGO_TARGET_DIR` points elsewhere, set `HORSE_OPENPGP_HELPER` to that
`release/horse-openpgp` path. Lookup order: `HORSE_OPENPGP_HELPER`, then
`tools/horse-openpgp/target/release/horse-openpgp`, then `PATH`.

CI builds the helper with `cargo test` / `cargo build --release --locked`,
caches cargo by lockfile **and** helper sources, and deletes the release
binary before rebuild so a restored `target/` cannot skip an outdated
link.

## Identity store

Files live under `$XDG_STATE_HOME/OpenRTX/` (or `~/.local/state/OpenRTX/`):

- `horse_identity.bin` or `horse_identity_<label>.bin` (mode 0600)
- Store **version 2**: Argon2id + XChaCha20-Poly1305 (Horse passphrase)
- Store **version 3**: packed identity encrypted to an OpenPGP recipient
  (decrypt via gpg/sq agent; no Horse passphrase on unwrap)
- `horse_radio_subkeys.json`: public radio-subkey fingerprints only
  (allowlist used when notation is absent)

Passphrase length: 8 to 32 bytes inclusive.

## Environment

| Variable | Role |
|----------|------|
| `HORSE_PASSPHRASE` | Horse store passphrase (tests / automation only) |
| `HORSE_OPENPGP_PASSPHRASE` | OpenPGP key passphrase for gpg loopback and the helper `--passphrase-fd` pipe (empty for unprotected throwaway keys) |
| `HORSE_OPENPGP_HELPER` | Absolute path to the `horse-openpgp` binary |
| `HORSE_REQUIRE_CRYPTO=1` | Fail (do not skip) when gpg/sq/helper are missing |
| `OPENRTX_HORSE_PROV_SOCK` | Override Linux emulator provision socket path |
| `GNUPGHOME` | Isolated gpg home (pytest / local throwaway keys) |

## Common commands

```bash
# Self-check (PyNaCl pack/wrap roundtrip)
python3 scripts/horse_provision.py selftest

# Create an identity (prompt for passphrase, or --passphrase-fd N)
python3 scripts/horse_provision.py generate myradio
python3 scripts/horse_provision.py list
python3 scripts/horse_provision.py show myradio

# Linux emulator socket (default $XDG_RUNTIME_DIR/openrtx_horse_prov.sock,
# mode 0600; override OPENRTX_HORSE_PROV_SOCK)
python3 scripts/horse_provision.py provision myradio --sock

# USB serial (list candidates first)
python3 scripts/horse_provision.py --list-ports
python3 scripts/horse_provision.py provision myradio --port /dev/ttyACM0

# Export a peer public record for the sidecar table (public keys only)
python3 scripts/horse_provision.py export-peer 3 \
  --address 010203040506 \
  --x25519 <64 hex> --ed25519 <64 hex>
```

Machine-readable public fields: add `--json` (never prints secrets).

## OpenPGP workflow

Notation domain is still a placeholder (`horse-radio@NOTATION-DOMAIN`).
Until a real domain is chosen, subkeys may be recorded in
`$XDG_STATE_HOME/OpenRTX/horse_radio_subkeys.json` (public fingerprints
only). Peers need those fingerprints out of band when notation is absent.
`--allow-unmarked` selects the first usable signing and encryption
subkeys by key flags without requiring notation or the allowlist.

```bash
# Plan only unless --yes (edits the keyring). Default subkey expiry: 2y
python3 scripts/horse_provision.py openpgp-new-radio-key \
  --gpg <PRIMARY_FPR> --expire 2y
python3 scripts/horse_provision.py openpgp-new-radio-key \
  --gpg <PRIMARY_FPR> --expire 2y --yes

# Extract radio subkeys into a Horse v2 identity file (needs Horse
# passphrase for the Argon2id wrap; OpenPGP passphrase is separate)
python3 scripts/horse_provision.py --passphrase-fd 0 \
  openpgp-extract-identity fromgpg \
  --gpg <PRIMARY_FPR> --allow-unmarked --force <<'EOF'
horsestore-pass
EOF

# Import a peer certificate (file / gpg / sq) into the peer table
python3 scripts/horse_provision.py openpgp-import-peer 3 \
  --file peer.pgp --address 010203040506 --allow-unmarked

# Re-wrap a v2 file as store version 3 (OpenPGP ciphertext)
python3 scripts/horse_provision.py openpgp-wrap myradio \
  --recipient <RECIPIENT_FPR> --force

# Export a public certificate
python3 scripts/horse_provision.py openpgp-export --gpg <FPR> -o cert.asc
```

`openpgp-extract-identity` exports secret material over pipes only
(`gpg --export-secret-keys`, with a subkeys export retry). Card stubs
and token-held subkeys are refused before extract (gpg colon field 15
on `ssb` lines must be `+`; primary may be offline).

Helper CLI (JSON on stdout; errors on stderr; no secrets in errors):

```bash
horse-openpgp inspect-cert < cert.pgp
horse-openpgp extract-public [--allow-unmarked] [--allowlist path] < cert.pgp
horse-openpgp extract-secret --passphrase-fd N [--allow-unmarked] \
  [--allowlist path] < secret.pgp
```

Under `--allow-unmarked`, the helper retries sequoia `StandardPolicy` at
skewed reference times and falls back to `NullPolicy` so freshly minted
gpg keys are usable in CI; subkeys are still selected by key flags.

## Safety

- Never commit, log, or write secret key material outside the encrypted
  identity store.
- Do not put passphrases on argv except for throwaway tests.
- Card stubs and token-held subkeys are refused (gpg colon field 15 on
  `ssb` lines must be `+`).
- RF provisioning to a real radio is **untested on hardware** in CI;
  use a dummy load when trying on a device.
