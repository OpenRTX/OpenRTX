<!--
 - SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 -
 - SPDX-License-Identifier: GPL-3.0-or-later
-->

# Retevis C62

This target is based on the ListenAI CSK6011B SoC.

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

The build will automatically run `west update --group-filter +c62` to fetch
C62-specific Zephyr modules (AF, FreeRTOS shims, LSF, URPC) before compiling.

Alternatively, using `west` directly:

```bash
west build -b c62 -d build .
```

## Flashing

> **Warning:** This may brick your device! Use at your own risk!

```bash
cskburn -s /dev/ttyUSB0 -C 6 -b 115200 0x000000 build/zephyr/zephyr.hex
```
