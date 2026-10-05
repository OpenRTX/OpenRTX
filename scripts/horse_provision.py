#!/usr/bin/env python3
#
# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Horse identity provisioning tool.
#
# Generates Ed25519/X25519 identity keypairs using libsodium (via PyNaCl),
# stores them in the Linux kernel keyring, and can provision them to a radio
# over USB-CDC serial.
#

import argparse
import json
import os
import struct
import subprocess
import sys
import time
from pathlib import Path

nacl = None
serial = None
try:
    import nacl.public
    import nacl.signing
    nacl = True
except ImportError:
    nacl = False

try:
    import serial
    import serial.tools.list_ports
except ImportError:
    serial = None


# Protocol constants (matches firmware expectations).
PROTOCOL_VERSION = 1
MSG_HELLO = 0x01
MSG_HELLO_ACK = 0x02
MSG_SEND_IDENTITY = 0x03
MSG_CONFIRM = 0x04
MSG_ERROR = 0xFF
# Must match horse_crypto.h
HORSE_KDF_VERSION = 1
HORSE_ARGON2ID_OPSLIMIT = 2
HORSE_ARGON2ID_MEMLIMIT = 16384
HORSE_IDENTITY_STORE_VERSION = 2


def keyctl_add(keyring_type, description, payload):
    """Add a key to the kernel keyring using keyctl."""
    try:
        proc = subprocess.run(
            ["keyctl", "add", keyring_type, description, payload],
            check=True,
            capture_output=True,
            text=True,
        )
        return proc.stdout.strip()
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"keyctl add failed: {e.stderr}") from e
    except FileNotFoundError:
        raise RuntimeError("keyctl not found; install keyutils package") from None


def keyctl_read(key_id):
    """Read a key from the kernel keyring."""
    try:
        proc = subprocess.run(
            ["keyctl", "read", key_id],
            check=True,
            capture_output=True,
            text=False,
        )
        return proc.stdout
    except subprocess.CalledProcessError as e:
        raise RuntimeError(f"keyctl read failed: {e.stderr}") from e


def keyctl_list(keyring):
    """List keys in a keyring."""
    try:
        proc = subprocess.run(
            ["keyctl", "list", keyring],
            check=True,
            capture_output=True,
            text=True,
        )
        return proc.stdout.strip().split("\n") if proc.stdout.strip() else []
    except subprocess.CalledProcessError:
        return []


def ed25519_sk_libsodium(ed25519_pk_hex, ed25519_sk_hex):
    """Return the 64-byte libsodium secret key (seed || public key)."""
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
    """Generate a new Horse identity (Ed25519 + X25519 keypair)."""
    if not nacl:
        raise RuntimeError("PyNaCl is required for generate; pip install pynacl")
    signing_key = nacl.signing.SigningKey.generate()
    verify_key = signing_key.verify_key

    ed25519_pk = bytes(verify_key)
    ed25519_sk = bytes(signing_key) + ed25519_pk

    x25519_sk = nacl.public.PrivateKey.generate()
    x25519_pk = bytes(x25519_sk.public_key)

    identity = {
        "version": 1,
        "label": label,
        "ed25519_pk": ed25519_pk.hex(),
        "ed25519_sk": ed25519_sk.hex(),
        "x25519_pk": x25519_pk.hex(),
        "x25519_sk": x25519_sk.hex(),
    }

    return identity


def pack_identity_binary(identity):
    """Pack identity into binary format matching horse_identity_keys_t."""
    ed25519_pk = bytes.fromhex(identity["ed25519_pk"])
    ed25519_sk = ed25519_sk_libsodium(identity["ed25519_pk"],
                                      identity["ed25519_sk"])
    x25519_pk = bytes.fromhex(identity["x25519_pk"])
    x25519_sk = bytes.fromhex(identity["x25519_sk"])
    if len(x25519_pk) != 32 or len(x25519_sk) != 32:
        raise ValueError("x25519 keys must be 32 bytes")

    packed = struct.pack(
        "<B3s32s64s32s32s",
        1,  # version
        b"\x00\x00\x00",  # reserved
        ed25519_pk,
        ed25519_sk,
        x25519_pk,
        x25519_sk,
    )
    if len(packed) != 164:
        raise ValueError("identity blob size mismatch")
    return packed


def _decode_keyring_payload(raw):
    if raw.startswith(b"{") or raw.startswith(b"["):
        return json.loads(raw.decode("utf-8"))
    text = raw.decode("ascii", errors="strict").strip()
    decoded = bytes.fromhex(text)
    return json.loads(decoded.decode("utf-8"))


def store_in_keyring(identity, keyring="user"):
    """Store identity in kernel keyring as JSON."""
    label = identity["label"]
    description = f"openrtx:horse:{label}"
    payload = json.dumps(identity)

    key_id = keyctl_add(keyring, description, payload)
    print(f"Stored identity '{label}' in keyring as key ID {key_id}")
    return key_id


def load_from_keyring(label, keyring="user"):
    """Load identity from kernel keyring."""
    description = f"openrtx:horse:{label}"
    keyring_id = f"@{keyring}"

    keys = keyctl_list(keyring_id)
    for key_line in keys:
        if description in key_line:
            key_id = key_line.split(":")[0]
            return _decode_keyring_payload(keyctl_read(key_id))

    raise ValueError(f"Identity '{label}' not found in keyring")


def list_identities(keyring="user"):
    """List all Horse identities in keyring."""
    keyring_id = f"@{keyring}"
    keys = keyctl_list(keyring_id)
    identities = []

    for key_line in keys:
        if "openrtx:horse:" in key_line:
            parts = key_line.split(":")
            if len(parts) >= 3:
                label = ":".join(parts[2:])
                identities.append(label)

    return identities


