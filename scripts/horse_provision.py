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

import argparse
import os
import struct
import sys
from pathlib import Path

have_nacl = False
serial = None
try:
    import nacl.bindings
    import nacl.pwhash
    import nacl.public
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
HORSE_STORE_MAGIC = 0x484B5354
HORSE_KDF_SALT_BYTES = 16
HORSE_STORE_BLOB_MAX = 256
HORSE_PEER_MAX = 64
HORSE_PEER_BYTES = 70


def state_dir():
    env = os.environ.get("XDG_STATE_HOME")
    if env:
        return Path(env) / "OpenRTX"
    home = os.environ.get("HOME")
    if home:
        return Path(home) / ".local/state/OpenRTX"
    raise RuntimeError("neither XDG_STATE_HOME nor HOME is set")


def identity_path(label=None):
    d = state_dir()
    if label is None or label == "":
        return d / "horse_identity.bin"
    return d / ("horse_identity_%s.bin" % label)


def peers_path():
    return state_dir() / "horse_peers.bin"


def ed25519_sk_libsodium(ed25519_pk_hex, ed25519_sk_hex):
    pk = bytes.fromhex(ed25519_pk_hex)
    sk = bytes.fromhex(ed25519_sk_hex)
    if len(pk) != 32:
        raise ValueError("ed25519_pk must be 32 bytes")
    if len(sk) == 32:
        return sk + pk
    if len(sk) == 64:
        if sk[32:] != pk:
            raise ValueError("ed25519_sk[32:] must equal ed25519_pk")
        return sk
    raise ValueError("ed25519_sk must be 32-byte seed or 64-byte libsodium key")


def generate_identity(label):
    if not have_nacl:
        raise RuntimeError("PyNaCl is required for generate; pip install pynacl")
    signing_key = nacl.signing.SigningKey.generate()
    verify_key = signing_key.verify_key
    ed25519_pk = bytes(verify_key)
    ed25519_sk = bytes(signing_key) + ed25519_pk
    x25519_sk = nacl.public.PrivateKey.generate()
    x25519_pk = bytes(x25519_sk.public_key)
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
    if len(x25519_pk) != 32 or len(x25519_sk) != 32:
        raise ValueError("x25519 keys must be 32 bytes")
    packed = struct.pack(
        "<B3s32s64s32s32s",
        1,
        b"\x00\x00\x00",
        ed25519_pk,
        ed25519_sk,
        x25519_pk,
        x25519_sk,
    )
    if len(packed) != 164:
        raise ValueError("identity blob size mismatch")
    return packed


def pack_peer(address, x25519_pk, ed25519_pk):
    if len(address) != 6 or len(x25519_pk) != 32 or len(ed25519_pk) != 32:
        raise ValueError("peer fields must be 6+32+32 bytes")
    packed = address + x25519_pk + ed25519_pk
    if len(packed) != HORSE_PEER_BYTES:
        raise ValueError("peer size mismatch")
    return packed


def wrap_identity(identity, passphrase):
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
    nonce = nacl.utils.random(nacl.bindings.crypto_aead_xchacha20poly1305_ietf_NPUBBYTES)
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
    if not have_nacl:
        raise RuntimeError("PyNaCl is required to open an identity")
    if len(data) < 34:
        raise ValueError("identity file too short")
    magic, ver, kdf, _res, ops, mem, salt, blen = struct.unpack_from(
        "<IBB2sII16sH", data)
    if (magic != HORSE_STORE_MAGIC or ver != HORSE_IDENTITY_STORE_VERSION or
            kdf != HORSE_KDF_VERSION or ops != HORSE_ARGON2ID_OPSLIMIT or
            mem != HORSE_ARGON2ID_MEMLIMIT):
        raise ValueError("unsupported identity blob header")
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
    if len(packed) != 164:
        raise ValueError("identity plaintext size")
    _ver, _res2, ed_pk, ed_sk, x_pk, x_sk = struct.unpack("<B3s32s64s32s32s", packed)
    return {
        "version": _ver,
        "ed25519_pk": ed_pk.hex(),
        "ed25519_sk": ed_sk.hex(),
        "x25519_pk": x_pk.hex(),
        "x25519_sk": x_sk.hex(),
    }


