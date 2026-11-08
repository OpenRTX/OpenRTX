#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Horse identity provisioning tool.
#
# Generates Ed25519/X25519 identity keypairs using libsodium (via PyNaCl)
# and stores them as passphrase-encrypted files under
# XDG_STATE_HOME/OpenRTX/ (mode 0600), matching the radio blob format.
#
# Passphrase: TTY getpass, --passphrase-fd N, or HORSE_PASSPHRASE (tests).
# Never put the passphrase on argv in normal use. --passphrase is deprecated.
#

import argparse
import getpass
import hashlib
import json
import os
import re
import socket
import struct
import sys
import tempfile
from pathlib import Path

# Path bootstrap for horse_openpgp (same directory).
_scripts_dir_early = str(Path(__file__).resolve().parent)
if _scripts_dir_early not in sys.path:
    sys.path.insert(0, _scripts_dir_early)

have_nacl = False
serial = None
try:
    import nacl.bindings
    import nacl.public
    import nacl.pwhash
    import nacl.signing
    import nacl.utils
    have_nacl = True
except ImportError:
    have_nacl = False

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None

try:
    from horse_openpgp import (  # type: ignore
        cmd_openpgp_export,
        cmd_openpgp_extract_identity,
        cmd_openpgp_import_peer,
        cmd_openpgp_new_radio_key,
        openpgp_available,
    )
except ImportError:
    openpgp_available = lambda: False

    def cmd_openpgp_new_radio_key(*_a, **_k):
        raise RuntimeError("OpenPGP support unavailable (install gpg/sq)")

    def cmd_openpgp_extract_identity(*_a, **_k):
        raise RuntimeError("OpenPGP support unavailable (install gpg/sq)")

    def cmd_openpgp_import_peer(*_a, **_k):
        raise RuntimeError("OpenPGP support unavailable (install gpg/sq)")

    def cmd_openpgp_export(*_a, **_k):
        raise RuntimeError("OpenPGP support unavailable (install gpg/sq)")


PROTOCOL_VERSION = 1
MSG_HELLO = 0x01
MSG_HELLO_ACK = 0x02
MSG_SEND_IDENTITY = 0x03
MSG_CONFIRM = 0x04
MSG_ERROR = 0xFF
HORSE_KDF_VERSION = 1
HORSE_ARGON2ID_OPSLIMIT = 2
HORSE_ARGON2ID_MEMLIMIT = 16384
HORSE_IDENTITY_STORE_VERSION = 2
HORSE_IDENTITY_STORE_VERSION_OPENPGP = 3
HORSE_STORE_MAGIC = 0x484B5354
HORSE_KDF_SALT_BYTES = 16
HORSE_STORE_BLOB_MAX = 256
HORSE_PASSPHRASE_MIN = 8
HORSE_PASSPHRASE_MAX = 32
HORSE_PEER_MAX = 64
HORSE_PEER_BYTES = 70
HORSE_ID_PACKED = 164
LABEL_RE = re.compile(r"^[A-Za-z0-9._-]+$")
DEFAULT_SOCK = "/tmp/openrtx_horse_prov.sock"

# X25519 low-order / reject points from RFC 7748 section 6.1 (public).
_X25519_BAD_PK = {
    bytes(32),
    bytes.fromhex(
        "0100000000000000000000000000000000000000000000000000000000000000"
    ),
    bytes.fromhex(
        "e0eb7a7c3b41b8ae1656e3faf19fc46ada098deb9c32b1fd866205165f49b800"
    ),
    bytes.fromhex(
        "5f9c95bca3508c24b1d0b1559c83ef5b04445cc4581c8e86d8224eddddf9b4c5"
    ),
    bytes.fromhex(
        "cdeb7a7c3b41b8ae1656e3faf19fc46ada098deb9c32b1fd866205165f49b880"
    ),
    bytes.fromhex(
        "ecffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"
    ),
    bytes.fromhex(
        "edffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"
    ),
    bytes.fromhex(
        "eeffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff7f"
    ),
}


def state_dir():
    env = os.environ.get("XDG_STATE_HOME")
    if env:
        return Path(env) / "OpenRTX"
    home = os.environ.get("HOME")
    if home:
        return Path(home) / ".local/state/OpenRTX"
    raise RuntimeError("neither XDG_STATE_HOME nor HOME is set")


