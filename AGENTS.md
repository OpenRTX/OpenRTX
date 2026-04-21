# OpenRTX Project Guidelines

## Project Overview

OpenRTX is a modular, open-source firmware for digital amateur radio devices. It targets 9+ hardware platforms plus a Linux emulator. The principle is "write it once for all devices" — code should be hardware-agnostic whenever possible, with platform-specific logic strictly isolated.

## Code Style

- **Standard**: Linux kernel coding style, enforced via clang-format v11+
- **C standard**: GNU17 (`-std=gnu17`)
- **C++ standard**: C++14 (`-std=c++14`), no exceptions, no RTTI
- **Indentation**: 4 spaces (no tabs)
- **Line length**: 80 characters max
- **Functions**: `snake_case` for C, `PascalCase` for C++ classes, `camelCase` for C++ methods
- **Braces**: Opening brace on next line for function definitions; same line for control structures
- **Formatter**: Run `scripts/clang_format.sh` before committing. The project is in a gradual adoption phase — add any new or modified files to the file list in the script.

## Architecture

```
openrtx/           Core firmware (hardware-agnostic)
├── include/        Public headers (calibration, core, fonts, interfaces, peripherals, protocols, rtx, ui)
└── src/
    ├── core/       State, graphics, audio, voice prompts, battery
    ├── protocols/  Protocol implementations (M17, etc.) — structured per OSI layers 1-3
    ├── rtx/        Operational adapters integrating protocols with audio, PTT, squelch
    └── ui/         UI implementations (default/ and module17/)
platform/           Hardware-specific code
├── drivers/        Device drivers organized by component type
├── mcu/            MCU-specific implementations (STM32F4xx, STM32H7xx, MK22FN512, x86_64)
└── targets/        Device configs with pin mappings and hardware definitions
tests/
├── unit/           Automated tests (Catch2 framework)
└── platform/       Manual hardware verification tests
scripts/            Build and utility scripts
```

### Key Separation Rules

- Never put hardware-specific code in `openrtx/` — use `platform/` and the interfaces layer
- Protocol code in `openrtx/src/protocols/` must be structured per OSI layers 1-3
- UI code is split between `default/` (standard radios) and `module17/` (Module17 device)

## Build and Test

**Build system**: Meson (>= 1.2.0)

```bash
# Linux build and test
meson setup build_linux
meson compile -C build_linux openrtx_linux
meson test -C build_linux

# For running e2e tests (scripts/run_e2e.py), the linux binaries must be built
# with a fixed version string so the Info screen renders deterministically:
meson setup build_linux -Dtest_version=e2e-test
# (or `meson setup --reconfigure build_linux -Dtest_version=e2e-test` on an
#  existing build directory)

# Cross-compile for ARM Cortex-M4 targets
meson setup --cross-file cross_cm4.txt build_cm4
meson compile -C build_cm4 openrtx_md3x0      # TYT MD-380/390
meson compile -C build_cm4 openrtx_mduv3x0     # TYT MD-UV380/390
meson compile -C build_cm4 openrtx_md9600      # TYT MD-9600
meson compile -C build_cm4 openrtx_gd77        # Radioddity GD-77
meson compile -C build_cm4 openrtx_dm1801      # Baofeng DM-1801
meson compile -C build_cm4 openrtx_dm1701      # Baofeng DM-1701
meson compile -C build_cm4 openrtx_mod17       # Module17
meson compile -C build_cm4 openrtx_cs7000      # CS7000-M17

# Cross-compile for ARM Cortex-M7
meson setup --cross-file cross_cm7.txt build_cm7
meson compile -C build_cm7 openrtx_cs7000p     # CS7000-M17 Plus

# Run with address sanitizer
meson setup build_linux_address -Dasan=true
meson test -C build_linux_address

# Code coverage
bash scripts/coverage.sh

# Zephyr/ESP32S3 build (T-TWR Plus)
# Requires Zephyr SDK and west; see west.yml for manifest
west build -b ttwrplus
python3 scripts/uf2conv.py <binary> -o openrtx_ttwrplus.uf2
```

The T-TWR Plus (ESP32S3/Xtensa) target uses Zephyr RTOS and CMake instead of Meson cross-compilation. Board definitions live in `platform/targets/ttwrplus/`, and Zephyr configuration is in `platform/mcu/ESP32S3/zephyr.conf`. The `west.yml` manifest pins the Zephyr, MCUboot, and HAL Espressif versions.

### Python Scripts

Python scripts in `scripts/` should be run inside a virtual environment with dependencies from `requirements.txt`:

```bash
python3 -m venv .venv
source .venv/bin/activate
pip install -r requirements.txt
```

## Conventions

### File Headers

Every source file must include an SPDX header:

```c
/*
 * SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
 * 
 * SPDX-License-Identifier: GPL-3.0-or-later
 */
```

### Documentation

Use Doxygen-style comments for public API functions:

```c
/**
 * \brief Brief description of function.
 *
 * @param name: parameter description.
 * @return description of return value.
 */
```

### Testing

- Unit tests use **Catch2** with BDD-style `TEST_CASE` and `REQUIRE` macros
- Tests live in `tests/unit/` and are registered in `meson.build`
- Test names use descriptive strings: `TEST_CASE("Golay24 encode/decode without errors", "[m17][golay]")`
- PRs must document testing performed (automated and/or manual)
- Run a specific test: `meson test -C build_linux "Test Name"`

### String Table

The `stringsTable_t` in `openrtx/include/ui/ui_strings.h` defines UI strings and is indexed by voice prompts. **Never reorder existing entries** — only append new strings at the end (before the closing brace).

### Commits and PRs

- Each commit should be a distinct, complete change (no broken intermediate states)
- Keep PRs focused; split large changes into incremental pieces
- Rebase before submission (no unnecessary merge commits)
- Each commit supported by an AI agent must contain its relevant "Co-authored-by" line in the commit message
- Each commit message should be prefaced with the module that the change relates to; common modules are: ui, m17, core, ci, test, nvm, build, drivers, scripts, rtx, protocols, reuse, audio, nvmem, linux; if a change uses multiple of these, it may be an indication that the change should be split into smaller commits
- Never add a git sign-off on behalf of the user; they must signoff the commits on their own; remind the user that they must do this before the PR can be considered ready
- All PRs raised by agents must be left in draft state; users must be responsible for taking it out of draft state once they've reviewed it; remind the user to do this

### Embedded Constraints

- C++ is compiled with `-fno-exceptions -fno-rtti -D__NO_EXCEPTIONS`
- Optimize for size (`-Os`) on embedded targets
- Be mindful of heap usage — `state.heap_usage` tracks it at runtime
- Custom linker scripts define memory layouts per platform
