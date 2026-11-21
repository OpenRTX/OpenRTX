// SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
// SPDX-License-Identifier: GPL-3.0-or-later
//! Horse OpenPGP helper: inspect certificates and extract radio subkeys.
//! Secrets never appear in error messages. Zeroize buffers after use.

use anyhow::{anyhow, bail, Context, Result};
use blake2::digest::{KeyInit, Mac};
use clap::{Parser, Subcommand};
use ed25519_dalek::SigningKey as Ed25519SigningKey;
use sequoia_openpgp::cert::prelude::*;
use sequoia_openpgp::crypto::mpi;
use sequoia_openpgp::crypto::Password;
use sequoia_openpgp::packet::key::SecretKeyMaterial;
use sequoia_openpgp::parse::Parse;
use sequoia_openpgp::policy::{NullPolicy, StandardPolicy};
use sequoia_openpgp::types::KeyFlags;
use sequoia_openpgp::Cert;
use serde::Serialize;
use std::fs::File;
use std::io::{self, Read};
use std::os::fd::FromRawFd;
use std::path::PathBuf;
use std::time::{Duration, SystemTime};
use x25519_dalek::{PublicKey as X25519Public, StaticSecret};
use zeroize::{Zeroize, Zeroizing};

const HORSE_NOTATION: &str = "horse-radio@NOTATION-DOMAIN";
const HORSE_IDFP_KEY: &[u8] = b"HORSE-IDFP";

#[derive(Parser, Debug)]
#[command(name = "horse-openpgp", about = "Horse OpenPGP radio-key helper")]
struct Cli {
    #[command(subcommand)]
    cmd: Cmd,
}

#[derive(Subcommand, Debug)]
enum Cmd {
    InspectCert {
        #[arg(long)]
        allowlist: Option<PathBuf>,
    },
    ExtractPublic {
        #[arg(long)]
        allow_unmarked: bool,
        #[arg(long)]
        allowlist: Option<PathBuf>,
    },
    ExtractSecret {
        #[arg(long)]
        passphrase_fd: i32,
        #[arg(long)]
        allow_unmarked: bool,
        #[arg(long)]
        allowlist: Option<PathBuf>,
    },
}

#[derive(Serialize)]
struct SubkeyInfo {
    fingerprint: String,
    algo: String,
    version: u8,
    can_sign: bool,
    can_encrypt: bool,
    creation_time: String,
    expiration_time: Option<String>,
    horse_marked: bool,
}

#[derive(Serialize)]
struct InspectOut {
    primary_fpr: String,
    subkeys: Vec<SubkeyInfo>,
}

#[derive(Serialize)]
struct PublicOut {
    cert_fpr: String,
    ed25519_pk: String,
    x25519_pk: String,
    horse_fingerprint: String,
    sign_fpr: String,
    encrypt_fpr: String,
}

#[derive(Serialize)]
struct SecretOut {
    cert_fpr: String,
    ed25519_pk: String,
    ed25519_seed: String,
    x25519_pk: String,
    x25519_sk: String,
    horse_fingerprint: String,
}

#[derive(serde::Deserialize, Default)]
struct Allowlist {
    #[serde(default)]
    sign_fps: Vec<String>,
    #[serde(default)]
    encrypt_fps: Vec<String>,
}

fn read_stdin() -> Result<Vec<u8>> {
    let mut buf = Vec::new();
    io::stdin().read_to_end(&mut buf)?;
    Ok(buf)
}

fn read_passphrase_fd(fd: i32) -> Result<Zeroizing<Vec<u8>>> {
    if fd < 0 {
        bail!("invalid passphrase fd");
    }
    let mut f = unsafe { File::from_raw_fd(fd) };
    let mut buf = Zeroizing::new(Vec::new());
    f.read_to_end(&mut buf)?;
    while matches!(buf.last().copied(), Some(b'\n') | Some(b'\r')) {
        buf.pop();
    }
    Ok(buf)
}

fn load_allowlist(path: &Option<PathBuf>) -> Result<Allowlist> {
    match path {
        None => Ok(Allowlist::default()),
        Some(p) => {
            let data = std::fs::read_to_string(p)
                .with_context(|| format!("read allowlist {}", p.display()))?;
            Ok(serde_json::from_str(&data)?)
        }
    }
}

