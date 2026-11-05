# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# apps.flash-<target>: the same flashing commands meson.build runs for the
# openrtx_<target>_flash custom targets, wrapped around the built firmware.
# Linux only: needs USB access (see 99-openrtx.rules / nixosModules.default).
{
  lib,
  writeShellApplication,
  dfu-util,
  radio_tool,
  python3,
  src,
  name,
  description,
  firmware,
  flash,
}:

let
  image = "${firmware}/${flash.file}";
  pythonEnv = python3.withPackages (ps: [
    ps.pyusb
    ps.urllib3
  ]);
  commands = {
    radio_tool = ''
      exec radio_tool -d 0 -f -i "${image}" "$@"
    '';
    gd77-loader = ''
      exec python3 ${src}/scripts/gd-77_firmware_loader.py -f "${image}" -m ${flash.wrapMode} "$@"
    '';
    dfu-util = ''
      exec dfu-util -d 0483:df11 -a 0 -D "${image}" -s ${flash.loadAddr} "$@"
    '';
    uf2 = ''
      exec python3 ${src}/scripts/uf2conv.py -D "${image}" "$@"
    '';
  };
in
writeShellApplication {
  name = "flash-${name}";
  runtimeInputs = [
    dfu-util
    radio_tool
    pythonEnv
  ];
  text = ''
    echo "Flashing ${flash.file} (${
      firmware.gitVersion or "unknown version"
    }) to ${name} using ${flash.method}" >&2
    ${commands.${flash.method}}
  '';
  meta = {
    description = "Flash OpenRTX onto a ${description}";
    license = lib.licenses.gpl3Plus;
    platforms = lib.platforms.linux;
  };
}