def sanitize_label(label):
    if label is None or label == "":
        raise ValueError("label is required")
    if ".." in label or "/" in label or "\\" in label:
        raise ValueError("label must not contain path separators or ..")
    if not LABEL_RE.match(label):
        raise ValueError("label must match [A-Za-z0-9._-]+")
    return label


def identity_path(label=None):
    d = state_dir()
    if label is None or label == "":
        return d / "horse_identity.bin"
    return d / ("horse_identity_%s.bin" % sanitize_label(label))


def peers_path():
    return state_dir() / "horse_peers.bin"


def passphrase_ok(passphrase):
    if passphrase is None:
        return False
    n = len(passphrase.encode("utf-8"))
    return HORSE_PASSPHRASE_MIN <= n <= HORSE_PASSPHRASE_MAX


def resolve_passphrase(args):
    """TTY getpass, --passphrase-fd, HORSE_PASSPHRASE, or deprecated flag."""
    if getattr(args, "passphrase_fd", None) is not None:
        fd = args.passphrase_fd
        data = os.read(fd, HORSE_PASSPHRASE_MAX + 8)
        text = data.decode("utf-8", errors="strict").rstrip("\n\r")
        if not passphrase_ok(text):
            raise RuntimeError(
                "passphrase must be %u to %u bytes" %
                (HORSE_PASSPHRASE_MIN, HORSE_PASSPHRASE_MAX)
            )
        return text
    env = os.environ.get("HORSE_PASSPHRASE")
    if env is not None and env != "":
        if not passphrase_ok(env):
            raise RuntimeError(
                "passphrase must be %u to %u bytes" %
                (HORSE_PASSPHRASE_MIN, HORSE_PASSPHRASE_MAX)
            )
        return env
    deprecated = getattr(args, "passphrase", None)
    if deprecated is not None and deprecated != "":
        print(
            "warning: --passphrase puts the secret in argv; use "
            "--passphrase-fd, HORSE_PASSPHRASE, or a TTY prompt",
            file=sys.stderr,
        )
        if not passphrase_ok(deprecated):
            raise RuntimeError(
                "passphrase must be %u to %u bytes" %
                (HORSE_PASSPHRASE_MIN, HORSE_PASSPHRASE_MAX)
            )
        return deprecated
    if sys.stdin.isatty():
        text = getpass.getpass("Horse passphrase: ")
        if not passphrase_ok(text):
            raise RuntimeError(
                "passphrase must be %u to %u bytes" %
                (HORSE_PASSPHRASE_MIN, HORSE_PASSPHRASE_MAX)
            )
        return text
    raise RuntimeError(
        "passphrase required via TTY, --passphrase-fd, or HORSE_PASSPHRASE"
    )


def ed25519_sk_libsodium(ed25519_pk_hex, ed25519_sk_hex):
    if not have_nacl:
        raise RuntimeError("PyNaCl is required")
    pk = bytes.fromhex(ed25519_pk_hex)
    sk = bytes.fromhex(ed25519_sk_hex)
    if len(pk) != 32:
        raise ValueError("ed25519_pk must be 32 bytes")
    if len(sk) == 32:
        derived = bytes(nacl.signing.SigningKey(sk).verify_key)
        if derived != pk:
            raise ValueError("ed25519 seed does not match ed25519_pk")
        return sk + pk
    if len(sk) == 64:
        derived = bytes(nacl.signing.SigningKey(sk[:32]).verify_key)
        if sk[32:] != pk or derived != pk:
            raise ValueError("ed25519_sk[32:] must equal key from seed")
        return sk
    raise ValueError("ed25519_sk must be 32-byte seed or 64-byte libsodium key")


def validate_x25519_pair(pk, sk):
    if not have_nacl:
        raise RuntimeError("PyNaCl is required")
    if len(pk) != 32 or len(sk) != 32:
        raise ValueError("x25519 keys must be 32 bytes")
    if pk in _X25519_BAD_PK or not any(pk):
        raise ValueError("x25519 public key is low-order or zero")
    derived = bytes(nacl.public.PrivateKey(sk).public_key)
    if derived != pk:
        raise ValueError("x25519_sk does not match x25519_pk")
    # Shared secret with self must be non-zero for a usable key.
    shared = nacl.bindings.crypto_scalarmult(sk, pk)
    if not any(shared):
        raise ValueError("x25519 shared secret is all-zero")


