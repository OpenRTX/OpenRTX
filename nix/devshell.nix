# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Development shell with everything the Meson builds, the unit tests, the
# helper scripts and the CI checks need. The pinned wrap subprojects are
# exposed through MESON_PACKAGE_CACHE_DIR, so `meson setup` works offline.
{
  lib,
  stdenv,
  mkShell,
  meson,
  ninja,
  pkg-config,
  cmake,
  git,
  sdl2-compat,
  codec2,
  catch2_3,
  readline,
  libusb1,
  dfu-util,
  openocd,
  gcovr,
  clang-tools,
  reuse,
  nixfmt,
  statix,
  deadnix,
  ffmpeg,
  python3,
  miosix-toolchain,
  radio_tool,
  subprojects,
}:

mkShell {
  name = "openrtx";

  packages = [
    meson
    ninja
    pkg-config
    cmake
    git
    sdl2-compat
    codec2
    catch2_3
    readline
    libusb1
    dfu-util
    openocd
    gcovr
    clang-tools
    reuse
    nixfmt
    statix
    deadnix
    ffmpeg
    radio_tool
    # requirements.txt plus what scripts/*.py import
    (python3.withPackages (
      ps: with ps; [
        setuptools
        pyusb
        intelhex
        urllib3
        numpy
        cairosvg
        pillow
      ]
    ))
  ]
  ++ lib.optional (lib.meta.availableOn stdenv.hostPlatform miosix-toolchain) miosix-toolchain;

  MESON_PACKAGE_CACHE_DIR = subprojects.packageCache;

  shellHook = ''
    echo "OpenRTX development shell"
    echo "  meson setup build_linux && meson compile -C build_linux openrtx_linux"
    echo "  meson setup --cross-file cross_cm4.txt build_cm4 && meson compile -C build_cm4 openrtx_md3x0_wrap"
    echo "  (wrap subprojects are served offline from MESON_PACKAGE_CACHE_DIR)"
  '';
}