fn horse_marked(notations: &[(String, String)], fpr: &str, allow: &Allowlist, for_sign: bool) -> bool {
    if notations
        .iter()
        .any(|(n, v)| n == HORSE_NOTATION && !v.is_empty())
    {
        return true;
    }
    let list = if for_sign {
        &allow.sign_fps
    } else {
        &allow.encrypt_fps
    };
    list.iter().any(|x| x.eq_ignore_ascii_case(fpr))
}

fn horse_blake2b_keyed(ed_pk: &[u8; 32], x_pk: &[u8; 32]) -> [u8; 32] {
    type Blake2bMac256 = blake2::Blake2bMac<blake2::digest::consts::U32>;
    let mut mac = <Blake2bMac256 as KeyInit>::new_from_slice(HORSE_IDFP_KEY)
        .expect("HORSE-IDFP key length");
    let mut data = [0u8; 64];
    data[..32].copy_from_slice(ed_pk);
    data[32..].copy_from_slice(x_pk);
    Mac::update(&mut mac, &data);
    let result = mac.finalize().into_bytes();
    let mut out = [0u8; 32];
    out.copy_from_slice(&result);
    out
}

fn strip_0x40(b: &[u8]) -> Result<[u8; 32]> {
    let raw = if b.len() == 33 && b[0] == 0x40 {
        &b[1..]
    } else if b.len() == 32 {
        b
    } else {
        bail!("unexpected ECC public length {}", b.len());
    };
    let mut out = [0u8; 32];
    out.copy_from_slice(raw);
    Ok(out)
}

fn pk_bytes(mpis: &mpi::PublicKey) -> Result<[u8; 32]> {
    match mpis {
        mpi::PublicKey::EdDSA { q, .. }
        | mpi::PublicKey::ECDSA { q, .. }
        | mpi::PublicKey::ECDH { q, .. } => strip_0x40(q.value()),
        mpi::PublicKey::Ed25519 { a } => {
            let mut out = [0u8; 32];
            out.copy_from_slice(a.as_ref());
            Ok(out)
        }
        mpi::PublicKey::X25519 { u } => {
            let mut out = [0u8; 32];
            out.copy_from_slice(u.as_ref());
            Ok(out)
        }
        _ => bail!("unsupported public key algorithm for Horse"),
    }
}

/// Convert OpenPGP Cv25519 secret MPI to libsodium native little-endian.
pub fn cv25519_secret_to_libsodium(mpi: &[u8], version: u8) -> Result<[u8; 32]> {
    let mut out = [0u8; 32];
    if version >= 6 {
        if mpi.len() != 32 {
            bail!("v6 X25519 secret length {}", mpi.len());
        }
        out.copy_from_slice(mpi);
    } else {
        if mpi.is_empty() || mpi.len() > 32 {
            bail!("v4 Cv25519 secret length {}", mpi.len());
        }
        let mut be = [0u8; 32];
        be[32 - mpi.len()..].copy_from_slice(mpi);
        for i in 0..32 {
            out[i] = be[31 - i];
        }
    }
    Ok(out)
}

pub fn ed25519_seed_from_mpi(mpi: &[u8], version: u8) -> Result<[u8; 32]> {
    let mut out = [0u8; 32];
    if version >= 6 {
        if mpi.len() != 32 {
            bail!("v6 Ed25519 seed length {}", mpi.len());
        }
        out.copy_from_slice(mpi);
        return Ok(out);
    }
    let raw = if mpi.len() == 32 {
        mpi
    } else if mpi.len() == 33 && mpi[0] == 0x40 {
        &mpi[1..]
    } else if mpi.len() < 32 {
        let mut padded = [0u8; 32];
        padded[32 - mpi.len()..].copy_from_slice(mpi);
        out.copy_from_slice(&padded);
        return Ok(out);
    } else {
        bail!("v4 Ed25519 seed length {}", mpi.len());
    };
    out.copy_from_slice(raw);
    Ok(out)
}

fn check_x25519_pair(sk: &[u8; 32], pk: &[u8; 32]) -> Result<()> {
    let secret = StaticSecret::from(*sk);
    let derived = X25519Public::from(&secret);
    if derived.as_bytes() != pk {
        bail!("X25519 secret does not derive the certificate public key");
    }
    Ok(())
}

