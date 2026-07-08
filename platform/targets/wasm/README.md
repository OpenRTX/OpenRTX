<!--
SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
SPDX-License-Identifier: GPL-3.0-or-later
-->

# OpenRTX WebAssembly emulator

A browser build of the OpenRTX Linux emulator, compiled to WebAssembly with
[Emscripten](https://emscripten.org/). It runs the exact same firmware core, UI,
and M17/FM DSP as the native `linux` target — only the platform edges (terminal
CLI, filesystem paths) are adapted for the browser.

## How it works

The emulator is built on POSIX threads that share state through mutexes,
condition variables, and a hand-rolled channel (`openrtx/src/core/chan.c`).
Rather than rewrite that concurrency model, the WASM build maps it onto
Emscripten's pthreads (Web Workers backed by a `SharedArrayBuffer`) and uses
**`-sPROXY_TO_PTHREAD`**, which runs `main()` on a worker. The browser's main
thread stays free for the event loop and for SDL2 rendering, so the existing
blocking loops in `threads.c` / `sdl_engine.c` keep working unchanged.

Two edges are adapted (both guarded by `#ifdef __EMSCRIPTEN__`):

- **CLI** — the native build runs a GNU readline shell on its own thread. The
  browser has no terminal, so `emulator.c` exports `emulator_command()` to
  JavaScript instead; the page's command box feeds lines straight into the same
  `process_line()` grammar (screenshots, scripted keypresses, etc.).
- **NVM** — `nvmem_linux.c` has no `$HOME`/XDG dirs in the browser, so radio
  state is stored at `/persist/state.bin` in the Emscripten virtual filesystem.
  The codeplug is created on first run via the existing `cps_create()` path.

## Prerequisites

Install and activate the [Emscripten SDK](https://emscripten.org/docs/getting_started/downloads.html):

```sh
git clone https://github.com/emscripten-core/emsdk.git
cd emsdk && ./emsdk install latest && ./emsdk activate latest
source ./emsdk_env.sh          # puts emcc/em++ on PATH
```

You also need `meson` and `ninja`.

## Build

From the repository root:

```sh
meson setup build_wasm --cross-file cross_wasm.txt
ninja -C build_wasm wasm
```

This produces `build_wasm/openrtx_wasm.html`, `openrtx_wasm.js`,
`openrtx_wasm.wasm`, and `openrtx_wasm.worker.js`.

## Run

WebAssembly threads require a
[cross-origin-isolated](https://web.dev/coop-coep/) page, i.e. the
`Cross-Origin-Opener-Policy` and `Cross-Origin-Embedder-Policy` response
headers. A plain file:// open or a vanilla static server will **not** work.
Use the bundled dev server, which sets those headers:

```sh
python scripts/serve_wasm.py build_wasm
# then open http://localhost:8000/openrtx_wasm.html
```

Click the screen to give the canvas keyboard focus. Arrow keys navigate,
`Enter` selects, `Esc` goes back, `0`–`9` are the keypad, `PgUp`/`PgDn` turn the
knob, `M` is monitor.

## Status / known limitations

This is an initial port focused on getting the UI running in the browser.

- **Persistence is ephemeral.** State lives in MEMFS and is lost on reload.
  Mounting IDBFS at `/persist` (in `Module.preRun`) plus periodic `FS.syncfs`
  would persist it — deferred, as IDBFS interacts awkwardly with
  `PROXY_TO_PTHREAD`.
- **No live audio.** The Linux target has no real audio backend (audio is
  file-based); Web Audio integration for M17/FM monitoring is a follow-up.
- **Busy-wait render loop.** `sdlEngine_run()` polls without yielding (as it
  does natively); on a worker this pegs a core. A small `SDL_Delay` would be
  gentler in the browser.
- The `codec2` subproject is compiled from source under emcc; if it needs
  toolchain tweaks they are independent of this target.
