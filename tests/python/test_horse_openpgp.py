#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# OpenPGP helper tests. All key material is throwaway, in temp GNUPGHOME.
#
"""Tests for horse_openpgp and tools/horse-openpgp."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "scripts"))

import horse_openpgp as hopg
import horse_provision as hp

FIXTURES = ROOT / "tests" / "fixtures" / "openpgp"
HELPER = hopg.find_helper()
REQUIRE = os.environ.get("HORSE_REQUIRE_CRYPTO") == "1"


def _skip_or_fail(msg):
    if REQUIRE:
        pytest.fail(msg)
    pytest.skip(msg)


@pytest.fixture(scope="module")
def helper_path():
    if HELPER is None:
        _skip_or_fail("horse-openpgp helper missing")
    return HELPER


@pytest.fixture()
def gnupg_home(tmp_path, monkeypatch):
    home = tmp_path / "gnupg"
    home.mkdir()
    home.chmod(0o700)
    monkeypatch.setenv("GNUPGHOME", str(home))
    monkeypatch.setenv("XDG_STATE_HOME", str(tmp_path / "state"))
    yield home
    subprocess.run(
        ["gpgconf", "--kill", "all"],
        check=False,
        env={**os.environ, "GNUPGHOME": str(home)},
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )


def _gpg(*args, check=True, input_bytes=None):
    return subprocess.run(
        ["gpg", "--batch", "--yes", "--pinentry-mode", "loopback",
         "--passphrase", ""] + list(args),
        input=input_bytes,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        check=check,
    )


def _make_radio_key(gnupg_home):
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    _gpg("--quick-gen-key",
         "Horse Throwaway <horse-throwaway@example.test>",
         "ed25519", "cert", "2y")
    listing = _gpg("--with-colons", "--list-secret-keys").stdout.decode()
    fpr = None
    for line in listing.splitlines():
        if line.startswith("fpr:"):
            fpr = line.split(":")[9]
            break
    assert fpr
    _gpg("--quick-add-key", fpr, "ed25519", "sign", "2y")
    _gpg("--quick-add-key", fpr, "cv25519", "encr", "2y")
    return fpr


def _strip_comments(text):
    return "\n".join(
        ln for ln in text.splitlines() if ln and not ln.startswith("#")
    )


def test_colon_field15_local_and_offline_primary():
    local = _strip_comments((FIXTURES / "colon_ssb_local.txt").read_text())
    hopg.reject_subkey_stub_or_card(local)
    offline = _strip_comments(
        (FIXTURES / "colon_offline_primary.txt").read_text()
    )
    hopg.reject_subkey_stub_or_card(offline)


def test_colon_field15_stub_and_card():
    stub = _strip_comments((FIXTURES / "colon_ssb_stub.txt").read_text())
    with pytest.raises(RuntimeError, match="stub"):
        hopg.reject_subkey_stub_or_card(stub)
    card = _strip_comments((FIXTURES / "colon_ssb_card.txt").read_text())
    with pytest.raises(RuntimeError, match="card"):
        hopg.reject_subkey_stub_or_card(card)


def test_wrap_uses_no_temp_secret_files(gnupg_home, tmp_path, monkeypatch):
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    fpr = _make_radio_key(gnupg_home)
    # Force tempfile into an empty dir so new files are attributable.
    tdir = tmp_path / "wraptmp"
    tdir.mkdir()
    monkeypatch.setenv("TMPDIR", str(tdir))
    before = set(tdir.iterdir())
    ct = hopg.wrap_identity_openpgp(b"\x00" * 164, fpr)
    after = set(tdir.iterdir())
    assert ct
    assert after == before


def test_new_radio_key_requires_yes(gnupg_home, monkeypatch, capsys):
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    fpr = _make_radio_key(gnupg_home)
    # Only primary so far after _make_radio_key already added subkeys —
    # generate a fresh primary-only key for this test.
    home2 = gnupg_home.parent / "gnupg2"
    home2.mkdir()
    home2.chmod(0o700)
    monkeypatch.setenv("GNUPGHOME", str(home2))
    _gpg("--quick-gen-key",
         "Horse Plan <plan@example.test>", "ed25519", "cert", "2y")
    listing = _gpg("--with-colons", "--list-secret-keys").stdout.decode()
    fpr = next(ln.split(":")[9] for ln in listing.splitlines()
               if ln.startswith("fpr:"))

    class A:
        gpg = fpr
        sq = None
        expire = "2y"
        yes = False

    with pytest.raises(RuntimeError, match="--yes"):
        hopg.cmd_openpgp_new_radio_key(A(), json_mode=False)
    out = capsys.readouterr().out
    assert "ed25519" in out
    assert HORSE_NOTATION_SNIPPET in out or hopg.HORSE_RADIO_NOTATION in out


HORSE_NOTATION_SNIPPET = "horse-radio@"


def test_extract_public_and_secret_roundtrip(gnupg_home, helper_path,
                                             monkeypatch):
    fpr = _make_radio_key(gnupg_home)
    cert = _gpg("--export", fpr).stdout
    sec = _gpg("--export-secret-subkeys", fpr).stdout
    assert cert and sec
    pub = subprocess.run(
        [helper_path, "extract-public", "--allow-unmarked"],
        input=cert, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True,
    )
    pub_j = json.loads(pub.stdout.decode())
    r_fd, w_fd = os.pipe()
    try:
        os.write(w_fd, b"")
        os.close(w_fd)
        w_fd = -1
        sec_p = subprocess.run(
            [helper_path, "extract-secret", "--passphrase-fd", str(r_fd),
             "--allow-unmarked"],
            input=sec, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
            pass_fds=(r_fd,), check=False,
        )
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
    assert sec_p.returncode == 0, sec_p.stderr.decode()[:500]
    sec_j = json.loads(sec_p.stdout.decode())
    assert pub_j["ed25519_pk"] == sec_j["ed25519_pk"]
    assert pub_j["x25519_pk"] == sec_j["x25519_pk"]
    assert pub_j["horse_fingerprint"] == sec_j["horse_fingerprint"]
    # Consistency: store as Horse identity and re-pack.
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    identity = {
        "version": 1,
        "ed25519_pk": sec_j["ed25519_pk"],
        "ed25519_sk": sec_j["ed25519_seed"],
        "x25519_pk": sec_j["x25519_pk"],
        "x25519_sk": sec_j["x25519_sk"],
    }
    packed = hp.pack_identity_binary(identity)
    assert len(packed) == hp.HORSE_ID_PACKED


def test_unmarked_without_allow_fails(gnupg_home, helper_path):
    fpr = _make_radio_key(gnupg_home)
    cert = _gpg("--export", fpr).stdout
    p = subprocess.run(
        [helper_path, "extract-public"],
        input=cert, stdout=subprocess.PIPE, stderr=subprocess.PIPE,
        check=False,
    )
    assert p.returncode != 0
    assert b"no usable marked" in p.stderr or b"marked" in p.stderr


def test_store_version3_roundtrip(gnupg_home, monkeypatch):
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    pytest.importorskip("nacl")
    fpr = _make_radio_key(gnupg_home)
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    ident = hp.generate_identity("opgp3")
    wrapped = hp.wrap_identity_openpgp_store(ident, fpr)
    assert hp.identity_store_version(wrapped) == 3
    opened = hp.unwrap_identity(wrapped)
    assert opened["ed25519_pk"] == ident["ed25519_pk"]
    assert opened["x25519_pk"] == ident["x25519_pk"]


def test_openpgp_encrypt_decrypt_roundtrip(gnupg_home, monkeypatch):
    """gpg encrypt then decrypt of packed identity (store v3 payload)."""
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    pytest.importorskip("nacl")
    fpr = _make_radio_key(gnupg_home)
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    ident = hp.generate_identity("encdec")
    packed = hp.pack_identity_binary(ident)
    ct = hopg.wrap_identity_openpgp(packed, fpr)
    assert ct and ct != packed
    pt = hopg.unwrap_identity_openpgp(ct)
    assert pt == packed
    store = hp.wrap_identity_openpgp_store(ident, fpr)
    opened = hp.unwrap_identity(store)
    assert opened["ed25519_pk"] == ident["ed25519_pk"]
    assert opened["x25519_pk"] == ident["x25519_pk"]
    assert opened["ed25519_sk"] == ident["ed25519_sk"]
    assert opened["x25519_sk"] == ident["x25519_sk"]


def test_openpgp_decrypt_rejects_garbage(gnupg_home, monkeypatch):
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    _make_radio_key(gnupg_home)
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    with pytest.raises(RuntimeError):
        hopg.unwrap_identity_openpgp(b"\x00not-openpgp-ciphertext\x00" * 8)


def test_openpgp_wrap_cli_encrypt_decrypt(gnupg_home, monkeypatch, tmp_path):
    """CLI: generate (v2) -> openpgp-wrap (v3 encrypt) -> unwrap (decrypt)."""
    if not shutil.which("gpg"):
        _skip_or_fail("gpg missing")
    pytest.importorskip("nacl")
    fpr = _make_radio_key(gnupg_home)
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    monkeypatch.setenv("XDG_STATE_HOME", str(tmp_path / "state"))
    err = __import__("io").StringIO()
    from contextlib import redirect_stderr

    with redirect_stderr(err):
        rc_gen = hp.main(["--json", "generate", "wrapcli"])
    assert rc_gen == 0, err.getvalue()
    path = hp.identity_path("wrapcli")
    assert path.is_file()
    assert hp.identity_store_version(path.read_bytes()) == 2

    err2 = __import__("io").StringIO()
    with redirect_stderr(err2):
        rc_wrap = hp.main([
            "--json", "openpgp-wrap", "wrapcli",
            "--recipient", fpr, "--force",
        ])
    assert rc_wrap == 0, err2.getvalue()
    blob = path.read_bytes()
    assert hp.identity_store_version(blob) == 3
    opened = hp.unwrap_identity(blob)
    assert len(opened["ed25519_pk"]) == 64
    assert len(opened["x25519_pk"]) == 64
    # show must work on v3 without Horse passphrase
    err3 = __import__("io").StringIO()
    with redirect_stderr(err3):
        rc_show = hp.main(["--json", "show", "wrapcli"])
    assert rc_show == 0, err3.getvalue()


def test_openpgp_extract_identity_cli(gnupg_home, helper_path, monkeypatch,
                                      tmp_path):
    fpr = _make_radio_key(gnupg_home)
    monkeypatch.setenv("HORSE_PASSPHRASE", "pytest-pass-ok")
    monkeypatch.setenv("HORSE_OPENPGP_HELPER", helper_path)
    monkeypatch.setenv("HORSE_OPENPGP_PASSPHRASE", "")
    import io
    from contextlib import redirect_stderr

    err = io.StringIO()
    with redirect_stderr(err):
        rc = hp.main([
            "--json", "openpgp-extract-identity", "fromgpg",
            "--gpg", fpr, "--allow-unmarked", "--force",
        ])
    assert rc == 0, err.getvalue()
    path = hp.identity_path("fromgpg")
    assert path.is_file()
    # Public-only: secrets must not appear in a second show JSON via stdout
    # check is covered by horse_provision tests; here just unwrap.
    ident = hp.unwrap_identity(path.read_bytes(), "pytest-pass-ok")
    assert len(ident["ed25519_pk"]) == 64
