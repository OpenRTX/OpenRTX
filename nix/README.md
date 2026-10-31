# Building OpenRTX with Nix

The flake at the repository root builds everything hermetically: every
firmware image, the Linux emulator and its unit tests, the `arm-miosix-eabi`
cross toolchain (GCC 9.2.0-mp3.2, built from source with the Miosix patches),
`radio_tool`, the Zephyr SDK for the T-TWR Plus, and the flashing helpers.
Nothing is downloaded during a build: the Meson wrap subprojects are fetched
by Nix and served to Meson through its package cache, and the firmware reports
its version as `v<meson version>-<short rev>` derived from the flake's own
revision instead of `git describe` (`-dirty` for uncommitted trees, `-unknown`
when built from a non-git source).

## Quick start

```sh
nix build .#openrtx-md3x0        # result/openrtx_md3x0.bin (+ raw bin, ELF)
nix build .#openrtx-gd77         # result/openrtx_gd77.sgl
nix build .#openrtx-ttwrplus     # result/openrtx_ttwrplus.uf2 (Zephyr)
nix build .#all                  # every firmware image + the emulator
nix run .                        # Linux emulator (SDL2 window)
nix run .#emulator-mod17         # Module17 UI variant
nix run .#flash-md3x0            # flash a radio in bootloader mode (Linux)
nix flake check                  # unit tests, ASan unit tests, lints
nix develop                      # meson + toolchain + tools dev shell
nix develop .#zephyr             # west + Zephyr SDK dev shell
nix fmt                          # format the Nix files (nix fmt -- --ci to check)
```

The first cross build compiles the toolchain, which takes about 15 minutes
on 16 cores. The result is bit-for-bit reproducible, so a binary cache can
serve it afterwards.

## Outputs

| Output | Contents |
| --- | --- |
| `packages.<sys>.openrtx-<radio>` | one directory per radio: `openrtx_<radio>.<bin/sgl>` as published by CI, the other images, and the ELF with debug info |
| `packages.<sys>.openrtx-ttwrplus` | T-TWR Plus: `.uf2`, `.bin`, `.elf`, bootloader (Linux only) |
| `packages.<sys>.all` (default) | `symlinkJoin` of every firmware package and the emulator |
| `packages.<sys>.emulator` | `openrtx_linux`, `openrtx_linux_smallscreen`, `openrtx_linux_mod17` |
| `packages.<sys>.emulator-asan` | same, built with `-Dasan=true` |
| `packages.<sys>.emulator-coverage` | runs the unit tests with coverage; gcovr report under `share/openrtx/coverage` |
| `packages.<sys>.miosix-toolchain` | `arm-miosix-eabi-{gcc,g++,gdb,...}` and `mx-postlinker` |
| `packages.<sys>.radio_tool`, `zephyr-sdk`, `udev-rules` | host tools and `99-openrtx.rules` |
| `packages.<sys>.flash-<radio>`, `apps.<sys>.flash-<radio>` | the flashing command `meson compile openrtx_<radio>_flash` would run (Linux only) |
| `apps.<sys>.emulator*`, `apps.<sys>.radio_tool` | run the emulator variants / `radio_tool` |
| `checks.<sys>.*` | `unit-tests`, `unit-tests-asan`, `nix-lint`, `reuse-lint`, `packages-present` |
| `devShells.<sys>.{default,zephyr}` | development environments |
| `overlays.default` | the `pkgs.openrtx.*` scope, for other flakes |
| `nixosModules.default` | installs the udev rules (`services.udev.packages`) |
| `formatter.<sys>` | `nixfmt` |

Radios: `md3x0 mduv3x0 md9600 dm1701 gd77 dm1801 mod17 cs7000 rt4d`
(Cortex-M4, `cross_cm4.txt`), `cs7000p` (Cortex-M7, `cross_cm7.txt`),
`ttwrplus` (ESP32-S3, Zephyr). The list lives in `nix/targets.nix` and mirrors
the `targets` table in `meson.build`.

Systems: `x86_64-linux` is tested. `aarch64-linux` should work (nothing in the
toolchain recipe is x86-specific) but has not been built. On `aarch64-darwin`
the Miosix toolchain, the firmware images, the Zephyr SDK and image, the udev
rules and the flash apps are not exposed: the toolchain recipe needs extra
patches on macOS and none of it has been validated there. The emulator
packages, apps and checks, `radio_tool` and the default dev shell are exposed
on macOS but untested; `packages.all` there contains only the emulator.

