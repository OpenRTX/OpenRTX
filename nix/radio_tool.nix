# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# radio_tool: wraps and flashes TYT / Connect Systems firmware images.
# meson.build does an unconditional, required find_program('radio_tool'), so
# this must be on PATH for every OpenRTX configure (native builds included).
# Pinned to v0.3.0, the same tag as .devcontainer/Dockerfile.
{
  lib,
  stdenv,
  fetchFromGitHub,
  cmake,
  pkg-config,
  libusb1,
}:

stdenv.mkDerivation {
  pname = "radio_tool";
  version = "0.3.0";

  src = fetchFromGitHub {
    owner = "v0l";
    repo = "radio_tool";
    rev = "cc90219e2f29873fffb7c4d3fa8d1d6af1883274"; # tag v0.3.0
    hash = "sha256-mdl0Z6bzuEbULXG0qW+Gfpk+adafOlXvPihUNJlwty8=";
  };

  nativeBuildInputs = [
    cmake
    pkg-config
  ];
  buildInputs = [ libusb1 ];

  # CMakeLists.txt uses GetGitRevisionDescription; without .git the version
  # string reports "HEAD-HASH-NOTFOUND", which is harmless.
  cmakeFlags = [ "-DBUILD_TESTING=OFF" ];

  meta = {
    description = "Universal radio firmware flashing and wrapping tool (TYT, Connect Systems)";
    homepage = "https://github.com/v0l/radio_tool";
    license = lib.licenses.gpl3Plus;
    mainProgram = "radio_tool";
    platforms = lib.platforms.unix;
  };
}
