# Horse device support

Pinned upstream device table:
[OpenRTX/openrtx.github.io](https://github.com/OpenRTX/openrtx.github.io)
commit `a939a79` (2026-10-28), file `dev_status.md`.

Horse (`CONFIG_HORSE`) is enabled only for **linux** and **md3x0** in
this fork's `meson.build`. No other targets are enabled by this work.
RF behaviour below that is marked "untested" has not been measured on
hardware.

## Basic functions (upstream) + Horse build

| Radio model | Upstream boot/display/etc. | Horse build (`CONFIG_HORSE`) | Notes |
| ----------- | -------------------------- | ---------------------------- | ----- |
| Tytera MD-380 / MD-390 | Supported (see upstream) | **Yes** (`openrtx_md3x0`) | Primary Horse radio target |
| Tytera MD-UV380 / Retevis RT3s | Supported | No | M17 TX yes upstream; Horse not enabled |
| Radioddity GD-77 | Supported | No | No M17 path; Horse not possible |
| Baofeng DM-1801 | Supported | No | No M17 path; Horse not possible |
| Tytera MD-9600 | Partial upstream | No | No M17 TX upstream |
| Module 17 | Supported | No | M17 TX yes; Horse not enabled |
| Lilygo T-TWR plus | Supported | No | No M17 TX upstream |
| Radtel RT-890 | Partial upstream | No | No M17 TX upstream |
| Talkpod A36plus | Supported | No | No M17 TX upstream |
| Retevis C62 | Partial | No | No M17 TX upstream |
| Connect Systems CS7000-M17 | Supported | No | M17 TX yes; Horse not enabled |
| Connect Systems CS7000-M17 Plus | Supported | No | M17 TX yes; Horse not enabled |
| Baofeng DM-1701 | Built by this fork; not in upstream table | No | Fork builds `openrtx_dm1701` |
| Radtel RT-4D | Built by this fork; not in upstream table | No | Fork builds `openrtx_rt4d` |
| Linux emulator | N/A | **Yes** (`openrtx_linux`) | Host development / loopback |

## Modes (upstream M17) + Horse RX/TX

| Radio model | M17 RX | M17 TX | Horse RX | Horse TX | Notes |
| ----------- | :----: | :----: | :------: | :------: | ----- |
| MD-380/390/RT3 **(VHF)** | Yes | No | Built (untested) | **Refused** (`HORSE_ERR_VHF`) | Same RF path as M17; TX refused in software |
| MD-380/390/RT3 **(UHF)** | Yes | Yes | Built (untested) | Built (untested) | C13 C5000 LINE_IN fix; deviation C21 open |
| MD-UV380 / RT3s | Yes | Yes | Not built | Not built | See work list below |
| GD-77 | N/A | N/A | N/A | N/A | No M17; Horse not possible |
| DM-1801 | N/A | N/A | N/A | N/A | No M17; Horse not possible |
| MD-9600 | No | No | Not built | Not built | No M17 TX upstream |
| Module 17 | Yes | Yes | Not built | Not built | See work list below |
| T-TWR plus | No | No | Not built | Not built | No M17 TX upstream |
| RT-890 | No | No | Not built | Not built | No M17 TX upstream |
| A36plus | No | No | Not built | Not built | No M17 TX upstream |
| C62 | No | No | Not built | Not built | No M17 TX upstream |
| CS7000-M17 | Yes | Yes | Not built | Not built | See work list below |
| CS7000-M17 Plus | Yes | Yes | Not built | Not built | See work list below |
| Linux emulator | N/A | N/A | Host tests | Host tests | Dual-band hwInfo; VHF refuse does not apply |

## Work needed to enable Horse on M17-capable targets (not done here)

Do **not** enable these targets without a flash/RAM budget check against
`HORSE_AUDIT.md` (MD-3x0 with libsodium ~350000 B flash / ~47% CCM).

### MD-UV380 / RT3s (`openrtx_mduv3x0`)

- Add `CONFIG_HORSE` + `horse_src` to the Meson target (keep non-Horse
  binaries byte-identical when Horse is off).
- `radio_UV3x0.cpp`: `OPMODE_HORSE` in `radio_setOpmode` and
  `radio_enableTx` (LINE_IN, bandwidth TBD vs C21).
- UI: Horse mode entry and `horseError` strings (default UI already has
  Horse under `#ifdef CONFIG_HORSE`).
- NVM identity offset and peer sidecar vs UV380 flash map.
- Confirm dual-band: VHF refuse must not block UHF TX on this radio.

### CS7000 / CS7000 Plus

- Meson `CONFIG_HORSE`; HR_C6000 opmode cases in `radio_CS7000.cpp`.
- Flash/RAM vs libsodium + 16 KiB Argon2 + worker + codec stacks.
- UI / NVM paths for CS7000 settings (CRC behaviour differs; see C9).

### Module 17

- Meson enable; baseband is already digital-oriented (no FM C5000).
- Map Horse modulator/demodulator to Module17 I/Q path.
- Calibration invert flags already read in `OpMode_Horse`.
- Flash budget on the Module17 MCU.

## Explicitly not candidates

- **GD-77 / DM-1801:** no M17 digital path in OpenRTX.
- **T-TWR plus, RT-890, A36plus, C62, MD-9600:** no M17 TX upstream;
  Horse TX would need a working 4-FSK TX path first.
