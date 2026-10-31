# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Development shell for the T-TWR Plus (ESP32-S3, Zephyr RTOS) target.
# `west update` still fetches the Zephyr tree from the network; see the
# instructions printed on entry. For a fully offline build use
# `nix build .#openrtx-ttwrplus`.
{
  mkShell,
  zephyr-sdk,
  zephyrPython,
  cmake,
  ninja,
  dtc,
  gperf,
  git,
  meson,
  pkg-config,
  esptool,
  dfu-util,
  libusb1,
  radio_tool,
  subprojects,
}:

let
  pythonEnv = zephyrPython.override {
    # requirements.txt
    extraPackages =
      ps: with ps; [
        setuptools
        pyusb
        numpy
        cairosvg
        urllib3
      ];
  };
in
mkShell {
  name = "openrtx-zephyr";

  packages = [
    zephyr-sdk
    pythonEnv
    cmake
    ninja
    dtc
    gperf
    git
    meson
    pkg-config
    esptool
    dfu-util
    libusb1
    # meson.build's find_program('radio_tool') is required even for this target.
    radio_tool
  ];

  ZEPHYR_SDK_INSTALL_DIR = zephyr-sdk;
  ZEPHYR_TOOLCHAIN_VARIANT = "zephyr";
  MESON_PACKAGE_CACHE_DIR = subprojects.packageCache;

  shellHook = ''
    cat <<'MSG'
    OpenRTX Zephyr (T-TWR Plus) shell.

    One-time workspace setup (west needs a workspace above the repository):
      cd ..                      # parent of the OpenRTX checkout
      west init -l OpenRTX       # registers this repository as the manifest
      west update                # fetches zephyr, mcuboot, hal_espressif
      cd OpenRTX

    Build:
      meson setup build && meson compile -C build openrtx_ttwrplus_uf2
    MSG
  '';
}
