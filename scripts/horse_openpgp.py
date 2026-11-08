#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Optional OpenPGP helpers for horse_provision.py.
# Secret material moves only over pipes (never temp files on disk).
# Extraction of raw libsodium keys is done by tools/horse-openpgp
# (sequoia-openpgp), not by pysequoia.
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
from pathlib import Path

HORSE_RADIO_NOTATION = "horse-radio@NOTATION-DOMAIN"
HORSE_RADIO_NOTATION_VALUE = "1"
DEFAULT_SUBKEY_EXPIRE = "2y"


def openpgp_available():
    return bool(shutil.which("gpg") or shutil.which("sq")
                or find_helper() is not None)


def find_helper():
    env = os.environ.get("HORSE_OPENPGP_HELPER")
    if env and os.path.isfile(env) and os.access(env, os.X_OK):
        return env
    here = Path(__file__).resolve().parents[1]
    cand = here / "tools" / "horse-openpgp" / "target" / "release" / "horse-openpgp"
    if cand.is_file() and os.access(cand, os.X_OK):
        return str(cand)
    which = shutil.which("horse-openpgp")
    return which


def _run(cmd, input_bytes=None, check=True, env=None, pass_fds=()):
    proc = subprocess.run(
        cmd,
        input=input_bytes,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=False,
        env=env,
        pass_fds=tuple(pass_fds),
    )
    if check and proc.returncode != 0:
        err = proc.stderr.decode("utf-8", errors="replace").strip()
        # Never echo secret material; stderr from gpg/sq is usually safe.
        raise RuntimeError("%s failed: %s" % (" ".join(cmd[:3]), err[:500]))
    return proc


def _gpg_cmd(*args):
    """gpg with non-interactive pinentry (unprotected / HORSE_OPENPGP_PASSPHRASE)."""
    passwd = os.environ.get("HORSE_OPENPGP_PASSPHRASE", "")
    return [
        "gpg", "--batch", "--yes",
        "--pinentry-mode", "loopback",
        "--passphrase", passwd,
    ] + list(args)


def _state_dir():
    env = os.environ.get("XDG_STATE_HOME")
    if env:
        return Path(env) / "OpenRTX"
    home = os.environ.get("HOME")
    if home:
        return Path(home) / ".local/state/OpenRTX"
    raise RuntimeError("neither XDG_STATE_HOME nor HOME is set")


def allowlist_path():
    return _state_dir() / "horse_radio_subkeys.json"


def load_allowlist():
    path = allowlist_path()
    if not path.is_file():
        return {"sign_fps": [], "encrypt_fps": []}
    data = json.loads(path.read_text())
    return {
        "sign_fps": list(data.get("sign_fps", [])),
        "encrypt_fps": list(data.get("encrypt_fps", [])),
    }


def save_allowlist(sign_fps, encrypt_fps):
    path = allowlist_path()
    path.parent.mkdir(parents=True, exist_ok=True)
    payload = {
        "sign_fps": sorted(set(sign_fps)),
        "encrypt_fps": sorted(set(encrypt_fps)),
        "notation": HORSE_RADIO_NOTATION,
        "note": "public fingerprints only; peers need these out of band "
                "until a notation domain is chosen",
    }
    fd, tmp = None, None
    import tempfile

    fd, tmp = tempfile.mkstemp(dir=str(path.parent), prefix=".horse_al_")
    try:
        os.fchmod(fd, 0o600)
        os.write(fd, json.dumps(payload, indent=2, sort_keys=True).encode())
        os.fsync(fd)
    finally:
        os.close(fd)
    os.replace(tmp, str(path))


def parse_secret_colon_field15(line):
    """Return (rectype, field15) for sec/ssb lines; else None.

    Per gnupg DETAILS (Field 15 - S/N of a token), with --with-secret:
      '+'  secret present locally
      '#'  stub (no secret)
      <serial> secret on a smartcard/token
    """
    if not line.startswith(("sec:", "ssb:")):
        return None
    parts = line.rstrip("\n").split(":")
    # Colon records are 1-indexed in the docs; split() index 14 == field 15.
    f15 = parts[14] if len(parts) > 14 else ""
    return parts[0], f15