def validate_ed25519_pk(pk):
    if not have_nacl:
        raise RuntimeError("PyNaCl is required")
    if len(pk) != 32:
        raise ValueError("ed25519_pk must be 32 bytes")
    if not any(pk):
        raise ValueError("ed25519 public key is all-zero")
    # Reject clearly invalid encodings via verify of a dummy (raises) or
    # crypto_core_ed25519_is_valid_point when bindings expose it.
    is_valid = getattr(nacl.bindings, "crypto_core_ed25519_is_valid_point", None)
    if is_valid is not None:
        if not is_valid(pk):
            raise ValueError("ed25519 public key is not a valid point")
    else:
        try:
            nacl.signing.VerifyKey(pk)
        except Exception as e:
            raise ValueError("ed25519 public key rejected: %s" % e) from e


def validate_peer_keys(address, x25519_pk, ed25519_pk):
    if len(address) != 6:
        raise ValueError("address must be 6 bytes")
    if not any(address):
        raise ValueError("peer address must not be all-zero")
    if x25519_pk in _X25519_BAD_PK or not any(x25519_pk):
        raise ValueError("peer x25519 public key is low-order or zero")
    validate_ed25519_pk(ed25519_pk)
    if have_nacl:
        # Probe shared secret with an ephemeral scalar; all-zero => reject.
        eph = nacl.public.PrivateKey.generate()
        try:
            shared = nacl.bindings.crypto_scalarmult(bytes(eph), x25519_pk)
        except Exception as e:
            raise ValueError("peer x25519 public key rejected: %s" % e) from e
        if not any(shared):
            raise ValueError("peer x25519 yields all-zero shared secret")


def identity_fingerprint(ed25519_pk, x25519_pk):
    """BLAKE2b-256 keyed HORSE-IDFP over ed25519_pk || x25519_pk.

    Matches libsodium crypto_generichash / firmware horse_crypto_identity_fingerprint.
    """
    if isinstance(ed25519_pk, str):
        ed25519_pk = bytes.fromhex(ed25519_pk)
    if isinstance(x25519_pk, str):
        x25519_pk = bytes.fromhex(x25519_pk)
    if len(ed25519_pk) != 32 or len(x25519_pk) != 32:
        raise ValueError("fingerprint needs 32-byte public keys")
    return hashlib.blake2b(
        ed25519_pk + x25519_pk, key=b"HORSE-IDFP", digest_size=32
    ).digest()


def generate_identity(label):
    if not have_nacl:
        raise RuntimeError("PyNaCl is required for generate; pip install pynacl")
    sanitize_label(label)
    signing_key = nacl.signing.SigningKey.generate()
    verify_key = signing_key.verify_key
    ed25519_pk = bytes(verify_key)
    ed25519_sk = bytes(signing_key) + ed25519_pk
    x25519_sk = nacl.public.PrivateKey.generate()
    x25519_pk = bytes(x25519_sk.public_key)
    validate_x25519_pair(x25519_pk, bytes(x25519_sk))
    return {
        "version": 1,
        "label": label,
        "ed25519_pk": ed25519_pk.hex(),
        "ed25519_sk": ed25519_sk.hex(),
        "x25519_pk": x25519_pk.hex(),
        "x25519_sk": bytes(x25519_sk).hex(),
    }


def pack_identity_binary(identity):
    ed25519_pk = bytes.fromhex(identity["ed25519_pk"])
    ed25519_sk = ed25519_sk_libsodium(identity["ed25519_pk"],
                                      identity["ed25519_sk"])
    x25519_pk = bytes.fromhex(identity["x25519_pk"])
    x25519_sk = bytes.fromhex(identity["x25519_sk"])
    validate_x25519_pair(x25519_pk, x25519_sk)
    validate_ed25519_pk(ed25519_pk)
    packed = struct.pack(
        "<B3s32s64s32s32s",
        1,
        b"\x00\x00\x00",
        ed25519_pk,
        ed25519_sk,
        x25519_pk,
        x25519_sk,
    )
    if len(packed) != HORSE_ID_PACKED:
        raise ValueError("identity blob size mismatch")
    return packed


