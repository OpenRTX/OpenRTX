# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Single source of truth for the Miosix-based firmware targets. Mirrors the
# `targets` table in meson.build; the artifact CI publishes for each radio
# (.github/workflows/main.yml) is installed as openrtx_<radio>.<ext>.
#
#   family    : cross file to use (cross_<family>.txt)
#   artifacts : meson targets to build, the file each writes into the build
#               directory, and the name it is installed under in $out.
#   flash     : recipe for apps.flash-<name> (see flash.nix); omitted when the
#               upstream flashing path is not usable.
#
# The T-TWR Plus (ttwrplus, ESP32-S3/Zephyr) is not in this table: it has its
# own derivation in ttwrplus.nix.
let
  # TYT-style radios: raw binary + radio_tool wrapped image (the release file).
  tyt = name: description: {
    inherit description;
    family = "cm4";
    artifacts = [
      {
        target = "openrtx_${name}_bin";
        file = "openrtx_${name}_bin";
        install = "openrtx_${name}_raw.bin";
      }
      {
        target = "openrtx_${name}_wrap";
        file = "openrtx_${name}_wrap";
        install = "openrtx_${name}.bin";
      }
    ];
    flash = {
      method = "radio_tool";
      file = "openrtx_${name}.bin";
    };
  };

  # GD-77 family: raw binary + SGL image for the OpenGD77 firmware loader.
  # wrapMode is meson.build's t['wrap'] for the radio.
  gdx = name: wrapMode: description: {
    inherit description;
    family = "cm4";
    artifacts = [
      {
        target = "openrtx_${name}_bin";
        file = "openrtx_${name}_bin";
        install = "openrtx_${name}_raw.bin";
      }
      {
        target = "openrtx_${name}_wrap";
        file = "openrtx_${name}_bin.sgl";
        install = "openrtx_${name}.sgl";
      }
    ];
    flash = {
      method = "gd77-loader";
      file = "openrtx_${name}.sgl";
      inherit wrapMode;
    };
  };

  # Connect Systems CS7000 family: raw binary (what dfu-util flashes), DFU
  # image and cs7000_wrap.py wrapped image. CI publishes the raw binary for
  # the CS7000 and the wrapped image for the CS7000 Plus as the .bin release.
  connectSystems = name: family: loadAddr: releaseIsWrapped: description: {
    inherit description family;
    artifacts = [
      {
        target = "openrtx_${name}_bin";
        file = "openrtx_${name}_bin";
        install = if releaseIsWrapped then "openrtx_${name}_raw.bin" else "openrtx_${name}.bin";
      }
      {
        target = "openrtx_${name}_dfu";
        file = "openrtx_${name}_dfu";
        install = "openrtx_${name}.dfu";
      }
      {
        target = "openrtx_${name}_wrap";
        file = "openrtx_${name}_wrap";
        install = if releaseIsWrapped then "openrtx_${name}.bin" else "openrtx_${name}_wrapped.bin";
      }
    ];
    flash = {
      method = "dfu-util";
      file = if releaseIsWrapped then "openrtx_${name}_raw.bin" else "openrtx_${name}.bin";
      inherit loadAddr;
    };
  };
in
{
  md3x0 = tyt "md3x0" "TYT MD-380, MD-390, Retevis RT3, Retevis RT8";
  mduv3x0 = tyt "mduv3x0" "TYT MD-UV380, MD-UV390, Retevis RT3s";
  md9600 = tyt "md9600" "TYT MD-9600";
  dm1701 = tyt "dm1701" "Baofeng DM-1701, Retevis RT84";
  gd77 = gdx "gd77" "GD-77" "Radioddity GD-77";
  dm1801 = gdx "dm1801" "DM-1801" "Baofeng DM-1801";
  cs7000 = connectSystems "cs7000" "cm4" "0x08000000" false "Connect Systems CS7000-M17";
  cs7000p = connectSystems "cs7000p" "cm7" "0x08100000" true "Connect Systems CS7000-M17 Plus";
  mod17 = {
    description = "Module17";
    family = "cm4";
    artifacts = [
      {
        target = "openrtx_mod17_bin";
        file = "openrtx_mod17_bin";
        install = "openrtx_mod17.bin";
      }
    ];
    flash = {
      method = "dfu-util";
      file = "openrtx_mod17.bin";
      loadAddr = "0x08000000";
    };
  };
  rt4d = {
    description = "Radtel RT-4D (work in progress)";
    family = "cm4";
    artifacts = [
      {
        target = "openrtx_rt4d_bin";
        file = "openrtx_rt4d_bin";
        install = "openrtx_rt4d.bin";
      }
    ];
    # meson.build has no usable wrap/flash entry for this radio yet.
  };
}