def store_identity_file(identity, passphrase, label=None):
    path = identity_path(label)
    path.parent.mkdir(parents=True, exist_ok=True)
    data = wrap_identity(identity, passphrase)
    fd = os.open(str(path), os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        os.write(fd, data)
        os.fchmod(fd, 0o600)
    finally:
        os.close(fd)
    return path


def list_identity_files():
    d = state_dir()
    if not d.is_dir():
        return []
    return sorted(p.name for p in d.glob("horse_identity*.bin"))


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


def provision_to_radio(identity, port=None, fifo=None, baudrate=115200):
    if fifo:
        if serial is None:
            print("Error: pyserial required", file=sys.stderr)
            return False
        print("Connecting to FIFO %s..." % fifo)
        try:
            ser = serial.Serial(fifo, baudrate, timeout=1.0)
        except serial.SerialException as e:
            print("Error opening FIFO: %s" % e, file=sys.stderr)
            return False
    else:
        print("Connecting to %s at %s baud..." % (port, baudrate))
        try:
            ser = serial.Serial(port, baudrate, timeout=1.0)
        except serial.SerialException as e:
            print("Error opening serial port: %s" % e, file=sys.stderr)
            return False
    try:
        print("Sending HELLO...")
        send_provisioning_message(ser, MSG_HELLO)
        msg_type, payload = recv_provisioning_message(ser, timeout=2.0)
        if msg_type != MSG_HELLO_ACK:
            print("Error: Radio did not respond with HELLO_ACK (got %s)" %
                  msg_type, file=sys.stderr)
            return False
        print("Radio acknowledged. Sending identity...")
        send_provisioning_message(ser, MSG_SEND_IDENTITY,
                                  pack_identity_binary(identity))
        msg_type, payload = recv_provisioning_message(ser, timeout=5.0)
        if msg_type == MSG_CONFIRM:
            print("Radio confirmed identity provisioning.")
            if payload:
                fingerprint = payload[:32].hex() if len(payload) >= 32 else payload.hex()
                print("Radio fingerprint: %s" % fingerprint)
            return True
        if msg_type == MSG_ERROR:
            error_msg = payload.decode("utf-8", errors="ignore") if payload else "unknown"
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
    path.parent.mkdir(parents=True, exist_ok=True)
    data = bytearray(HORSE_PEER_MAX * HORSE_PEER_BYTES)
    if path.is_file():
        existing = path.read_bytes()
        data[:len(existing)] = existing[:len(data)]
    off = (index - 1) * HORSE_PEER_BYTES
    data[off:off + HORSE_PEER_BYTES] = rec
    fd = os.open(str(path), os.O_WRONLY | os.O_CREAT | os.O_TRUNC, 0o600)
    try:
        os.write(fd, bytes(data))
        os.fchmod(fd, 0o600)
    finally:
        os.close(fd)
    return rec


def selftest():
    pk = bytes(range(32))
    seed = bytes(range(32, 64))
    ident32 = {
        "ed25519_pk": pk.hex(),
        "ed25519_sk": seed.hex(),
        "x25519_pk": bytes(range(64, 96)).hex(),
        "x25519_sk": bytes(range(96, 128)).hex(),
    }
    packed = pack_identity_binary(ident32)
    if len(packed) != 164:
        raise RuntimeError("packed size")
    sk = packed[36:100]
    if sk != seed + pk:
        raise RuntimeError("32-byte seed was not expanded to seed||pk")
    ident64 = dict(ident32)
    ident64["ed25519_sk"] = (seed + pk).hex()
    if pack_identity_binary(ident64) != packed:
        raise RuntimeError("64-byte sk pack mismatch")
    if HORSE_KDF_VERSION != 1 or HORSE_IDENTITY_STORE_VERSION != 2:
        raise RuntimeError("kdf header version")
    if HORSE_ARGON2ID_OPSLIMIT != 2 or HORSE_ARGON2ID_MEMLIMIT != 16384:
        raise RuntimeError("argon2id params drifted from firmware")
    peer = pack_peer(b"ABCDEF", pk, bytes(range(32, 64)))
    if len(peer) != HORSE_PEER_BYTES:
        raise RuntimeError("peer size")
    header = struct.pack(
        "<IBB2sII16sH",
        HORSE_STORE_MAGIC,
        HORSE_IDENTITY_STORE_VERSION,
        HORSE_KDF_VERSION,
        b"\x00\x00",
        HORSE_ARGON2ID_OPSLIMIT,
        HORSE_ARGON2ID_MEMLIMIT,
        bytes(16),
        1,
    )
    if len(header) != 34:
        raise RuntimeError("store header size")
    if have_nacl:
        wrapped = wrap_identity(ident32, "test-pass")
        opened = unwrap_identity(wrapped, "test-pass")
        if opened["ed25519_pk"] != ident32["ed25519_pk"]:
            raise RuntimeError("wrap round-trip")
        if "ed25519_sk" in opened and False:
            raise RuntimeError("unreachable")
    print("horse_provision.py: selftest passed")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Horse identity files and radio provisioning"
    )
    parser.add_argument("--passphrase", default="",
                        help="Passphrase for identity files (never printed)")
    sub = parser.add_subparsers(dest="command")
    sub.add_parser("selftest")
    gen = sub.add_parser("generate")
    gen.add_argument("label")
    sub.add_parser("list")
    show = sub.add_parser("show")
    show.add_argument("label")
    prov = sub.add_parser("provision")
    prov.add_argument("label")
    prov.add_argument("--fifo")
    prov.add_argument("--port")
    prov.add_argument("--baudrate", type=int, default=115200)
    exp = sub.add_parser("export-peer")
    exp.add_argument("index", type=int)
    exp.add_argument("--address", required=True, help="12 hex chars (6 bytes)")
    exp.add_argument("--x25519", required=True, help="64 hex chars")
    exp.add_argument("--ed25519", required=True, help="64 hex chars")
    args = parser.parse_args()
    if not args.command:
        parser.print_help()
        return 1
    try:
        if args.command == "selftest":
            return selftest()
        if args.command == "generate":
            if not args.passphrase:
                raise RuntimeError("passphrase required")
            identity = generate_identity(args.label)
            path = store_identity_file(identity, args.passphrase, args.label)
            print("Generated identity '%s'" % args.label)
            print("  Ed25519 public key: %s" % identity["ed25519_pk"])
            print("  X25519 public key:  %s" % identity["x25519_pk"])
            print("  Stored at %s" % path)
            return 0
        if args.command == "list":
            names = list_identity_files()
            if names:
                print("Stored Horse identity files:")
                for name in names:
                    print("  - %s" % name)
            else:
                print("No identity files found.")
            return 0
        if args.command == "show":
            if not args.passphrase:
                raise RuntimeError("passphrase required")
            path = identity_path(args.label)
            ident = unwrap_identity(path.read_bytes(), args.passphrase)
            print("Identity '%s':" % args.label)
            print("  Ed25519 public key: %s" % ident["ed25519_pk"])
            print("  X25519 public key:  %s" % ident["x25519_pk"])
            return 0
        if args.command == "export-peer":
            rec = export_peer_record(
                args.index,
                bytes.fromhex(args.address),
                bytes.fromhex(args.x25519),
                bytes.fromhex(args.ed25519),
            )
            print("Wrote %u-byte peer record at index %u" %
                  (len(rec), args.index))
            return 0
        if args.command == "provision":
            if not args.passphrase:
                raise RuntimeError("passphrase required")
            path = identity_path(args.label)
            ident = unwrap_identity(path.read_bytes(), args.passphrase)
            port = args.port
            fifo = args.fifo
            if not port and not fifo:
                if serial is None:
                    raise RuntimeError("specify --port or --fifo")
                ports = serial.tools.list_ports.comports()
                if not ports:
                    raise RuntimeError("No serial ports found")
                port = ports[0].device
            if not provision_to_radio(ident, port=port, fifo=fifo,
                                      baudrate=args.baudrate):
                return 1
            return 0
    except (ValueError, RuntimeError, KeyError, OSError) as e:
        print("Error: %s" % e, file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