def pack_peer(address, x25519_pk, ed25519_pk):
    validate_peer_keys(address, x25519_pk, ed25519_pk)
    packed = address + x25519_pk + ed25519_pk
    if len(packed) != HORSE_PEER_BYTES:
        raise ValueError("peer size mismatch")
    return packed


def atomic_write(path, data, mode=0o600):
    """Write via temp file, fsync, replace, fsync directory."""
    path = Path(path)
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, tmp_name = tempfile.mkstemp(dir=str(path.parent), prefix=".horse_tmp_")
    tmp_path = Path(tmp_name)
    try:
        os.fchmod(fd, mode)
        written = 0
        view = memoryview(data)
        while written < len(data):
            n = os.write(fd, view[written:])
            if n <= 0:
                raise OSError("short write")
            written += n
        os.fsync(fd)
    except Exception:
        os.close(fd)
        try:
            tmp_path.unlink()
        except OSError:
            pass
        raise
    else:
        os.close(fd)
    os.replace(str(tmp_path), str(path))
    dir_fd = os.open(str(path.parent), os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(dir_fd)
    finally:
        os.close(dir_fd)
    return path


def wrap_identity(identity, passphrase):
    if not passphrase_ok(passphrase):
        raise RuntimeError(
            "passphrase must be %u to %u bytes" %
            (HORSE_PASSPHRASE_MIN, HORSE_PASSPHRASE_MAX)
        )
    if not have_nacl:
        raise RuntimeError("PyNaCl is required to encrypt an identity")
    packed = pack_identity_binary(identity)
    salt = nacl.utils.random(HORSE_KDF_SALT_BYTES)
    key = nacl.pwhash.argon2id.kdf(
        32,
        passphrase.encode("utf-8"),
        salt,
        opslimit=HORSE_ARGON2ID_OPSLIMIT,
        memlimit=HORSE_ARGON2ID_MEMLIMIT,
    )
    nonce = nacl.utils.random(
        nacl.bindings.crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
    )
    ct = nacl.bindings.crypto_aead_xchacha20poly1305_ietf_encrypt(
        packed, None, nonce, key)
    blob = nonce + ct
    if len(blob) > HORSE_STORE_BLOB_MAX:
        raise RuntimeError("wrapped identity exceeds blob capacity")
    header = struct.pack(
        "<IBB2sII16sH",
        HORSE_STORE_MAGIC,
        HORSE_IDENTITY_STORE_VERSION,
        HORSE_KDF_VERSION,
        b"\x00\x00",
        HORSE_ARGON2ID_OPSLIMIT,
        HORSE_ARGON2ID_MEMLIMIT,
        salt,
        len(blob),
    )
    return header + blob + bytes(HORSE_STORE_BLOB_MAX - len(blob))


def unwrap_identity(data, passphrase):
    if not passphrase_ok(passphrase):
        raise RuntimeError(
            "passphrase must be %u to %u bytes" %
            (HORSE_PASSPHRASE_MIN, HORSE_PASSPHRASE_MAX)
        )
    if not have_nacl:
        raise RuntimeError("PyNaCl is required to open an identity")
    if len(data) < 34:
        raise ValueError("identity file too short")
    magic, ver, kdf, _res, ops, mem, salt, blen = struct.unpack_from(
        "<IBB2sII16sH", data)
    if magic != HORSE_STORE_MAGIC:
        raise ValueError("unsupported identity blob header")
    if ver == HORSE_IDENTITY_STORE_VERSION_OPENPGP:
        raise ValueError(
            "OpenPGP-wrapped identity: use openpgp-extract-identity / gpg"
        )
    if (ver != HORSE_IDENTITY_STORE_VERSION or kdf != HORSE_KDF_VERSION
            or ops != HORSE_ARGON2ID_OPSLIMIT
            or mem != HORSE_ARGON2ID_MEMLIMIT):
        raise ValueError("unsupported identity blob header")
    if blen > HORSE_STORE_BLOB_MAX:
        raise ValueError("blob length exceeds HORSE_STORE_BLOB_MAX")
    if 34 + blen > len(data):
        raise ValueError("blob length exceeds file length")
    blob = data[34:34 + blen]
    npub = nacl.bindings.crypto_aead_xchacha20poly1305_ietf_NPUBBYTES
    if len(blob) <= npub:
        raise ValueError("wrapped blob truncated")
    key = nacl.pwhash.argon2id.kdf(
        32,
        passphrase.encode("utf-8"),
        salt,
        opslimit=HORSE_ARGON2ID_OPSLIMIT,
        memlimit=HORSE_ARGON2ID_MEMLIMIT,
    )
    packed = nacl.bindings.crypto_aead_xchacha20poly1305_ietf_decrypt(
        blob[npub:], None, blob[:npub], key)
    if len(packed) != HORSE_ID_PACKED:
        raise ValueError("identity plaintext size")
    _ver, _res2, ed_pk, ed_sk, x_pk, x_sk = struct.unpack(
        "<B3s32s64s32s32s", packed)
    ed25519_sk_libsodium(ed_pk.hex(), ed_sk.hex())
    validate_x25519_pair(x_pk, x_sk)
    return {
        "version": _ver,
        "ed25519_pk": ed_pk.hex(),
        "ed25519_sk": ed_sk.hex(),
        "x25519_pk": x_pk.hex(),
        "x25519_sk": x_sk.hex(),
    }


def store_identity_file(identity, passphrase, label=None, force=False):
    path = identity_path(label)
    if path.exists() and not force:
        raise RuntimeError(
            "identity file exists (%s); pass --force to overwrite" % path
        )
    data = wrap_identity(identity, passphrase)
    return atomic_write(path, data)


def list_identity_files():
    d = state_dir()
    if not d.is_dir():
        return []
    return sorted(p.name for p in d.glob("horse_identity*.bin"))


class _SockTransport:
    """Minimal file-like duplex over a connected Unix stream socket."""

    def __init__(self, path, timeout=5.0):
        self._sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self._sock.settimeout(timeout)
        self._sock.connect(path)
        self.timeout = timeout

    def write(self, data):
        self._sock.sendall(data)
        return len(data)

    def read(self, n):
        chunks = []
        got = 0
        while got < n:
            try:
                b = self._sock.recv(n - got)
            except TimeoutError:
                break
            if not b:
                break
            chunks.append(b)
            got += len(b)
        return b"".join(chunks)

    def flush(self):
        pass

    def close(self):
        try:
            self._sock.close()
        except OSError:
            pass


def send_provisioning_message(ser, msg_type, payload=b""):
    header = struct.pack("<BBH", PROTOCOL_VERSION, msg_type, len(payload))
    ser.write(header)
    if payload:
        ser.write(payload)
    ser.flush()


def recv_provisioning_message(ser, timeout=5.0):
    ser.timeout = timeout
    header = ser.read(4)
    if len(header) != 4:
        return None, None
    version, msg_type, payload_len = struct.unpack("<BBH", header)
    if version != PROTOCOL_VERSION:
        return None, None
    payload = ser.read(payload_len) if payload_len > 0 else b""
    return msg_type, payload


def provision_to_radio(identity, port=None, sock=None, baudrate=115200,
                       json_mode=False):
    local_fp = identity_fingerprint(identity["ed25519_pk"],
                                    identity["x25519_pk"])
    if sock:
        try:
            ser = _SockTransport(sock, timeout=5.0)
        except OSError as e:
            print("Error opening socket %s: %s" % (sock, e), file=sys.stderr)
            return False
    else:
        if serial is None:
            print("Error: pyserial required for --port", file=sys.stderr)
            return False
        try:
            ser = serial.Serial(port, baudrate, timeout=1.0)
        except serial.SerialException as e:
            print("Error opening serial port: %s" % e, file=sys.stderr)
            return False
    try:
        if not json_mode:
            print("Sending HELLO...")
        send_provisioning_message(ser, MSG_HELLO)
        msg_type, payload = recv_provisioning_message(ser, timeout=2.0)
        if msg_type != MSG_HELLO_ACK:
            print("Error: Radio did not respond with HELLO_ACK (got %s)" %
                  msg_type, file=sys.stderr)
            return False
        send_provisioning_message(ser, MSG_SEND_IDENTITY,
                                  pack_identity_binary(identity))
        msg_type, payload = recv_provisioning_message(ser, timeout=5.0)
        if msg_type == MSG_CONFIRM:
            radio_fp = payload[:32] if payload and len(payload) >= 32 else b""
            if radio_fp != local_fp:
                print(
                    "Error: radio fingerprint mismatch "
                    "(local %s radio %s)" %
                    (local_fp.hex(), radio_fp.hex() if radio_fp else "none"),
                    file=sys.stderr,
                )
                return False
            if json_mode:
                print(json.dumps({
                    "status": "ok",
                    "fingerprint": local_fp.hex(),
                }))
            else:
                print("Radio confirmed identity provisioning.")
                print("Fingerprint: %s" % local_fp.hex())
            return True
        if msg_type == MSG_ERROR:
            error_msg = (payload.decode("utf-8", errors="ignore")
                         if payload else "unknown")
            print("Radio reported error: %s" % error_msg, file=sys.stderr)
            return False
        print("Error: Unexpected response from radio (got %s)" % msg_type,
              file=sys.stderr)
        return False
    finally:
        ser.close()


def export_peer_record(index, address, x25519_pk, ed25519_pk):
    if index < 1 or index > HORSE_PEER_MAX:
        raise ValueError("peer index out of range")
    rec = pack_peer(address, x25519_pk, ed25519_pk)
    path = peers_path()
    data = bytearray(HORSE_PEER_MAX * HORSE_PEER_BYTES)
    if path.is_file():
        existing = path.read_bytes()
        data[:len(existing)] = existing[:len(data)]
    off = (index - 1) * HORSE_PEER_BYTES
    data[off:off + HORSE_PEER_BYTES] = rec
    atomic_write(path, bytes(data))
    return rec


def public_view(identity, label=None):
    fp = identity_fingerprint(identity["ed25519_pk"], identity["x25519_pk"])
    out = {
        "ed25519_pk": identity["ed25519_pk"],
        "x25519_pk": identity["x25519_pk"],
        "fingerprint": fp.hex(),
    }
    if label is not None:
        out["label"] = label
    return out


def emit(obj, json_mode):
    if json_mode:
        print(json.dumps(obj, sort_keys=True))
    else:
        if "label" in obj:
            print("Identity '%s':" % obj["label"])
        if "ed25519_pk" in obj:
            print("  Ed25519 public key: %s" % obj["ed25519_pk"])
        if "x25519_pk" in obj:
            print("  X25519 public key:  %s" % obj["x25519_pk"])
        if "fingerprint" in obj:
            print("  Fingerprint:        %s" % obj["fingerprint"])
        if "path" in obj:
            print("  Stored at %s" % obj["path"])
        if "names" in obj:
            if obj["names"]:
                print("Stored Horse identity files:")
                for name in obj["names"]:
                    print("  - %s" % name)
            else:
                print("No identity files found.")
        if "peer_index" in obj:
            print("Wrote peer record at index %u" % obj["peer_index"])


def selftest():
    require = os.environ.get("HORSE_REQUIRE_CRYPTO") == "1"
    if not have_nacl:
        print("horse_provision.py: skipped (no PyNaCl)", file=sys.stderr)
        return 1 if require else 77

    # Consistent seed||pk pack.
    sk = nacl.signing.SigningKey.generate()
    seed = bytes(sk)
    pk = bytes(sk.verify_key)
    ident32 = {
        "ed25519_pk": pk.hex(),
        "ed25519_sk": seed.hex(),
        "x25519_pk": bytes(nacl.public.PrivateKey.generate().public_key).hex(),
        "x25519_sk": bytes(nacl.public.PrivateKey.generate()).hex(),
    }
    # Fix x25519 pair to be consistent.
    xsk = nacl.public.PrivateKey.generate()
    ident32["x25519_sk"] = bytes(xsk).hex()
    ident32["x25519_pk"] = bytes(xsk.public_key).hex()

    packed = pack_identity_binary(ident32)
    if len(packed) != HORSE_ID_PACKED:
        raise RuntimeError("packed size")
    if packed[36:100] != seed + pk:
        raise RuntimeError("32-byte seed was not expanded to seed||pk")

    bad = dict(ident32)
    bad["ed25519_pk"] = bytes(range(32)).hex()
    try:
        pack_identity_binary(bad)
        raise RuntimeError("mismatched ed25519 seed/pk accepted")
    except ValueError:
        pass

    ident64 = dict(ident32)
    ident64["ed25519_sk"] = (seed + pk).hex()
    if pack_identity_binary(ident64) != packed:
        raise RuntimeError("64-byte sk pack mismatch")

    if HORSE_KDF_VERSION != 1 or HORSE_IDENTITY_STORE_VERSION != 2:
        raise RuntimeError("kdf header version")
    if HORSE_ARGON2ID_OPSLIMIT != 2 or HORSE_ARGON2ID_MEMLIMIT != 16384:
        raise RuntimeError("argon2id params drifted from firmware")

    try:
        wrap_identity(ident32, "")
        raise RuntimeError("empty passphrase accepted")
    except RuntimeError as e:
        if "passphrase" not in str(e).lower():
            raise
    try:
        wrap_identity(ident32, "1234567")
        raise RuntimeError("short passphrase accepted")
    except RuntimeError as e:
        if "passphrase" not in str(e).lower():
            raise

    peer = pack_peer(b"ABCDEF", bytes.fromhex(ident32["x25519_pk"]), pk)
    if len(peer) != HORSE_PEER_BYTES:
        raise RuntimeError("peer size")
    try:
        pack_peer(bytes(6), bytes.fromhex(ident32["x25519_pk"]), pk)
        raise RuntimeError("zero address accepted")
    except ValueError:
        pass

    wrapped = wrap_identity(ident32, "test-pass-ok")
    opened = unwrap_identity(wrapped, "test-pass-ok")
    if opened["ed25519_pk"] != ident32["ed25519_pk"]:
        raise RuntimeError("wrap round-trip")
    fp = identity_fingerprint(ident32["ed25519_pk"], ident32["x25519_pk"])
    if len(fp) != 32:
        raise RuntimeError("fingerprint size")

    print("horse_provision.py: selftest passed")
    return 0


def build_parser():
    parser = argparse.ArgumentParser(
        description="Horse identity files and radio provisioning"
    )
    parser.add_argument(
        "--passphrase",
        default=None,
        help="Deprecated: passphrase on argv (prefer --passphrase-fd)",
    )
    parser.add_argument(
        "--passphrase-fd",
        type=int,
        default=None,
        help="Read passphrase from file descriptor N",
    )
    parser.add_argument(
        "--json",
        action="store_true",
        help="Machine-readable public output only",
    )
    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List serial port candidates and exit",
    )
    sub = parser.add_subparsers(dest="command")
    sub.add_parser("selftest")
    gen = sub.add_parser("generate")
    gen.add_argument("label")
    gen.add_argument("--force", action="store_true")
    sub.add_parser("list")
    show = sub.add_parser("show")
    show.add_argument("label")
    prov = sub.add_parser("provision")
    prov.add_argument("label")
    prov.add_argument("--sock", default=None,
                      help="Unix socket path (Linux emulator)")
    prov.add_argument("--port", default=None, help="Serial device path")
    prov.add_argument("--baudrate", type=int, default=115200)
    exp = sub.add_parser("export-peer")
    exp.add_argument("index", type=int)
    exp.add_argument("--address", required=True, help="12 hex chars (6 bytes)")
    exp.add_argument("--x25519", required=True, help="64 hex chars")
    exp.add_argument("--ed25519", required=True, help="64 hex chars")

    op_new = sub.add_parser("openpgp-new-radio-key")
    op_new.add_argument("--gpg", metavar="KEYID", default=None)
    op_new.add_argument("--sq", metavar="KEYID", default=None)
    op_ext = sub.add_parser("openpgp-extract-identity")
    op_ext.add_argument("label")
    op_ext.add_argument("--gpg", metavar="KEYID", default=None)
    op_ext.add_argument("--sq", metavar="KEYID", default=None)
    op_ext.add_argument("--allow-unmarked", action="store_true")
    op_ext.add_argument("--force", action="store_true")
    op_imp = sub.add_parser("openpgp-import-peer")
    op_imp.add_argument("index", type=int)
    op_imp.add_argument("--file", default=None)
    op_imp.add_argument("--gpg", metavar="KEYID", default=None)
    op_imp.add_argument("--sq", metavar="KEYID", default=None)
    op_imp.add_argument("--address", required=True)
    op_imp.add_argument("--allow-unmarked", action="store_true")
    op_exp = sub.add_parser("openpgp-export")
    op_exp.add_argument("--gpg", metavar="KEYID", default=None)
    op_exp.add_argument("--sq", metavar="KEYID", default=None)
    op_exp.add_argument("-o", "--output", default=None)
    return parser