def reject_subkey_stub_or_card(colon_listing):
    """Reject ssb lines whose field 15 is not '+'. Primary sec may be stub.

    Verified against installed gnupg DETAILS.gz Field 15.
    """
    for line in colon_listing.splitlines():
        parsed = parse_secret_colon_field15(line)
        if parsed is None:
            continue
        rectype, f15 = parsed
        if rectype != "ssb":
            continue
        if f15 == "+":
            continue
        if f15 == "#":
            raise RuntimeError(
                "secret subkey is a stub (no local secret); "
                "the radio needs a raw secret key"
            )
        if f15:
            raise RuntimeError(
                "secret subkey is on a card/token (serial %s); "
                "the radio needs a raw secret key. "
                "The primary key may remain on a token." % f15
            )
        raise RuntimeError(
            "secret subkey field 15 is empty (expected '+'); "
            "re-list with gpg --with-secret --with-colons"
        )


def wrap_identity_openpgp(packed_identity: bytes, recipient: str) -> bytes:
    """Encrypt packed identity to an OpenPGP recipient via stdin pipe."""
    if not shutil.which("gpg"):
        raise RuntimeError("gpg required for OpenPGP identity wrap")
    proc = _run([
        "gpg", "--batch", "--yes",
        "--trust-model", "always",
        "--encrypt",
        "--recipient", recipient,
        "--output", "-",
        "--status-fd", "2",
    ], input_bytes=packed_identity)
    if not proc.stdout:
        raise RuntimeError("gpg encrypt produced empty ciphertext")
    return proc.stdout


def unwrap_identity_openpgp(ciphertext: bytes) -> bytes:
    """Decrypt OpenPGP-wrapped identity via stdin (agent/pinentry)."""
    if not shutil.which("gpg"):
        raise RuntimeError("gpg required for OpenPGP identity unwrap")
    proc = _run([
        "gpg", "--batch", "--decrypt",
        "--output", "-",
        "--status-fd", "2",
    ], input_bytes=ciphertext)
    if not proc.stdout:
        raise RuntimeError("gpg decrypt produced empty plaintext")
    return proc.stdout