def send_provisioning_message(ser, msg_type, payload=b""):
    """Send a provisioning message over serial."""
    header = struct.pack("<BBH", PROTOCOL_VERSION, msg_type, len(payload))
    ser.write(header)
    if payload:
        ser.write(payload)
    ser.flush()


def recv_provisioning_message(ser, timeout=5.0):
    """Receive a provisioning message from serial."""
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
    """Provision identity to radio over USB-CDC or a Linux FIFO."""
    if fifo:
        if serial is None:
            print("Error: pyserial required", file=sys.stderr)
            return False
        print(f"Connecting to FIFO {fifo}...")
        try:
            ser = serial.Serial(fifo, baudrate, timeout=1.0)
        except serial.SerialException as e:
            print(f"Error opening FIFO: {e}", file=sys.stderr)
            return False
    else:
        print(f"Connecting to {port} at {baudrate} baud...")
        try:
            ser = serial.Serial(port, baudrate, timeout=1.0)
        except serial.SerialException as e:
            print(f"Error opening serial port: {e}", file=sys.stderr)
            return False

    try:
        print("Sending HELLO...")
        send_provisioning_message(ser, MSG_HELLO)

        msg_type, payload = recv_provisioning_message(ser, timeout=2.0)
        if msg_type != MSG_HELLO_ACK:
            print(f"Error: Radio did not respond with HELLO_ACK (got {msg_type})", file=sys.stderr)
            return False

        print("Radio acknowledged. Sending identity...")
        identity_bin = pack_identity_binary(identity)
        send_provisioning_message(ser, MSG_SEND_IDENTITY, identity_bin)

        msg_type, payload = recv_provisioning_message(ser, timeout=5.0)
        if msg_type == MSG_CONFIRM:
            print("Radio confirmed identity provisioning.")
            if payload:
                fingerprint = payload[:32].hex() if len(payload) >= 32 else payload.hex()
                print(f"Radio fingerprint: {fingerprint}")
            return True
        elif msg_type == MSG_ERROR:
            error_msg = payload.decode("utf-8", errors="ignore") if payload else "unknown error"
            print(f"Radio reported error: {error_msg}", file=sys.stderr)
            return False
        else:
            print(f"Error: Unexpected response from radio (got {msg_type})", file=sys.stderr)
            return False

    finally:
        ser.close()


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
    json_bytes = json.dumps({"version": 1, "label": "t"}).encode("utf-8")
    if _decode_keyring_payload(json_bytes)["label"] != "t":
        raise RuntimeError("json keyring decode")
    if _decode_keyring_payload(json_bytes.hex().encode("ascii"))["label"] != "t":
        raise RuntimeError("hex keyring decode")
    print("horse_provision.py: selftest passed")
    return 0


def main():
    parser = argparse.ArgumentParser(
        description="Horse identity key management and provisioning tool"
    )
    parser.add_argument(
        "--keyring",
        default="user",
        help="Kernel keyring to use (default: user)",
    )

    subparsers = parser.add_subparsers(dest="command", help="Command to execute")

    subparsers.add_parser("selftest", help="Check Ed25519 packing and keyring decode")

    gen_parser = subparsers.add_parser("generate", help="Generate a new identity")
    gen_parser.add_argument("label", help="Label/name for the identity (e.g., callsign)")

    list_parser = subparsers.add_parser("list", help="List stored identities")

    show_parser = subparsers.add_parser("show", help="Show identity details")
    show_parser.add_argument("label", help="Label of identity to show")

    provision_parser = subparsers.add_parser("provision", help="Provision identity to radio")
    provision_parser.add_argument("label", help="Label of identity to provision")
    provision_parser.add_argument(
        "--fifo",
        help="Linux FIFO path for emulator provisioning (default: /tmp/openrtx_horse_prov.fifo)",
    )
    provision_parser.add_argument(
        "--port",
        help="Serial port (e.g., /dev/ttyACM0). If not specified, will attempt to auto-detect",
    )
    provision_parser.add_argument(
        "--baudrate", type=int, default=115200, help="Serial baudrate (default: 115200)"
    )

    args = parser.parse_args()

    if not args.command:
        parser.print_help()
        return 1

    try:
        if args.command == "selftest":
            return selftest()

        if args.command == "generate":
            identity = generate_identity(args.label)
            key_id = store_in_keyring(identity, args.keyring)
            print(f"\nGenerated identity '{args.label}':")
            print(f"  Ed25519 public key: {identity['ed25519_pk']}")
            print(f"  X25519 public key:  {identity['x25519_pk']}")
            print(f"  Stored in keyring as key ID: {key_id}")

        elif args.command == "list":
            identities = list_identities(args.keyring)
            if identities:
                print("Stored Horse identities:")
                for label in identities:
                    print(f"  - {label}")
            else:
                print("No identities found in keyring.")

        elif args.command == "show":
            identity = load_from_keyring(args.label, args.keyring)
            print(f"Identity '{args.label}':")
            print(f"  Ed25519 public key: {identity['ed25519_pk']}")
            print(f"  X25519 public key:  {identity['x25519_pk']}")
            print("  Secret keys: not printed")

        elif args.command == "provision":
            identity = load_from_keyring(args.label, args.keyring)

            port = args.port
            fifo = args.fifo
            if not port and not fifo:
                ports = serial.tools.list_ports.comports()
                if not ports:
                    print("Error: No serial ports found. Specify --port or --fifo.",
                          file=sys.stderr)
                    return 1
                port = ports[0].device
                print(f"Auto-detected port: {port}")

            if not provision_to_radio(identity, port=port, fifo=fifo,
                                      baudrate=args.baudrate):
                return 1

        return 0

    except (ValueError, RuntimeError, KeyError) as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
