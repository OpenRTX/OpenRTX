# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# 99-openrtx.rules packaged for `services.udev.packages`.
{
  lib,
  runCommand,
  src,
}:
runCommand "openrtx-udev-rules"
  {
    meta = {
      description = "udev rules granting access to OpenRTX radios in bootloader mode";
      homepage = "https://openrtx.org";
      license = lib.licenses.gpl3Plus;
      platforms = lib.platforms.linux;
    };
  }
  ''
    install -Dm644 ${src}/99-openrtx.rules "$out/lib/udev/rules.d/99-openrtx.rules"
  ''
