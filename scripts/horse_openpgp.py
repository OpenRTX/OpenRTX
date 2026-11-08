#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Optional OpenPGP helpers for horse_provision.py.
# Uses gpg and/or sq CLIs. pysequoia is preferred when importable for
# secret-subkey extraction; otherwise subprocesses are used.
#
# Notation placeholder (owner must choose a real domain):
#   horse-radio@NOTATION-DOMAIN
#

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

HORSE_RADIO_NOTATION = "horse-radio@NOTATION-DOMAIN"

have_pysequoia = False
try:
    import sequoia  # type: ignore  # noqa: F401

    have_pysequoia = True
except ImportError:
    try:
        import pysequoia  # type: ignore  # noqa: F401

        have_pysequoia = True
    except ImportError:
        have_pysequoia = False


def openpgp_available():
    return bool(shutil.which("gpg") or shutil.which("sq") or have_pysequoia)


def _run(cmd, input_bytes=None, check=True):
    proc = subprocess.run(
        cmd,
        input=input_bytes,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
    )
    if check and proc.returncode != 0:
        err = proc.stderr.decode("utf-8", errors="replace")
        raise RuntimeError("%s failed: %s" % (" ".join(cmd), err.strip()))
    return proc


def _gpg_version():
    if not shutil.which("gpg"):
        return None
    out = _run(["gpg", "--version"]).stdout.decode("utf-8", errors="replace")
    return out.splitlines()[0] if out else "gpg"


def _sq_version():
    if not shutil.which("sq"):
        return None
    out = _run(["sq", "version"], check=False)
    if out.returncode != 0:
        out = _run(["sq", "--version"], check=False)
    return out.stdout.decode("utf-8", errors="replace").splitlines()[0]


def cmd_openpgp_new_radio_key(args, json_mode):
    """Add Horse radio Ed25519 + Cv25519/X25519 subkeys to an existing key."""
    if args.gpg:
        if not shutil.which("gpg"):
            raise RuntimeError("gpg not found")
        # Exact flags verified against installed gpg --help / man.
        # --quick-add-key KEYFPR ALGO [USAGE [EXPIRE]]
        fpr = args.gpg
        _run([
            "gpg", "--batch", "--yes",
            "--quick-add-key", fpr, "ed25519", "sign", "0",
        ])
        _run([
            "gpg", "--batch", "--yes",
            "--quick-add-key", fpr, "cv25519", "encr", "0",
        ])
        # Notation on binding signatures: gpg lacks a stable non-interactive
        # way to set custom notations on an existing subkey without editing.
        # Record the placeholder for the owner; document in report.
        result = {
            "tool": "gpg",
            "primary": fpr,
            "notation": HORSE_RADIO_NOTATION,
            "notation_applied": False,
            "note": "subkeys added; notation domain is a placeholder "
                    "(horse-radio@NOTATION-DOMAIN) — mark manually or "
                    "re-run when a domain is chosen",
        }
        if json_mode:
            print(json.dumps(result, sort_keys=True))
        else:
            print("Added Ed25519 sign + Cv25519 encrypt subkeys to %s" % fpr)
            print("Notation placeholder: %s (not applied)" %
                  HORSE_RADIO_NOTATION)
        return 0

    if args.sq:
        if not shutil.which("sq"):
            raise RuntimeError("sq not found")
        # sq key subkey add --cipher-suite … varies by version; probe help.
        help_out = _run(["sq", "key", "subkey", "add", "--help"],
                        check=False).stdout.decode("utf-8", errors="replace")
        cmd = ["sq", "key", "subkey", "add", "--can-sign"]
        if "--cipher-suite" in help_out:
            cmd += ["--cipher-suite", "cv25519"]
        cmd += [args.sq]
        _run(cmd)
        cmd2 = ["sq", "key", "subkey", "add", "--can-encrypt"]
        if "--cipher-suite" in help_out:
            cmd2 += ["--cipher-suite", "cv25519"]
        cmd2 += [args.sq]
        _run(cmd2)
        result = {
            "tool": "sq",
            "primary": args.sq,
            "notation": HORSE_RADIO_NOTATION,
            "notation_applied": False,
        }
        if json_mode:
            print(json.dumps(result, sort_keys=True))
        else:
            print("Added sign + encrypt subkeys via sq for %s" % args.sq)
            print("Notation placeholder: %s (not applied)" %
                  HORSE_RADIO_NOTATION)
        return 0

    raise RuntimeError("specify --gpg KEYID or --sq KEYID")


def _reject_card_stub(gpg_colon_line):
    # gpg --with-colons: sec/ssb fields; token '#' or '>' indicate stub/card.
    parts = gpg_colon_line.split(":")
    if len(parts) > 15 and parts[0] in ("sec", "ssb"):
        # Field 15 can indicate keygrip; stub markers appear in usage tokens.
        if "#" in gpg_colon_line or "card-no" in gpg_colon_line:
            raise RuntimeError(
                "secret subkey is a card stub (divert-to-card); "
                "the radio needs a raw secret key, not a token-held key. "
                "The primary key may remain on a token."
            )