def cmd_openpgp_new_radio_key(args, json_mode):
    """Add Horse radio Ed25519 + Cv25519 subkeys to an existing key."""
    expire = getattr(args, "expire", None) or DEFAULT_SUBKEY_EXPIRE
    yes = bool(getattr(args, "yes", False))

    if not args.gpg and not args.sq:
        raise RuntimeError("specify --gpg KEYID or --sq KEYID")

    if args.gpg:
        if not shutil.which("gpg"):
            raise RuntimeError("gpg not found")
        fpr = args.gpg
        plan = {
            "tool": "gpg",
            "primary": fpr,
            "subkeys": [
                {"algo": "ed25519", "usage": "sign", "expire": expire},
                {"algo": "cv25519", "usage": "encr", "expire": expire},
            ],
            "notation": HORSE_RADIO_NOTATION,
            "notation_value": HORSE_RADIO_NOTATION_VALUE,
            "allowlist": str(allowlist_path()),
            "requires_yes": True,
        }
        print(json.dumps(plan, indent=2, sort_keys=True)
              if not json_mode else json.dumps(plan, sort_keys=True))
        if not yes:
            raise RuntimeError(
                "refusing to edit keyring without --yes "
                "(notation domain is still a placeholder: %s)"
                % HORSE_RADIO_NOTATION
            )

        # --cert-notation applies to binding signatures from --quick-add-key
        # (verified on GnuPG 2.4.4: hashed subpkt 20 present).
        before = _run(_gpg_cmd(
            "--with-colons", "--list-keys", fpr,
        )).stdout.decode("utf-8", errors="replace")
        before_fps = set()
        for line in before.splitlines():
            if line.startswith("fpr:"):
                before_fps.add(line.split(":")[9])

        for algo, usage in (("ed25519", "sign"), ("cv25519", "encr")):
            _run(_gpg_cmd(
                "--cert-notation",
                "%s=%s" % (HORSE_RADIO_NOTATION, HORSE_RADIO_NOTATION_VALUE),
                "--quick-add-key", fpr, algo, usage, expire,
            ))

        after = _run(_gpg_cmd(
            "--with-colons", "--list-keys", fpr,
        )).stdout.decode("utf-8", errors="replace")
        new_fps = []
        for line in after.splitlines():
            if line.startswith("fpr:"):
                fp = line.split(":")[9]
                if fp not in before_fps:
                    new_fps.append(fp)

        # Heuristic: first new is sign, second encrypt (add order).
        sign_fps = new_fps[:1]
        enc_fps = new_fps[1:2]
        save_allowlist(
            load_allowlist()["sign_fps"] + sign_fps,
            load_allowlist()["encrypt_fps"] + enc_fps,
        )
        result = {
            "tool": "gpg",
            "primary": fpr,
            "notation": HORSE_RADIO_NOTATION,
            "notation_applied": True,
            "expire": expire,
            "sign_fps": sign_fps,
            "encrypt_fps": enc_fps,
            "allowlist": str(allowlist_path()),
        }
        if json_mode:
            print(json.dumps(result, sort_keys=True))
        else:
            print("Added Ed25519 sign + Cv25519 encrypt subkeys to %s" % fpr)
            print("Notation %s applied; fingerprints also in %s" %
                  (HORSE_RADIO_NOTATION, allowlist_path()))
        return 0

    if not shutil.which("sq"):
        raise RuntimeError("sq not found")
    help_out = _run(["sq", "key", "subkey", "add", "--help"],
                    check=False).stdout.decode("utf-8", errors="replace")
    plan = {
        "tool": "sq",
        "primary": args.sq,
        "expire": expire,
        "notation": HORSE_RADIO_NOTATION,
        "help_has_notation": "--notation" in help_out,
        "allowlist": str(allowlist_path()),
        "requires_yes": True,
    }
    print(json.dumps(plan, indent=2, sort_keys=True)
          if not json_mode else json.dumps(plan, sort_keys=True))
    if not yes:
        raise RuntimeError(
            "refusing to edit keyring without --yes "
            "(notation domain is still a placeholder: %s)"
            % HORSE_RADIO_NOTATION
        )

    # sq 0.33 / 1.x flags vary; probe.
    def add_one(can_flag):
        cmd = ["sq", "key", "subkey", "add", can_flag]
        if "--cipher-suite" in help_out:
            cmd += ["--cipher-suite", "cv25519"]
        if "--expires" in help_out:
            cmd += ["--expires", expire]
        elif "--expiration" in help_out:
            cmd += ["--expiration", expire]
        if "--notation" in help_out:
            cmd += [
                "--notation",
                "%s=%s" % (HORSE_RADIO_NOTATION, HORSE_RADIO_NOTATION_VALUE),
            ]
        cmd += [args.sq]
        _run(cmd)

    add_one("--can-sign")
    add_one("--can-encrypt")
    result = {
        "tool": "sq",
        "primary": args.sq,
        "notation": HORSE_RADIO_NOTATION,
        "notation_applied": "--notation" in help_out,
        "expire": expire,
        "allowlist": str(allowlist_path()),
        "note": "record subkey fingerprints in the allowlist out of band "
                "if notation was not applied",
    }
    if json_mode:
        print(json.dumps(result, sort_keys=True))
    else:
        print("Added subkeys via sq for %s" % args.sq)
    return 0


def _helper_run(argv, input_bytes=None, pass_fds=()):
    helper = find_helper()
    if helper is None:
        raise RuntimeError(
            "horse-openpgp helper not found; build tools/horse-openpgp "
            "or set HORSE_OPENPGP_HELPER"
        )
    cmd = [helper] + argv
    env = os.environ.copy()
    return _run(cmd, input_bytes=input_bytes, env=env, pass_fds=pass_fds)