fn check_ed25519_pair(seed: &[u8; 32], pk: &[u8; 32]) -> Result<()> {
    let sk = Ed25519SigningKey::from_bytes(seed);
    if sk.verifying_key().as_bytes() != pk {
        bail!("Ed25519 seed does not derive the certificate public key");
    }
    Ok(())
}

fn notations_from_sig(sig: &sequoia_openpgp::packet::Signature) -> Vec<(String, String)> {
    let mut out = Vec::new();
    for n in sig.notation_data() {
        let name = n.name().to_string();
        let value = match std::str::from_utf8(n.value()) {
            Ok(v) => v.to_string(),
            Err(_) => continue,
        };
        out.push((name, value));
    }
    out
}

/// Reference time for policy checks. A few seconds ahead absorbs gpg
/// second-rounding so freshly minted keys are not "not yet valid".
fn policy_time() -> SystemTime {
    SystemTime::now() + Duration::from_secs(60)
}

/// Run `f` with a ValidCert. Fresh gpg keys sometimes fail StandardPolicy
/// with "No binding signature"; retry a few reference times and, when
/// `--allow-unmarked` is set, fall back to NullPolicy (still selects by
/// key flags; marking is already relaxed). Policy must outlive ValidCert,
/// so this owns it for the callback.
fn with_valid_cert<R, F>(cert: &Cert, allow_unmarked: bool, f: F) -> Result<R>
where
    F: FnOnce(ValidCert<'_>) -> Result<R>,
{
    let std = StandardPolicy::new();
    let times = [
        Some(policy_time()),
        Some(SystemTime::now()),
        None,
    ];
    let mut last = None;
    for t in times {
        match cert.with_policy(&std, t) {
            Ok(vc) => return f(vc),
            Err(e) => last = Some(e),
        }
    }
    if allow_unmarked {
        // SAFETY: explicit opt-in for throwaway/CI radio keys. Callers still
        // select subkeys by key flags after accepting any binding signature.
        let null = unsafe { NullPolicy::new() };
        for t in times {
            if let Ok(vc) = cert.with_policy(&null, t) {
                return f(vc);
            }
        }
    }
    Err(anyhow!(
        "certificate policy: {}",
        last.map(|e| e.to_string()).unwrap_or_else(|| "unknown".into())
    ))
}

fn inspect(allowlist: Option<PathBuf>) -> Result<()> {
    let allow = load_allowlist(&allowlist)?;
    let cert = Cert::from_bytes(&read_stdin()?).context("parse certificate")?;
    let subkeys = with_valid_cert(&cert, true, |vc| {
        let mut subkeys = Vec::new();
        for ka in vc.keys().subkeys() {
            let fpr = ka.key().fingerprint().to_string();
            let flags = ka.key_flags().unwrap_or_else(KeyFlags::empty);
            let can_sign = flags.for_signing();
            let can_encrypt = flags.for_transport_encryption()
                || flags.for_storage_encryption();
            let notations = notations_from_sig(ka.binding_signature());
            let marked = horse_marked(&notations, &fpr, &allow, can_sign);
            subkeys.push(SubkeyInfo {
                fingerprint: fpr,
                algo: format!("{:?}", ka.key().pk_algo()),
                version: ka.key().version(),
                can_sign,
                can_encrypt,
                creation_time: format!("{:?}", ka.key().creation_time()),
                expiration_time: ka
                    .key_expiration_time()
                    .map(|t| format!("{:?}", t)),
                horse_marked: marked,
            });
        }
        Ok(subkeys)
    })?;

    println!(
        "{}",
        serde_json::to_string(&InspectOut {
            primary_fpr: cert.fingerprint().to_string(),
            subkeys,
        })?
    );
    Ok(())
}

fn pick_fps(
    vc: &ValidCert<'_>,
    allow: &Allowlist,
    allow_unmarked: bool,
) -> Result<(String, String)> {
    let mut sign_fpr = None;
    let mut enc_fpr = None;
    for ka in vc.keys().subkeys() {
        let fpr = ka.key().fingerprint().to_string();
        let flags = ka.key_flags().unwrap_or_else(KeyFlags::empty);
        let notations = notations_from_sig(ka.binding_signature());
        if flags.for_signing() {
            let marked = horse_marked(&notations, &fpr, allow, true);
            if (marked || allow_unmarked) && sign_fpr.is_none() {
                sign_fpr = Some(fpr.clone());
            }
        }
        if flags.for_transport_encryption() || flags.for_storage_encryption() {
            let marked = horse_marked(&notations, &fpr, allow, false);
            if (marked || allow_unmarked) && enc_fpr.is_none() {
                enc_fpr = Some(fpr);
            }
        }
    }
    Ok((
        sign_fpr.ok_or_else(|| anyhow!("no usable marked signing subkey"))?,
        enc_fpr.ok_or_else(|| anyhow!("no usable marked encryption subkey"))?,
    ))
}

fn extract_public(allow_unmarked: bool, allowlist: Option<PathBuf>) -> Result<()> {
    let allow = load_allowlist(&allowlist)?;
    let cert = Cert::from_bytes(&read_stdin()?).context("parse certificate")?;
    let (sign_fpr, enc_fpr, ed_pk, x_pk) =
        with_valid_cert(&cert, allow_unmarked, |vc| {
            let (sign_fpr, enc_fpr) = pick_fps(&vc, &allow, allow_unmarked)?;
            let mut ed_pk = [0u8; 32];
            let mut x_pk = [0u8; 32];
            for ka in vc.keys().subkeys() {
                let fpr = ka.key().fingerprint().to_string();
                if fpr == sign_fpr {
                    ed_pk = pk_bytes(ka.key().mpis())?;
                }
                if fpr == enc_fpr {
                    x_pk = pk_bytes(ka.key().mpis())?;
                }
            }
            Ok((sign_fpr, enc_fpr, ed_pk, x_pk))
        })?;
    let horse_fp = horse_blake2b_keyed(&ed_pk, &x_pk);
    println!(
        "{}",
        serde_json::to_string(&PublicOut {
            cert_fpr: cert.fingerprint().to_string(),
            ed25519_pk: hex::encode(ed_pk),
            x25519_pk: hex::encode(x_pk),
            horse_fingerprint: hex::encode(horse_fp),
            sign_fpr,
            encrypt_fpr: enc_fpr,
        })?
    );
    Ok(())
}

fn map_secret(
    secret: &SecretKeyMaterial,
    version: u8,
    public: &mpi::PublicKey,
    want_sign: bool,
) -> Result<([u8; 32], [u8; 32])> {
    match secret {
        SecretKeyMaterial::Unencrypted(unenc) => {
            unenc.map(|mpis| -> Result<([u8; 32], [u8; 32])> {
                let (out_sk, out_pk) = match mpis {
                    mpi::SecretKeyMaterial::EdDSA { scalar }
                    | mpi::SecretKeyMaterial::ECDSA { scalar } => {
                        if !want_sign {
                            bail!("expected encryption secret, got signing");
                        }
                        let sk = ed25519_seed_from_mpi(scalar.value(), version)?;
                        let pk = pk_bytes(public)?;
                        check_ed25519_pair(&sk, &pk)?;
                        (sk, pk)
                    }
                    mpi::SecretKeyMaterial::Ed25519 { x } => {
                        if !want_sign {
                            bail!("expected encryption secret, got Ed25519");
                        }
                        let mut sk = [0u8; 32];
                        sk.copy_from_slice(x.as_ref());
                        let pk = pk_bytes(public)?;
                        check_ed25519_pair(&sk, &pk)?;
                        (sk, pk)
                    }
                    mpi::SecretKeyMaterial::ECDH { scalar, .. } => {
                        if want_sign {
                            bail!("expected signing secret, got ECDH");
                        }
                        let sk = cv25519_secret_to_libsodium(scalar.value(), version)?;
                        let pk = pk_bytes(public)?;
                        check_x25519_pair(&sk, &pk)?;
                        (sk, pk)
                    }
                    mpi::SecretKeyMaterial::X25519 { x } => {
                        if want_sign {
                            bail!("expected signing secret, got X25519");
                        }
                        let mut sk = [0u8; 32];
                        sk.copy_from_slice(x.as_ref());
                        let pk = pk_bytes(public)?;
                        check_x25519_pair(&sk, &pk)?;
                        (sk, pk)
                    }
                    _ => bail!("unsupported secret MPI for radio subkey"),
                };
                Ok((out_sk, out_pk))
            })
        }
        SecretKeyMaterial::Encrypted(_) => {
            bail!("subkey still encrypted (stub, card, or wrong passphrase)")
        }
    }
}

fn extract_secret(
    passphrase_fd: i32,
    allow_unmarked: bool,
    allowlist: Option<PathBuf>,
) -> Result<()> {
    let allow = load_allowlist(&allowlist)?;
    let pass = read_passphrase_fd(passphrase_fd)?;
    let pass_bytes: &[u8] = pass.as_ref();
    let password = Password::from(pass_bytes.to_vec());
    let cert = Cert::from_bytes(&read_stdin()?).context("parse secret key")?;
    let (sign_fpr, enc_fpr) = with_valid_cert(&cert, allow_unmarked, |vc| {
        pick_fps(&vc, &allow, allow_unmarked)
    })?;

    let mut ed_seed = [0u8; 32];
    let mut ed_pk = [0u8; 32];
    let mut x_sk = [0u8; 32];
    let mut x_pk = [0u8; 32];
    let mut got_sign = false;
    let mut got_enc = false;

    for ka in cert.keys().subkeys() {
        let fpr = ka.key().fingerprint().to_string();
        let want_sign = fpr == sign_fpr;
        let want_enc = fpr == enc_fpr;
        if !want_sign && !want_enc {
            continue;
        }
        let key = ka.key().clone();
        let key = match key.parts_into_secret() {
            Ok(k) => k,
            Err(_) => bail!("subkey has no secret material (stub or card)"),
        };
        let unlocked = if key.has_unencrypted_secret() {
            key
        } else {
            key.decrypt_secret(&password)
                .map_err(|_| anyhow!("failed to decrypt secret subkey"))?
        };
        let version = unlocked.version();
        let public = unlocked.mpis().clone();
        let secret = unlocked.secret().clone();
        let (sk, pk) = map_secret(&secret, version, &public, want_sign)?;
        if want_sign {
            ed_seed = sk;
            ed_pk = pk;
            got_sign = true;
        } else {
            x_sk = sk;
            x_pk = pk;
            got_enc = true;
        }
    }

    if !got_sign || !got_enc {
        bail!("failed to extract both radio subkey secrets");
    }

    let horse_fp = horse_blake2b_keyed(&ed_pk, &x_pk);
    let out = SecretOut {
        cert_fpr: cert.fingerprint().to_string(),
        ed25519_pk: hex::encode(ed_pk),
        ed25519_seed: hex::encode(ed_seed),
        x25519_pk: hex::encode(x_pk),
        x25519_sk: hex::encode(x_sk),
        horse_fingerprint: hex::encode(horse_fp),
    };
    ed_seed.zeroize();
    x_sk.zeroize();
    println!("{}", serde_json::to_string(&out)?);
    Ok(())
}

fn main() {
    if let Err(e) = real_main() {
        eprintln!("error: {:#}", e);
        std::process::exit(1);
    }
}

fn real_main() -> Result<()> {
    let cli = Cli::parse();
    match cli.cmd {
        Cmd::InspectCert { allowlist } => inspect(allowlist),
        Cmd::ExtractPublic {
            allow_unmarked,
            allowlist,
        } => extract_public(allow_unmarked, allowlist),
        Cmd::ExtractSecret {
            passphrase_fd,
            allow_unmarked,
            allowlist,
        } => extract_secret(passphrase_fd, allow_unmarked, allowlist),
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn v4_cv25519_secret_reversed() {
        let mut be = [0u8; 32];
        for i in 0..32 {
            be[i] = i as u8;
        }
        let le = cv25519_secret_to_libsodium(&be, 4).unwrap();
        for i in 0..32 {
            assert_eq!(le[i], be[31 - i]);
        }
    }

    #[test]
    fn v6_x25519_native() {
        let mut sk = [0u8; 32];
        for i in 0..32 {
            sk[i] = (31 - i) as u8;
        }
        assert_eq!(cv25519_secret_to_libsodium(&sk, 6).unwrap(), sk);
    }

    #[test]
    fn ed25519_seed_roundtrip_derive() {
        let seed = [7u8; 32];
        let sk = Ed25519SigningKey::from_bytes(&seed);
        check_ed25519_pair(&seed, sk.verifying_key().as_bytes()).unwrap();
    }
}