## Flashing on NixOS

```nix
# flake.nix
inputs.openrtx.url = "github:OpenRTX/OpenRTX";

# configuration.nix (or any NixOS module)
imports = [ inputs.openrtx.nixosModules.default ];
```

Then, with the radio in bootloader mode, `nix run .#flash-<radio>`. Extra
arguments are passed through to `radio_tool`, `dfu-util`, the GD-77 loader or
`uf2conv.py`.

## Design notes

* `nixpkgs` is the only input. It follows `nixpkgs-unstable` (newest meson,
  nixfmt and Python packages) and `flake.lock` pins the revision on which the
  toolchain, emulator, tests and Zephyr build were verified.
* Every component is a `callPackage`-style file under `nix/`, instantiated in
  the `pkgs.openrtx` scope defined by `overlays.default`; `packages`, `apps`,
  `checks` and `devShells` are views on that scope.
* The toolchain (`nix/miosix-toolchain.nix`) replays the Miosix kernel's
  `install-script.sh` in a single derivation. nixpkgs' own GCC infrastructure
  cannot be reused: gcc 9 is gone from nixpkgs, `arm-miosix-eabi` is not a
  triple `lib.systems` understands, and the Miosix patches change the thread
  model and newlib configuration. It is an unwrapped compiler on `PATH`, so the
  nixpkgs `cc-wrapper` flags never reach it.
* Firmware derivations pass the repository's own `cross_cm4.txt` /
  `cross_cm7.txt` to Meson, build the targets CI builds (plus the wrapped
  CS7000 image, which CI does not build) with `meson compile`, and copy the
  artifacts by name. `-ffile-prefix-map` keeps store paths out of the images
  so a toolchain rebuild does not change them; the ELF is scrubbed with
  `remove-references-to` because the toolchain's own libc carries its store
  path in its debug info, and the derivation refuses to reference the
  toolchain at runtime.
* `meson.build` calls `subproject('codec2')` and `subproject('XPowersLib')`
  unconditionally and `find_program('radio_tool')` is required, so every
  derivation gets the pinned subprojects via `MESON_PACKAGE_CACHE_DIR` and a
  real `radio_tool` on `PATH`. The `tinyusb` wrap is unused and not fetched.
* The Zephyr build fetches the three `west.yml` projects at their pinned
  revisions and turns them into local git repositories with a `manifest-rev`
  branch, which is what `west` needs to treat them as up to date. Those
  throw-away commits use a fixed date and `BUILD_VERSION` is set to the pinned
  Zephyr revision, so the image is reproducible. The SDK is the same 0.17.0
  release CI uses; its Python-enabled gdb variants are dropped because they
  link a libpython nixpkgs no longer ships.
* `nix flake check` only builds the emulator tests and the lints. Firmware
  images are heavy; build them explicitly with `nix build .#all`.

## Updating pins

* nixpkgs: `nix flake update nixpkgs`, then `nix flake check` and
  `nix build .#all` before committing `flake.lock`. The fragile spots on a
  rolling branch are `gcc13Stdenv` in `nix/miosix-toolchain.nix` (gdb 9.1 does
  not build with newer GCC), the sdl2-compat/SDL3 workaround for the ASan
  tests in `nix/emulator.nix`, and the Zephyr Python packages in
  `nix/zephyr-python.nix`. Run `nix fmt` afterwards: `checks.nix-lint` runs
  `nixfmt --check`.
* Wrap subprojects: keep `nix/subprojects.nix` in sync with
  `subprojects/*.wrap` (`nix-prefetch-git` gives the hash).
* Miosix toolchain recipe: `miosixRev` in `nix/miosix-toolchain.nix` mirrors
  `MIOSIX_KERNEL_SHA` in `.devcontainer/Dockerfile`.
* Zephyr: `nix/ttwrplus.nix` mirrors `west.yml`; `nix/zephyr-sdk.nix` mirrors
  the `sdk-version` of the CI job.
* `radio_tool`: `nix/radio_tool.nix` mirrors the tag in
  `.devcontainer/Dockerfile`.
