# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
"""pytest coverage for scripts/horse_provision.py (public data only)."""

from __future__ import annotations

import os
import socket
import struct
import sys
import threading
import time
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
sys.path.insert(0, str(SCRIPTS))

pytest.importorskip("nacl")

import horse_provision as hp


@pytest.fixture()
def xdg(tmp_path, monkeypatch):
    monkeypatch.setenv("XDG_STATE_HOME", str(tmp_path))
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    return tmp_path


def test_wrap_unwrap_roundtrip(xdg):
    ident = hp.generate_identity("rt")
    wrapped = hp.wrap_identity(ident, "pytest-pass-ok")
    opened = hp.unwrap_identity(wrapped, "pytest-pass-ok")
    assert opened["ed25519_pk"] == ident["ed25519_pk"]
    assert opened["x25519_pk"] == ident["x25519_pk"]


def test_tampered_ciphertext(xdg):
    ident = hp.generate_identity("rt")
    wrapped = bytearray(hp.wrap_identity(ident, "pytest-pass-ok"))
    wrapped[40] ^= 0xFF
    with pytest.raises(Exception):
        hp.unwrap_identity(bytes(wrapped), "pytest-pass-ok")


def test_wrong_passphrase(xdg):
    ident = hp.generate_identity("rt")
    wrapped = hp.wrap_identity(ident, "pytest-pass-ok")
    with pytest.raises(Exception):
        hp.unwrap_identity(wrapped, "wrong-pass!")


def test_header_fields_rejected(xdg):
    ident = hp.generate_identity("rt")
    wrapped = bytearray(hp.wrap_identity(ident, "pytest-pass-ok"))
    # Corrupt magic
    wrapped[0] ^= 0xFF
    with pytest.raises(ValueError):
        hp.unwrap_identity(bytes(wrapped), "pytest-pass-ok")


def test_truncated_file(xdg):
    with pytest.raises(ValueError):
        hp.unwrap_identity(b"short", "pytest-pass-ok")


def test_seed_pubkey_mismatch_32_and_64():
    import nacl.signing

    sk = nacl.signing.SigningKey.generate()
    seed = bytes(sk)
    pk = bytes(sk.verify_key)
    xsk = __import__("nacl.public", fromlist=["PrivateKey"]).PrivateKey.generate()
    base = {
        "ed25519_pk": pk.hex(),
        "ed25519_sk": seed.hex(),
        "x25519_pk": bytes(xsk.public_key).hex(),
        "x25519_sk": bytes(xsk).hex(),
    }
    hp.pack_identity_binary(base)
    bad = dict(base)
    bad["ed25519_pk"] = bytes(range(32)).hex()
    with pytest.raises(ValueError):
        hp.pack_identity_binary(bad)
    bad64 = dict(base)
    bad64["ed25519_sk"] = (seed + bytes(range(32))).hex()
    with pytest.raises(ValueError):
        hp.pack_identity_binary(bad64)


def test_overwrite_refused_without_force(xdg):
    ident = hp.generate_identity("lab")
    hp.store_identity_file(ident, "pytest-pass-ok", "lab")
    with pytest.raises(RuntimeError, match="--force"):
        hp.store_identity_file(ident, "pytest-pass-ok", "lab", force=False)
    hp.store_identity_file(ident, "pytest-pass-ok", "lab", force=True)


def test_atomic_write_leaves_old_on_interrupt(xdg, monkeypatch):
    path = hp.identity_path("atom")
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_bytes(b"OLDCONTENT")
    real_replace = os.replace

    def boom(src, dst):
        raise OSError("simulated failure before replace")

    monkeypatch.setattr(os, "replace", boom)
    with pytest.raises(OSError):
        hp.atomic_write(path, b"NEWCONTENT")
    assert path.read_bytes() == b"OLDCONTENT"
    monkeypatch.setattr(os, "replace", real_replace)
    hp.atomic_write(path, b"NEWCONTENT")
    assert path.read_bytes() == b"NEWCONTENT"