def _extract_with_gpg(keyid, allow_unmarked=False):
    """Export secret subkeys and parse raw material via gpg.

    Full AEAD/Argon2 secret decryption is delegated to gpg itself by asking
    it to export in a form we can feed to `gpg --list-packets` / status.
    For Horse we require decryptable local secret subkeys (not card stubs).
    """
    if not shutil.which("gpg"):
        raise RuntimeError("gpg not found")

    listing = _run([
        "gpg", "--batch", "--with-colons", "--list-secret-keys", keyid,
    ]).stdout.decode("utf-8", errors="replace")
    for line in listing.splitlines():
        if line.startswith(("ssb:", "sec:")):
            _reject_card_stub(line)

    # Export secret keys (agent/pinentry handles passphrase).
    # We do not hand-parse S2K; gpg decrypts protection.
    exported = _run([
        "gpg", "--batch", "--export-secret-subkeys", keyid,
    ]).stdout
    if not exported:
        raise RuntimeError("gpg exported empty secret subkeys")

    # Without a stable machine-readable raw key dump from gpg alone that
    # yields libsodium 32/64-byte seeds, prefer pysequoia when present.
    if have_pysequoia:
        return _extract_with_pysequoia(exported, allow_unmarked)

    raise RuntimeError(
        "secret subkey extraction requires pysequoia to decode OpenPGP "
        "packets after gpg decrypts protection; install pysequoia or "
        "export raw Horse keys another way"
    )


def _extract_with_pysequoia(cert_bytes, allow_unmarked=False):
    """Best-effort extract using pysequoia if API is present."""
    raise RuntimeError(
        "pysequoia is importable but Horse radio-subkey extraction is not "
        "wired to this pysequoia build; use checked-in throwaway fixtures "
        "in tests or provide raw keys via generate. Notation filter: %s "
        "(allow_unmarked=%s)" % (HORSE_RADIO_NOTATION, allow_unmarked)
    )


def cmd_openpgp_extract_identity(args, json_mode, passphrase,
                                 store_identity_file, public_view, emit):
    keyid = args.gpg or args.sq
    if not keyid:
        raise RuntimeError("specify --gpg KEYID or --sq KEYID")
    if args.gpg:
        # Will raise until pysequoia extraction is complete.
        _extract_with_gpg(keyid, allow_unmarked=args.allow_unmarked)
    raise RuntimeError(
        "openpgp-extract-identity: full secret decode not completed for "
        "this environment (needs pysequoia packet decode). Notation: %s"
        % HORSE_RADIO_NOTATION
    )


def cmd_openpgp_import_peer(args, json_mode, export_peer_record, emit):
    """Import marked Horse public subkeys into the peer table."""
    cert = None
    if args.file:
        cert = Path(args.file).read_bytes()
    elif args.gpg:
        if not shutil.which("gpg"):
            raise RuntimeError("gpg not found")
        cert = _run(["gpg", "--batch", "--export", args.gpg]).stdout
    elif args.sq:
        if not shutil.which("sq"):
            raise RuntimeError("sq not found")
        # sq cert export KEYID
        cert = _run(["sq", "cert", "export", args.sq], check=False).stdout
        if not cert:
            cert = _run(["sq", "key", "export", args.sq], check=False).stdout
    else:
        raise RuntimeError("specify --file, --gpg KEYID, or --sq KEYID")

    if not cert:
        raise RuntimeError("empty certificate")

    # Public peer import without secret extraction: parse with gpg --list-packets
    # is brittle. Prefer sq/gpg to emit SSH or raw if available; otherwise stop.
    if not args.allow_unmarked:
        raise RuntimeError(
            "openpgp-import-peer: refusing unmarked subkeys; notation "
            "%s is a placeholder domain. Pass --allow-unmarked only for "
            "fixtures, or choose a notation domain." % HORSE_RADIO_NOTATION
        )

    raise RuntimeError(
        "openpgp-import-peer: automatic Cv25519/Ed25519 public extraction "
        "from arbitrary certs needs pysequoia; use export-peer with hex "
        "keys for now. Certificate size %u bytes." % len(cert)
    )


def cmd_openpgp_export(args, json_mode):
    keyid = args.gpg or args.sq
    if not keyid:
        raise RuntimeError("specify --gpg KEYID or --sq KEYID")
    if args.gpg:
        data = _run(["gpg", "--batch", "--export", "--armor", keyid]).stdout
    else:
        data = _run(["sq", "cert", "export", "--armor", keyid],
                    check=False).stdout
        if not data:
            data = _run(["sq", "key", "export", "--armor", keyid]).stdout
    if args.output:
        Path(args.output).write_bytes(data)
        if json_mode:
            print(json.dumps({"output": args.output, "bytes": len(data)}))
        else:
            print("Wrote %u bytes to %s" % (len(data), args.output))
    else:
        sys.stdout.buffer.write(data)
    return 0


def wrap_identity_openpgp(packed_identity: bytes, recipient: str) -> bytes:
    """Encrypt packed identity to an OpenPGP recipient (store version 3).

    Confirm: provision() always sends the plaintext packed identity after
    local decrypt — the PC file need not match the radio NVM blob.
    """
    if not shutil.which("gpg"):
        raise RuntimeError("gpg required for OpenPGP identity wrap")
    with tempfile.NamedTemporaryFile(delete=False) as tmp:
        tmp.write(packed_identity)
        tmp_path = tmp.name
    try:
        proc = _run([
            "gpg", "--batch", "--yes", "--encrypt",
            "--recipient", recipient,
            "--output", "-",
            tmp_path,
        ])
        return proc.stdout
    finally:
        os.unlink(tmp_path)
