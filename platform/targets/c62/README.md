<!--
 - SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 -
 - SPDX-License-Identifier: GPL-3.0-or-later
-->

# Retevis C62

This target is based on the ListenAI CSK6011B (CSK6 family) SoC.

## Building

For the following commands enter shell with the ListenAI environment

```bash
lisa zep exec bash
````

The C62 is a Zephyr target. It is built via `west` but wrapped through `meson` for convenience:

```bash
rm -rf build
meson setup build
meson compile -C build openrtx_c62
```

The build will automatically run `west update --group-filter +csk6` to fetch
CSK6-specific Zephyr modules (AF, FreeRTOS shims, LSF, URPC) before compiling.

Alternatively, using `west` directly:

```bash
west build -b c62 -d build .
```

### DSP firmware

OpenRTX does not boot without the ListenAI DSP firmware, flashed at
`0x100000`, and the image must match the audio code. The build pins it by
hash and places it in `build/zephyr/dsp_firmware.bin`, next to `zephyr.bin`:

| | |
|---|---|
| Source | [`OpenRTX/lsf-zephyr-sdk`](https://github.com/OpenRTX/lsf-zephyr-sdk) `2ac60552` (branch `feature/6012_nano_recorder_48k`), `resources/dsp_firmware.bin` |
| Size | 466,832 bytes (48 kHz audio) |
| sha256 | `9579b617f048c646465118f6d0b9e1b765b4d7d811ff9c8649c5754fe45a8903` |

By default it is downloaded while configuring. For offline builds, point the
`C62_DSP_FIRMWARE` environment variable (or CMake variable) at a local copy;
the build fails if its hash differs.

## Flashing

> **Warning:** This may brick your device! Use at your own risk!

cskburn talks to the boot ROM in burn mode: radio off, hold the lower side
key, power on, release; the screen stays dark. cskburn resets the radio after
each command, so enter burn mode again before every one, and don't press the
main PTT while it runs: it shares the boot ROM's UART RX line.

Back up the whole 4 MiB flash first, and only ever restore a radio from its
own backup: the OEM firmware is bound to the chip, and the data at `0x3b0000`
is unique to each radio.

```bash
cskburn -s /dev/ttyUSB0 -C 6 -b 115200 -v --read 0x0:0x400000:backup.bin
```

Then write both images in one session:

```bash
cskburn -s /dev/ttyUSB0 -C 6 -b 115200 -v --verify-all \
    0x100000 build/zephyr/dsp_firmware.bin \
    0x000000 build/zephyr/zephyr.bin
```