def cmd_openpgp_extract_identity(args, json_mode, passphrase,
                                 store_identity_file, public_view, emit):
    keyid = args.gpg or args.sq
    if not keyid:
        raise RuntimeError("specify --gpg KEYID or --sq KEYID")
    helper = find_helper()
    if helper is None:
        require = os.environ.get("HORSE_REQUIRE_CRYPTO") == "1"
        msg = "horse-openpgp helper missing"
        if require:
            raise RuntimeError(msg)
        print(msg + " (skip)", file=sys.stderr)
        return 77

    if args.gpg:
        if not shutil.which("gpg"):
            raise RuntimeError("gpg not found")
        listing = _run(_gpg_cmd(
            "--with-colons", "--with-secret", "--list-secret-keys", keyid,
        )).stdout.decode("utf-8", errors="replace")
        reject_subkey_stub_or_card(listing)
        # Full secret export keeps primary binding signatures sequoia needs.
        secrets = [_run(_gpg_cmd("--export-secret-keys", keyid)).stdout]
        sub = _run(_gpg_cmd("--export-secret-subkeys", keyid)).stdout
        if sub and sub != secrets[0]:
            secrets.append(sub)
    else:
        if not shutil.which("sq"):
            raise RuntimeError("sq not found")
        secret = _run(["sq", "key", "export", args.sq], check=False).stdout
        if not secret:
            raise RuntimeError("sq key export empty")
        secrets = [secret]

    secrets = [s for s in secrets if s]
    if not secrets:
        raise RuntimeError("empty secret key export")

    # Horse identity passphrase is separate (store wrap).
    if passphrase is None:
        raise RuntimeError("Horse passphrase required to store identity")
    # OpenPGP key passphrase via anonymous pipe to helper --passphrase-fd.
    # Never argv or environment for the secret itself; HORSE_OPENPGP_PASSPHRASE
    # is only for throwaway CI keys (empty when unprotected).
    opgp_pass = os.environ.get("HORSE_OPENPGP_PASSPHRASE", "")
    allow = ["--allow-unmarked"] if args.allow_unmarked else []
    al = allowlist_path()
    argv_base = ["extract-secret"] + allow
    if al.is_file():
        argv_base += ["--allowlist", str(al)]

    proc = None
    last_err = None
    for secret in secrets:
        r_fd, w_fd = os.pipe()
        try:
            os.write(w_fd, opgp_pass.encode("utf-8"))
            os.close(w_fd)
            w_fd = -1
            argv = argv_base + ["--passphrase-fd", str(r_fd)]
            try:
                proc = _helper_run(
                    argv, input_bytes=secret, pass_fds=(r_fd,),
                )
                last_err = None
                break
            except RuntimeError as e:
                last_err = e
                proc = None
        finally:
            try:
                os.close(r_fd)
            except OSError:
                pass
            if w_fd >= 0:
                try:
                    os.close(w_fd)
                except OSError:
                    pass
    if proc is None:
        raise last_err if last_err else RuntimeError("extract-secret failed")

    data = json.loads(proc.stdout.decode("utf-8"))
    identity = {
        "version": 1,
        "ed25519_pk": data["ed25519_pk"],
        "ed25519_sk": data["ed25519_seed"],  # 32-byte seed; pack expands
        "x25519_pk": data["x25519_pk"],
        "x25519_sk": data["x25519_sk"],
    }
    path = store_identity_file(
        identity, passphrase, args.label, force=args.force
    )
    view = public_view(identity, args.label)
    view["path"] = str(path)
    view["cert_fpr"] = data.get("cert_fpr")
    emit(view, json_mode)
    return 0


def cmd_openpgp_import_peer(args, json_mode, export_peer_record, emit):
    helper = find_helper()
    if helper is None:
        require = os.environ.get("HORSE_REQUIRE_CRYPTO") == "1"
        msg = "horse-openpgp helper missing"
        if require:
            raise RuntimeError(msg)
        print(msg + " (skip)", file=sys.stderr)
        return 77

    if args.file:
        cert = Path(args.file).read_bytes()
    elif args.gpg:
        if not shutil.which("gpg"):
            raise RuntimeError("gpg not found")
        cert = _run(["gpg", "--batch", "--export", args.gpg]).stdout
    elif args.sq:
        if not shutil.which("sq"):
            raise RuntimeError("sq not found")
        cert = _run(["sq", "cert", "export", args.sq], check=False).stdout
        if not cert:
            cert = _run(["sq", "key", "export", args.sq], check=False).stdout
    else:
        raise RuntimeError("specify --file, --gpg KEYID, or --sq KEYID")

    if not cert:
        raise RuntimeError("empty certificate")

    allow = ["--allow-unmarked"] if args.allow_unmarked else []
    al = allowlist_path()
    argv = ["extract-public"] + allow
    if al.is_file():
        argv += ["--allowlist", str(al)]
    proc = _helper_run(argv, input_bytes=cert)
    data = json.loads(proc.stdout.decode("utf-8"))

    address = bytes.fromhex(args.address)
    rec = export_peer_record(
        args.index,
        address,
        bytes.fromhex(data["x25519_pk"]),
        bytes.fromhex(data["ed25519_pk"]),
    )
    out = {
        "peer_index": args.index,
        "cert_fpr": data.get("cert_fpr"),
        "horse_fingerprint": data.get("horse_fingerprint"),
        "ed25519_pk": data["ed25519_pk"],
        "x25519_pk": data["x25519_pk"],
        "bytes": len(rec),
    }
    if json_mode:
        print(json.dumps(out, sort_keys=True))
    else:
        print("Imported peer at index %u" % args.index)
        print("  Certificate fingerprint: %s" % out.get("cert_fpr"))
        print("  Horse fingerprint:       %s" % out.get("horse_fingerprint"))
    return 0


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