def test_label_sanitising():
    with pytest.raises(ValueError):
        hp.sanitize_label("../etc")
    with pytest.raises(ValueError):
        hp.sanitize_label("a/b")
    with pytest.raises(ValueError):
        hp.sanitize_label("bad label")
    assert hp.sanitize_label("ok_label-1.2") == "ok_label-1.2"


def test_peer_indices_and_range(xdg):
    import nacl.public
    import nacl.signing

    sk = nacl.signing.SigningKey.generate()
    xsk = nacl.public.PrivateKey.generate()
    addr = b"ABCDEF"
    hp.export_peer_record(1, addr, bytes(xsk.public_key), bytes(sk.verify_key))
    hp.export_peer_record(64, addr, bytes(xsk.public_key), bytes(sk.verify_key))
    with pytest.raises(ValueError):
        hp.export_peer_record(0, addr, bytes(xsk.public_key), bytes(sk.verify_key))
    with pytest.raises(ValueError):
        hp.export_peer_record(65, addr, bytes(xsk.public_key), bytes(sk.verify_key))


def test_peer_zero_and_small_order(xdg):
    import nacl.signing

    sk = nacl.signing.SigningKey.generate()
    pk = bytes(sk.verify_key)
    with pytest.raises(ValueError):
        hp.pack_peer(bytes(6), bytes(32), pk)
    with pytest.raises(ValueError):
        hp.pack_peer(b"ABCDEF", bytes(32), pk)


def test_passphrase_not_in_argv_output(xdg, monkeypatch, capsys):
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    rc = hp.main(["--json", "generate", "sec"])
    assert rc == 0
    out = capsys.readouterr().out
    assert "pytest-pass-ok" not in out
    assert "ed25519_sk" not in out


def test_parse_args_rejects_typos():
    with pytest.raises(SystemExit):
        hp.build_parser().parse_args(["generatee", "x"])


def _fake_radio(sock_path, fingerprint, mode="ok"):
    """Serve HELLO_ACK then CONFIRM/ERROR on a Unix socket."""

    def run():
        srv = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        try:
            os.unlink(sock_path)
        except FileNotFoundError:
            pass
        srv.bind(sock_path)
        srv.listen(1)
        srv.settimeout(5.0)
        conn, _ = srv.accept()
        try:
            hdr = conn.recv(4)
            ver, typ, plen = struct.unpack("<BBH", hdr)
            assert typ == hp.MSG_HELLO
            conn.sendall(struct.pack("<BBH", 1, hp.MSG_HELLO_ACK, 0))
            hdr = conn.recv(4)
            ver, typ, plen = struct.unpack("<BBH", hdr)
            body = b""
            while len(body) < plen:
                body += conn.recv(plen - len(body))
            if mode == "error":
                err = b"boom"
                conn.sendall(struct.pack("<BBH", 1, hp.MSG_ERROR, len(err)) + err)
            elif mode == "badfp":
                bad = bytes(32)
                conn.sendall(struct.pack("<BBH", 1, hp.MSG_CONFIRM, 32) + bad)
            elif mode == "timeout":
                time.sleep(3.0)
            else:
                conn.sendall(
                    struct.pack("<BBH", 1, hp.MSG_CONFIRM, 32) + fingerprint
                )
        finally:
            conn.close()
            srv.close()

    t = threading.Thread(target=run, daemon=True)
    t.start()
    time.sleep(0.05)
    return t


def test_provision_happy_and_error_and_badfp(xdg, tmp_path):
    ident = hp.generate_identity("prov")
    fp = hp.identity_fingerprint(ident["ed25519_pk"], ident["x25519_pk"])
    sock = str(tmp_path / "prov.sock")
    _fake_radio(sock, fp, "ok")
    assert hp.provision_to_radio(ident, sock=sock) is True

    sock2 = str(tmp_path / "prov2.sock")
    _fake_radio(sock2, fp, "error")
    assert hp.provision_to_radio(ident, sock=sock2) is False

    sock3 = str(tmp_path / "prov3.sock")
    _fake_radio(sock3, fp, "badfp")
    assert hp.provision_to_radio(ident, sock=sock3) is False


def test_selftest_exit_codes(monkeypatch):
    monkeypatch.setenv("HORSE_REQUIRE_CRYPTO", "1")
    assert hp.selftest() == 0
