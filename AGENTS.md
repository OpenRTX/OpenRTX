# OpenRTX Project Guidelines

## Project Overview

OpenRTX is a modular, open-source firmware for digital amateur radio devices. It targets 9+ hardware platforms plus a Linux emulator. The principle is "write it once for all devices" — code should be hardware-agnostic whenever possible, with platform-specific logic strictly isolated.

## Code Style

- **Standard**: Linux kernel coding style, enforced via clang-format v11+
- **C standard**: GNU17 (`-std=gnu17`) on the Meson targets. The T-TWR Plus Zephyr build compiles the shared `openrtx/` C as C99 (Zephyr's default), so hardware-agnostic C must stay C99-compatible: no C11-only features such as `max_align_t` — use `__BIGGEST_ALIGNMENT__` instead
- **C++ standard**: C++14 (`-std=c++14`), no exceptions, no RTTI
- **Indentation**: 4 spaces (no tabs)
- **Line length**: 80 characters max
- **Functions**: `snake_case` for C, `PascalCase` for C++ classes, `camelCase` for C++ methods
- **Braces**: Opening brace on next line for function *and class* definitions (`BraceWrapping: AfterClass: true`); same line for control structures — `for (int i = 0; i < N; i++) {`
- **Formatter**: Run `scripts/clang_format.sh` before committing. The project is in a gradual adoption phase, and the script carries an *exclusion* list of files not yet converted — when you bring a file up to the current style, remove it from that list. The script only inspects files known to git, so `git add` a new file before running the formatter. See [Working With the Formatter](#working-with-the-formatter).

Note that `coding_style.md` on the website predates the clang-format migration (#166) and still
describes Allman bracing for control statements. Where the two disagree, `.clang-format` in this
repository is authoritative.

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
└── platform/       Manual hardware verification tests and host-side analysis tools
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
uv venv
uv pip install -r requirements.txt
source .venv/bin/activate
```

## Conventions

Most of this section was transcribed from maintainer review comments rather than written from
scratch: roughly 400 review comments across PRs #55 through #500, collected into rules. Each rule
carries the PR numbers it was drawn from, so it can be traced back to the review that produced it —
and corrected at the source when the transcription got it wrong.

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
 * First sentence is the summary; most of the code does not use \brief.
 *
 * @param name: parameter description.
 * @return description of return value.
 */
```

Data members and enumerators get a trailing `///< Description.` comment, aligned within the block
(`protocols/M17/Demodulator.hpp`).

Documentation lives in the **header**, next to the prototype — not in the `.c`/`.cpp` file (#396).
Every public function, method and non-obvious struct field gets a header comment (#340, #363, #390, #396).

### Naming

- Every function carries its module as a prefix: `history_insert`, `input_getDtmfChar`, `aprsPktList_init`, `vfo_load`. Functions without the prefix get sent back (#306, #340, #367, #396, #456).
- Drivers are named `<peripheral><Chip>` — `spiStm32`, `adcStm32`, `flashStm32`, `adcHrc7000`. Name the driver after the silicon it talks to, never after the radio that happens to use it first: the next target will reuse it (#403, #498). Files are snake_case `<peripheral>_<chip>.c`: `adc_stm32f4.c`, `flash_stm32.c`, `gpio_bk4819.c` (#534).
- All build-time feature and hardware macros use the `CONFIG_` prefix, matching `CONFIG_RTC`, `CONFIG_GPS`, `CONFIG_DISPLAY`. Not a bare name (#426, #492). `PLATFORM_<TARGET>` is the one exception: it identifies the target being built (#510).
- Name a `CONFIG_` macro for what it configures, not for the first thing that used it: `CONFIG_CODEC_ENCODE_GAIN`, not `CONFIG_MIC_GAIN`, because codec2 accepts any input stream (#442).
- Names describe what a thing *is*, not the container it currently sits in or the operation currently performed on it: `struct MessageSource` over `struct MessageListEntry`, `struct hdlcFrame` over `struct frame`, `ui_printAprsPkt` over `ui_drawAprsPkt` (you do not *draw* a packet), `testSyncword` over `trySync` (we are already synchronised; we are testing) (#396, #417, #492).
- Prefer the shorter name when it is not ambiguous: `sample`, not `processSingleSample`; `multiKeyEvent`, not `multiKeyEventInProgress`; `snapshotSize`, not `snapshotLens` (#306, #417, #492).
- Operation tables are named `<thing>_ops`, kernel-style: `struct message_ops` (#492).

### Types and Data Structures

- **Do not `typedef` a `struct` or an `enum`.** They are types already. Write `struct address`, not `address_t`. Reserve `typedef` for types that are not structs or enums. The codebase still contains many old typedefs and they are being removed progressively; do not add new ones. See the [kernel typedef guidance](https://www.kernel.org/doc/html/v5.6/process/coding-style.html#typedefs) (#340, #396, #481).
- Use an `enum` (or `enum class` in C++) instead of a group of related `#define`s — for symbol IDs, chip command sets, register values, state codes, sector sizes (#165, #360, #362, #467, #498). A bitfield may legitimately be assigned from an enum; see `settings.h`'s `display_timer` (#360).
- Integer variables use explicitly-sized types from `stdint.h`/`cstdint`. `char` and `unsigned char` are not 8-bit integer types. Sizes, lengths and indices are `size_t`, which avoids casts (#509).
- Functions that can fail return `0` on success and a negative `errno` value on failure (`-ENOENT`, `-ENOTSUP`), never a positive code or a `bool` (#478, #484, #509, #534).
- Initialize structs with designated initializers (`.spi = &flash_spi`), so a layout change cannot silently reassign fields (#534).
- Order struct fields to eliminate hidden padding: pointers first, then progressively narrower members. Field order is a memory decision on these targets, not a stylistic one (#396).
- Group related flags into a single bitfield word, one logical group per `uint8_t`, with an explicit `_reserved`/`_unused` member filling the remainder. Declare the group as one declaration with continuation members, as in `uint8_t display_timer : 4, gps_enabled : 1, _reserved : 3;` (#409). clang-format sometimes mangles bitfields; check its output (#467).
- For variable-length payloads use a flexible array member and allocate the whole object in one `malloc(sizeof(struct x) + n)`. This avoids a second allocation, avoids fragmentation, and lets you index `obj->payload[i]` instead of doing pointer arithmetic (#396, #456).
- Do not expose the wire layout. If a class owns the byte packing, keep the mask-and-shift inside its methods and keep the raw type out of the public API — exposing "how the sausage is made" invites misuse (#390).
- A fixed-size array beats a linked list when the count is small and bounded; a list is worth it only when the bound is large and rarely reached (#396).

### Layering and Ownership

This is the section with the most review traffic. The rule underneath all of it: each layer knows only
about the layer directly below it, and generic layers never learn a specific protocol's name.

- Never put hardware-specific code in `openrtx/`. Use `platform/` and the interfaces layer.
- Conversely, never put protocol or policy decisions in a driver. `radio_*` drivers manage the PLL and the signal path and nothing else; assigning a DMR ID, deciding a squelch policy, or picking a mode belongs to the OpMode handler (#102).
- **The UI never calls the radio API.** Set the `sync_rtx` flag that `ui_updateFSM` takes and let the rtx thread do the reconfiguration (#335).
- **LED state is set only in the dedicated FSM**, never from an OpMode (#360).
- State used by exactly one subsystem lives in that subsystem's state struct. A field only the UI reads belongs in the UI state (`ui_default.h`), not in the global `state` (#429).
- Generic structures stay generic. `struct pktDesc` must not grow a mode-specific field such as an M17 destination callsign; mode-specific configuration goes through `rtx_configure`. Likewise a minimum-packet-size check belongs in the OpMode, not in the generic rtx entry point (#427).
- Signal chains are split the way M17 is split: demodulator produces raw frames and knows only about syncwords; a separate deframer/decoder does unstuffing, CRC and protocol semantics. Keeping framing out of the demodulator is what lets a target with a hardware demodulator reuse the upper half (#396).
- Anything general-purpose belongs in the library, not in the module that first needed it. Date arithmetic goes to `core/datetime.h`, CRCs to `core/crc.h`, coordinate conversions to `core/gps.h` or `core/utils.h` — even when the only current caller is one driver (#360, #456, #498).
- Do not forward-declare a function from another translation unit. Include its header (#498).
- Scripts go in the repository-root `scripts/` folder, all of them together (#165).

### Hardware Targets and Drivers

- Peripheral drivers implement the standard interface in `openrtx/include/peripherals/` (`adc.h`, `gpio.h`, `i2c.h`, `spi.h`, `rtc.h`). Take `adc_stm32f4.c` as the reference implementation (#498).
- Cross-cutting concerns belong in the generic wrapper, implemented once for all drivers. `adc_getRawSample` takes the mutex; a driver's `sample()` must not take it again (#498).
- Peripheral instances — SPI devices, mutexes, config structs, the `platform` descriptor — are all declared in the target's `hwconfig.h`/`hwconfig.c`. Older targets scatter these around; new ones do not. `platform/targets/CS7000/` is the reference layout (#467, #498).
- Every GPIO gets a functional label in the target's `pinmap.h`, defined as `PORT, PIN` so the macro carries the full pin identity. Write `gpio_readPin(PTT_IN)`, never `gpio_readPin(GPIOB, 11)` (#498, #500).
- MCU register definitions live in `platform/mcu/<CHIP>/registers.h` as a struct cast onto the peripheral base address — the CMSIS pattern — not as a flat list of `#define`s. Name the folder after the chip (`HR_C7000`), not the CPU architecture (`CSKY_V2`) (#498).
- Target-specific pin and board constants do not belong in an MCU-level header; push them down to `platform/targets/<TARGET>/pinmap.h` (#498).
- Reuse the shared stubs in `platform/drivers/stubs/` during bring-up rather than writing target-local stub files (#498).
- Build configuration must not carry a default path that works on one developer's machine. Require the flag (#498).

### Initialization and Lifecycle

- **Explicit initialization only.** No `if (!x_ready) x_init();` lazy paths anywhere. A peripheral is initialized by its init function being called; if that does not happen it is a bug, and the bug should be visible rather than papered over. Implicit init hides defects (#498).
- Double initialization is an error, and it is better for it to have a noticeable effect than to be silently swallowed by a `bool initialized` guard (#500).
- Do not add empty `init`/`terminate` functions to a driver just to fill out the interface. Omit them (#403).
- Validate and set up at the top of a function, so an error path never leaves a half-constructed object behind (#427).
- When a class wraps a resource, use the constructor and destructor rather than `init`/`deinit` methods (#481). An `init(buffer)` or `reinit()` method that re-arms an existing object is fine (#427, #528).
- Prefer a `reset()` call over repeating field-clearing logic, so a future field only has to be handled in one place (#390).

### Concurrency and Real-Time

- **Never override `txDisable`.** It is a safety feature (#427).
- Do not add an API that lets any thread write into `rtxStatus` without a lock (#427).
- Do not take the state mutex from an rtx thread. The delay disrupts modulation timing; read-only access to state is safe without a critical section (#360, #396).
- Keep critical sections to the minimum number of statements that actually need protection (#396).
- Anything called from the rtx FSM must be enter-do-one-thing-exit. A loop that sends 25 frames at 40 ms each blocks the whole rtx thread for a second (#427).
- Do not call `ui_updateFSM` from feature code; the UI thread already does, and passing `NULL` will crash the firmware (#335).
- Do not use `volatile` unless the variable is genuinely shared with an interrupt handler (#500).
- In the signal path avoid per-frame or per-sample allocate/free cycles. Hold a buffer and swap pointers — the M17 demodulator's ready/working frame swap is the pattern to copy (#396). Allocating a large buffer when an operating mode starts and freeing it when the mode ends is the preferred shape, not an exception (#509); see [Embedded Constraints](#embedded-constraints).

### Persistent Data

Two structures cannot be reordered, ever, and both failure modes are silent:

- **`settings_t` (`openrtx/include/core/settings.h`)** — new fields are appended at the **bottom** of the struct. Inserting a field in the middle invalidates every user's saved settings (#340, #427).
- **`stringsTable_t` (`openrtx/include/ui/ui_strings.h`)** — new strings are appended at the **end**, and must be added to *every* language table (`EnglishStrings.h`, `SpanishStrings.h`, …) at the matching position. A missing or misordered entry breaks voice prompts and crashes the firmware once multi-language support lands (#335, #339, #353).
- Know which store a value belongs in. Settings are what the user sets independently of the tuned channel. Per-channel values belong to the channel; volatile operating values belong to the VFO state. Not everything that needs to survive a screen change is a setting (#390, #409). When unsure, ask before adding a field: M17 meta text went back and forth three times on #390.
- Default values derive from the hardware configuration, not from a constant. A hardcoded frequency default breaks VHF radios; see `cps.c` (#335).

### C++ Class Conventions

Bullets marked "M17 classes" describe what the formatted `protocols/M17` classes do rather than
something a reviewer asked for; they are the house style by example.

- Member order, top to bottom: public methods, private methods, private constants and enums, private variables. `protocols/M17/Demodulator.hpp` is the reference (#360, #390, #396, #417).
- Default to `private`. Expose a method only when application code outside the class actually calls it; a helper used only by `operator==` or by one public method should not be in the public section (#363, #390).
- Keep `std::string` out of protocol classes. Take `const char *` and a length (#363).
- Container-like classes follow `std::array` naming: `operator[]` and `data()`, not `payload()` returning an internal reference.
- Lookup tables are `static constexpr` (#396).
- Non-trivial method bodies go in the `.cpp`. Inline in the header only when there is a reason (#492).
- In a C++ translation unit, file-local functions are `static`; `extern "C"` is only for symbols C code actually calls (#500).
- Do not write a C-style wrapper around a method that already exists on a class instance — call `at1846s.i2c_readReg16(...)` directly, or add the properly-named method to the class (#500).
- Header guards are `#ifndef NAME_H` / `#define NAME_H` / `#endif // NAME_H`, not `#pragma once`; C++-only headers add `#ifndef __cplusplus` / `#error This header is C++ only!` (M17 classes, `FrameDecoder.hpp`).
- One class per file, named after the class (`Demodulator.hpp`, `Demodulator.cpp`). Declarations in the header, definitions in the `.cpp`; only trivial getters in the class body, no `inline` free functions in headers (#492, #522).
- Use constructors and destructors (#481); the M17 classes declare both explicitly even when empty, documented `Constructor.` and `Destructor.`, and initialize through the initializer list or a `reset()` call from the constructor. Access specifiers sit at column 0 (#166).
- Private constants are `static constexpr` in `UPPER_SNAKE_CASE`; make one public only when callers need it (#522). A `static constexpr` member that is odr-used, such as a `std::array` passed by reference, needs an out-of-class definition in the `.cpp` under C++14 (`Demodulator.cpp`).
- Data members are `camelCase` with no `m_` prefix or trailing underscore (M17 classes), grouped together rather than interleaved with methods (#500).
- Getters are `const` and return `const T &` where no copy is needed (M17 classes, `StreamFrame::data()`).
- Use `std::array` over raw arrays (#525); avoid `std::map` and other heap-allocating containers in firmware code (M17 classes).
- A method that produces a result returns `bool` meaning "a new result is ready", as `Demodulator::update()` and `sample()` do (#522).
- Includes are root-relative (`"protocols/M17/FrameDecoder.hpp"`), never `..`-relative (M17 classes).
- The tree does not use anonymous namespaces; file-local code is `static` (#500).
- Host-side tools (C or C++ programs that run on the build machine) parse arguments with `getopt`, following `scripts/c2tool.c`: a `printHelp()` built from `puts`, errors as `puts("Error: ...")` and a negative return (#522).

### Reuse Before Reimplementation

- Before writing a filter, use `openrtx/include/core/fir.hpp` or `iir.hpp`, or extend one with a fixed-point variant. Before writing a correlator, look at `protocols/M17/Correlator.hpp`. One implementation covering two cases is worth more than two tuned ones (#396).
- Use `ARRAY_SIZE` from `core/utils.h` instead of a magic number or a runtime size variable — it is a compile-time constant and optimizes better (#335, #429).
- No magic numbers. A macro or a `constexpr` (#335, #396).
- Check whether the platform already provides the behaviour before guarding it: `free(NULL)` is a no-op, `memcpy` of size 0 does nothing, a NULL dereference already faults loudly on both Linux and STM32. Defensive checks in internal helpers cost code space and hide bugs; the UI is expected to pass sane values (#376, #390). Trust callers inside the firmware; validate once at the boundary where data enters from outside it — off the air, from NVM, from a host. When a check is genuinely needed, put it once in the callee, the way `free(NULL)` behaves, rather than at every call site (#509).

### Conditional Compilation

- Do not wrap a module's declarations and definitions in `#ifdef CONFIG_*`. Unused functions and variables are garbage-collected at link time, so gating the *code* is unnecessary; put the guard at the call site if one is needed at all (#240, #492). Choosing between two implementations inside a `.c`/`.cpp` with a `CONFIG_` guard is fine (#500).
- Do gate anything that costs memory whether or not it is used — struct fields in `state` and `rtxStatus`, and their initialization. "You should not pay for what you do not use" (#396).
- Prefer making the directive disappear by supplying a default:
  ```c
  #ifndef CONFIG_CODEC_OVERSAMPLING
  #define CONFIG_CODEC_OVERSAMPLING 1
  #endif
  ```
  so callers need no `#ifdef` at all (#442).
- Select behaviour with a `CONFIG_` macro rather than shipping a weak default implementation that targets override. Targets with the feature define the config and provide the implementation, as with `CONFIG_RTC` (#500).
- When a `CONFIG_` block only matters to one translation unit, define it in the `.c`/`.cpp`, not the header (#492).

### Readability

- Prefer a `switch`-based state machine over a chain of `if`/`else if`; see the M17 demodulator (#390, #396).
- Prefer a table indexed by a computed value over a cascade of branches — build the index and index the array (#353, #358).
- Parenthesize each individual condition in a compound `if`/`while`: `if ((a == 0) && (b == 1))`. Logically redundant, deliberately kept; the M17 classes and the maintainers' own review suggestions do it (#529).
- But do not over-parenthesize an expression. Split it into named intermediate steps instead (#396).
- Declare a variable in the narrowest scope that works — inside the `for` body if that is where it is used (#414).
- File-scope `static` variables go at the top of the file, not inside a function (#306).
- Guard clauses return early: `if (printBufSize <= 1) return false;` beats wrapping the rest in an `else` (#390). But once a function has bookkeeping at its end, keep a single exit so a later edit cannot skip it (#509).
- Do not return by value something already filled through a reference or pointer parameter — it is a hidden copy (#390, #396).
- Use `memset` where you mean `memset`; it is clearer than a loop and compiles to the same thing (#390).
- Remove commented-out code, debug-only functions, unused declarations, and comments that restate the line below them (#340, #370, #396, #498).
- Call `strlen` once and keep the result (#390).

### Working With the Formatter

clang-format is a tool, not the authority. It is applied progressively, and readability wins where
they conflict.

- **Put a trailing comma on the last element of an enum, struct initializer or array initializer.** Without it clang-format produces mangled output, and this has been the cause of a large share of review comments (#450, #467, #498).
- Field alignment inside struct and enum definitions is intentional. If the formatter collapses aligned columns into single spaces, that is a readability regression to be worked around — not accepted (#166, #383).
- If the formatter mangles a construct, fix the input (usually a missing comma) or add a local exception — do not commit the mangled version (#383), and do not add the file to the exclusion list in `scripts/clang_format.sh`.
- `scripts/clang_format.sh` carries an **exclusion** list of files not yet converted. When you bring a file up to the current style, delete it from that list so the check enforces the style from then on. Note that the script only inspects files known to git: `git add` a new file before running the formatter.
- Reformatting an existing file always goes in its own dedicated commit, separate from behavioural changes (#494, and repeatedly elsewhere).

### Testing

- Unit tests use **Catch2** with `TEST_CASE` and `REQUIRE` macros; `SECTION` where cases share setup
- Tests live in `tests/unit/` and are registered in `meson.build` as `x_test = executable('x_test', sources : unit_test_src + ['tests/unit/X.cpp'], kwargs : unit_test_opts)` plus a `test('Descriptive Name', x_test)` line grouped with the others at the end of the file
- Test names use descriptive strings: `TEST_CASE("Golay24 encode/decode without errors", "[m17][golay]")`
- Helpers are file-scope `static` functions and `static constexpr` constants above the tests; includes are Catch2, a blank line, standard headers, a blank line, project headers
- CI runs `meson test` with `--test-args '--reporter junit'`, so every registered test executable must be a Catch2 binary, not a plain program (#522)
- A test that reads a committed asset takes the asset directory as an absolute path from `meson.project_source_root()` through a compile definition, never a path relative to the build directory (the 2021 demodulator test broke that way)
- PRs must document testing performed (automated and/or manual)
- Run a specific test: `meson test -C build_linux "Test Name"`

### String Table

The `stringsTable_t` in `openrtx/include/ui/ui_strings.h` defines UI strings and is indexed by voice prompts. **Never reorder existing entries** — only append new strings at the end (before the closing brace), and add the matching entry to every language table. See [Persistent Data](#persistent-data).

### Commits and PRs

- Each commit should be a distinct, complete change (no broken intermediate states)
- Split a PR along the lines of the code it touches — one commit per module or logical addition. A maintainer asked for exactly this split on #340, #342, #498; on #498 they spelled out the commit boundaries themselves (glue code / platform / display+backlight / keyboard / GPS)
- Commit messages are written in the imperative mood, first line a short summary, details in the body. Match the style in the `master` history (#342)
- Keep messages short: a summary line and a few plain lines of body. Long, dense messages get sent back (#513)
- Two unrelated features are two PRs, not two commits — GNSS and meta text were split on #342, syncword detection was split out of #360
- **Address review comments with fixup commits** (`git commit --fixup=<sha>`) and squash before merge, so the merged history contains no trace of rework. This keeps `git bisect` honest (#396, #494, #498)
- **Applying clang-format to existing files always gets its own commit**, never mixed with behaviour changes (#494)
- Do not add something in one commit and move it in another (#494)
- Aim for bisectable commits — each one building on its own. This is understood to be hard during early device bring-up and is a preference, not a gate (#498)
- Rebase before submission (no unnecessary merge commits). When a maintainer rebases or mainlines a branch under you, re-sync and force-push (#403, #492)
- Do not push new work onto a PR while a review block is outstanding; it makes the diff unreviewable. Open a second PR instead (#498)
- Put a PR back to draft while reworking, and mark it ready when done (#358)
- All files need SPDX headers; the `reuse` CI stage fails without them (#396)
- Each commit supported by an AI agent must contain its relevant "Co-authored-by" line in the commit message
- Each commit message should be prefaced with the module that the change relates to; common modules are: ui, m17, core, ci, test, nvm, build, drivers, scripts, rtx, protocols, reuse, audio, nvmem, linux; if a change uses multiple of these, it may be an indication that the change should be split into smaller commits
- Never add a git sign-off on behalf of the user; they must signoff the commits on their own; remind the user that they must do this before the PR can be considered ready
- All PRs raised by agents must be left in draft state; users must be responsible for taking it out of draft state once they've reviewed it; remind the user to do this

### Embedded Constraints

- C++ is compiled with `-fno-exceptions -fno-rtti -D__NO_EXCEPTIONS`
- Optimize for size (`-Os`) on embedded targets
- Be mindful of heap usage — `state.heap_usage` tracks it at runtime
- Prefer the heap over a static array for large or optional buffers. On STM32F4 targets `.data`/`.bss` live in the 64 KB CCM while the heap draws from the 128 KB main RAM, so a static buffer costs its full size whether or not the feature runs — pay for what you effectively use
- Custom linker scripts define memory layouts per platform
