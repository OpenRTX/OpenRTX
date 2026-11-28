# Horse FEC v2 as built

Protocol version 2. This file is the FEC and modem claim for the
current tree, not a design sketch. On-air format is unchanged by the
receiver-only work below.

## Waveform

- 4800 symbols/s 4-FSK, 192-symbol (48-byte) frames
- Sync (v2): LSF `{0x15,0x57}`, voice `{0x45,0xFD}`, EOT `{0x77,0x74}`
- Dibits: `00=+1`, `01=+3`, `10=-1`, `11=-3`
- Voice/LSF coding: M17 convolutional encoder, DATA_PUNCTURE (12-tap),
  interleaver, decorrelator, Viterbi
- Soft Viterbi: `uint16_t`, 0 = strong 0, 32767 = erasure, 65535 =
  strong 1, puncture `0x7FFF`
- Soft map: linear distance to the 4-FSK bit boundaries, scaled by the
  outer-deviation estimate. Versus an ideal Gaussian LLR the cliff at
  1 % voice FER is about 0.8-1.0 dB worse; operational 8000-11000 is
  not mapping-limited.

## Acquisition and lock (receiver)

- Acquire Hamming 0 on LSF or voice; track Hamming 2
- After one accepted voice frame, flywheel: nearest sync, HD <= 4
- Before authentication, that rule is the whole voice path
- After LSF CRC and a valid tag (`lockAuthenticated`): attempt a voice
  decode in every frame slot unless the sync is within Hamming 2 of
  EOT. The 32-bit tag decides acceptance
- Coast unlock: 4 missed syncs, or EOT HD <= 2, or 12 frames without a
  tag. Missed EOT still unlocks on coast (host: extra frames after the
  last voice is the miss limit, not a hang)

## Meson floors and owner targets

Floors (hard path still compiled in `gates`; not performance goals):

- strict >= 180/200 at noise 8000
- usable >= 165/200 at noise 10000

Owner-approved soft-decision targets (both seeds 1 and 2, 200 TX,
`miss_unlock=4`):

- noise 8000: strict >= 190/200, usable >= 195/200
- noise 10000: usable >= 180/200, strict >= 150/200

Host campaign that set those numbers (encrypt/sign/both, two seeds):
8000 strict 192-196 usable 198-200; 10000 strict 156-159 usable 186-191.

## Stream loss (noise 10000, 200 TX x 300 voice, soft+track)

| tag-decided | nloss | miss | decode | tag | unknown | voice_ty |
|------------:|------:|-----:|-------:|----:|--------:|---------:|
| off (before) | 33 | 26 | 3 | 4 | 22 | 59978 |
| on (after) | 30 | 23 | 3 | 4 | 12 | 59988 |

Unknown HD 3-4 syncs drop under the authenticated rule; tag failures
are unchanged. Not an on-air change.

## Memory (host)

`sizeof(HorseFrameDecoder)` = 8872. Fragment soft accumulator is
`int16_t` with (soft-32767)/3 then saturating add (same majority sign
as the previous `int32_t` path). That array is 1920 B (was 3840 B).

Other static/heap that can shrink without changing on-air behaviour:

- `fragCopy` 360 B (hard majority; still used on the hard path)
- `lastSpareSoft` 192 B (could be a decode local)
- demod `unique_ptr` frame/soft/sample pairs (heap, not BSS)
- crypto worker job buffer (~0.5 KiB BSS) plus a 16 KiB worker stack

Host libsodium stack high-water (pattern-filled pthread, delta vs
empty call; Cortex-M4 will differ):

| Call | delta bytes |
|------|------------:|
| X25519 keypair | 16 |
| X25519+BLAKE2b derive | 616 |
| Ed25519 sign | 56 |
| Ed25519 verify | 1616 |
| XChaCha20+BLAKE2b voice | 64 |
| BLAKE2b raw | 16 |
| XChaCha20 raw | 16 |
| Argon2id 16 KiB | 5000 |

RTX thread stack is 512 B. Remaining Horse call-chain budget was
~48-112 B, so every sodium call runs on the 16 KiB crypto worker.
TX does not `radio_enableTx` until key agreement (keypair, optional
sign, derive) has finished. PTT release cancels the job.

## Unverified without MD-3x0 cross map and a radio

See the single list in `docs/horse/PR_DESCRIPTION.md`.
