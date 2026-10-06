# Horse first hardware tests (MD-3x0)

Document only. No on-air work was done for this plan. Tests are ordered.
Each step lists what to watch and what counts as a pass. Earlier
"unverified" items closed by a pass are listed at the end of the step.

Radios: TYT MD-380/390 running this branch's `openrtx_md3x0` with
`CONFIG_HORSE` and libsodium 1.0.20. Use a dummy load or cabled
attenuators. Do not key a transmitter into an antenna until step 4.

## 1. Provisioning and passphrase unlock

**Do:** Flash firmware. Provision an identity with
`scripts/horse_provision.py` (non-empty passphrase). Power on, enter
Horse, unlock. Repeat with an identity stored with passphrase `""` if
that path is still claimed.

**Observe:** UI accepts the passphrase or reports a Horse error; the
radio does not crash or reboot; unlock completes in a few seconds.

**Pass:** Unlock succeeds for the provisioned identity; a wrong
passphrase fails closed (no TX, `Horse: no keys` or equivalent, not
cleartext). Empty-passphrase case either unlocks as specified or is
recorded as a known failure.

**Closes:** identity NVM offset vs MD-3x0 partition map; provisioning
path on device; Argon2id 16 KiB heap vs remaining SRAM at unlock;
empty-passphrase unlock if that case is run.

## 2. RX of a recorded baseband file (if the hardware allows injection)

**Do:** If the MD-3x0 audio/C5000 path can be fed a 24 kHz (or
documented) I/Q or discriminator recording from the host loopback
corpus, inject a clean v2 LSF+voice+EOT file. If injection is not
possible, skip and mark this step unverified.

**Observe:** `horse_src` / `horse_dst` on the display; speaker audio
after LSF CRC and a valid tag; EOT returns the radio to idle.

**Pass:** Opening LSF accepted, voice frames decoded with tags, EOT
seen, no reboot. Skip is not a pass.

**Closes:** device Viterbi per frame at 168 MHz (qualitative: audio
without dropouts); C5000 RX path; demod on real ADC samples.

## 3. First TX into a dummy load, keying path watched

**Do:** Dummy load on the antenna port. Scope or current-shunt on the
PA enable / C5000 TX path. PTT in encrypt mode with a valid peer.

**Observe:** Time from PTT to `radio_enableTx` / PA bias; LSF then
voice; PTT release sends EOT then unkey (not zeros). Display shows
destination, not `Horse: TX crypto`.

**Pass:** Transmitter keys only after X25519 (and sign, if enabled)
finish; EOT then unkey on release; no keying on crypto refuse.

**Closes:** C13 C5000 TX enable; PTT-to-key delay vs the estimates in
`HORSE_AUDIT.md`; TX not keyed until key agreement.

## 4. Two radios cabled through attenuators

**Do:** Two provisioned radios, known peer keys, 40 dB or more of
attenuation, no antennas.

**Observe:** Radio A PTT, radio B display and audio; reverse.

**Pass:** B shows A's callsign, intelligible MELPe, EOT on release.
A does not decode B without the matching peer (covered in step 6).

**Closes:** on-air (cabled) TX/RX; UI during a live call.

## 5. Each of the three modes

**Do:** Repeat step 4 for encrypt-only, signed-only, and
encrypt+signed. Confirm LSF flags (encrypted / signed) on the receiver
if a debug view exists; otherwise use audio + signature accept/reject.

**Pass:** Encrypt: audio only with matching X25519. Signed: audio with
valid Ed25519 session signature; unsigned or wrong signer rejected.
Both: both checks required.

**Closes:** three-mode path on hardware (host already has
`horse_session_modem`).

## 6. Wrong key

**Do:** Receiver provisioned with a different X25519 and/or Ed25519
peer than the transmitter.

**Observe:** No decrypted audio; no `horseLsfOk` session; error or
idle, not plaintext MELPe.

**Pass:** Fail closed. No speaker audio from the foreign transmission.

**Closes:** C2/C12 fail-closed on the radio (host already tested).

## 7. PTT release during key setup

**Do:** PTT, then release before the PA keys (during X25519 / sign /
derive). Repeat several times.

**Observe:** `horse_crypto_cancel`; transmitter never stays keyed;
UI returns to idle; next PTT can start a new session.

**Pass:** No stuck TX, no crash, no late keying after unkey.

**Closes:** PTT cancel vs 16 KiB crypto worker on Miosix.

## 8. Maximum-length transmission

**Do:** Hold PTT until `VOICE_FN_MAX` / call-limit (`Horse: call limit`).

**Observe:** Last legal voice frame, EOT, unkey; error string; radio
still accepts a new PTT after idle.

**Pass:** Clean EOT and unkey at the limit; no wrap of FN/nonce; no
hang.

**Closes:** call-limit path on hardware; long TX thermal/PA behaviour
only as far as this dummy-load step.

## 9. Low signal

**Do:** Increase attenuation (or reduce TX power) until decode is
marginal. Do not use an antenna.

**Observe:** Late entry via fragments if the opening LSF is missed;
tag-decided voice after a valid session; drop to idle on EOT or loss
of lock; no cleartext.

**Pass:** Behaviour matches host floors qualitatively (more erasures,
not a crash). Record RSSI / attenuator setting; this is not a
sensitivity specification.

**Closes:** device demod under noise; flywheel/tag-decided path on ADC
samples (host campaign remains the quantitative record).

## Not closed by these steps

- RF into an antenna, type approval, or occupied-bandwidth plots
- Exact Cortex-M4 cycle counts (estimates only until a DWT/ITM trace)
- Other radio families (`CONFIG_HORSE` is Linux and MD-3x0 only)