def strip_meson_catch2_args(argv):
    """Drop Catch2/meson --reporter args so parse_args still rejects typos."""
    out = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--reporter" and i + 1 < len(argv):
            i += 2
            continue
        if a.startswith("--reporter="):
            i += 1
            continue
        out.append(a)
        i += 1
    return out


def main(argv=None):
    if argv is None:
        argv = sys.argv[1:]
    argv = strip_meson_catch2_args(list(argv))
    parser = build_parser()
    args = parser.parse_args(argv)
    if args.list_ports:
        if serial is None:
            print("pyserial not installed", file=sys.stderr)
            return 1
        for p in serial.tools.list_ports.comports():
            print("%s\t%s" % (p.device, p.description))
        return 0
    if not args.command:
        parser.print_help()
        return 1
    json_mode = bool(args.json)
    try:
        if args.command == "selftest":
            return selftest()
        if args.command == "generate":
            passphrase = resolve_passphrase(args)
            identity = generate_identity(args.label)
            path = store_identity_file(
                identity, passphrase, args.label, force=args.force
            )
            view = public_view(identity, args.label)
            view["path"] = str(path)
            emit(view, json_mode)
            return 0
        if args.command == "list":
            names = list_identity_files()
            if json_mode:
                print(json.dumps({"names": names}))
            else:
                emit({"names": names}, False)
            return 0
        if args.command == "show":
            passphrase = resolve_passphrase(args)
            path = identity_path(args.label)
            ident = unwrap_identity(path.read_bytes(), passphrase)
            emit(public_view(ident, args.label), json_mode)
            return 0
        if args.command == "export-peer":
            rec = export_peer_record(
                args.index,
                bytes.fromhex(args.address),
                bytes.fromhex(args.x25519),
                bytes.fromhex(args.ed25519),
            )
            if json_mode:
                print(json.dumps({
                    "peer_index": args.index,
                    "address": args.address,
                    "x25519_pk": args.x25519,
                    "ed25519_pk": args.ed25519,
                    "bytes": len(rec),
                }))
            else:
                emit({"peer_index": args.index}, False)
            return 0
        if args.command == "provision":
            passphrase = resolve_passphrase(args)
            path = identity_path(args.label)
            ident = unwrap_identity(path.read_bytes(), passphrase)
            port = args.port
            sock = args.sock
            if not port and not sock:
                raise RuntimeError(
                    "specify --port or --sock "
                    "(use --list-ports for serial candidates; "
                    "default emulator socket is %s)" % DEFAULT_SOCK
                )
            if sock is None and port == DEFAULT_SOCK:
                sock = port
                port = None
            if not provision_to_radio(
                ident, port=port, sock=sock, baudrate=args.baudrate,
                json_mode=json_mode
            ):
                return 1
            return 0
        if args.command == "openpgp-new-radio-key":
            return cmd_openpgp_new_radio_key(args, json_mode)
        if args.command == "openpgp-extract-identity":
            passphrase = None
            try:
                passphrase = resolve_passphrase(args)
            except RuntimeError:
                passphrase = None
            return cmd_openpgp_extract_identity(
                args, json_mode, passphrase, store_identity_file,
                public_view, emit,
            )
        if args.command == "openpgp-import-peer":
            return cmd_openpgp_import_peer(
                args, json_mode, export_peer_record, emit
            )
        if args.command == "openpgp-export":
            return cmd_openpgp_export(args, json_mode)
    except (ValueError, RuntimeError, KeyError, OSError) as e:
        print("Error: %s" % e, file=sys.stderr)
        return 1
    return 1


if __name__ == "__main__":
    sys.exit(main())
