# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Meson wrap-git subprojects, pinned to the revisions in subprojects/*.wrap,
# so that `meson setup` never touches the network.
#
# How this works (nixpkgs meson >= 1.3, verified against 1.10.2,
# mesonbuild/wrap/wrap.py):
#   * If subprojects/<dir> is missing and $MESON_PACKAGE_CACHE_DIR/<dir> (or
#     subprojects/packagecache/<dir>) is a directory, meson copies it into
#     subprojects/<dir>, applies the wrap's patch_directory overlay from
#     subprojects/packagefiles/<dir>, and writes the wrap hash file itself.
#   * -Dwrap_mode=nodownload (the nixpkgs meson hook default) only forbids the
#     git clone / URL download branch; the cache branch is taken first.
#   * If subprojects/<dir> already exists and has a meson.build, meson uses it
#     as-is: no overlay is applied. A hand-populated directory therefore has
#     to contain the packagefiles overlay already (see populateHook).
#
# Wrap directory names default to the wrap file basename; none of OpenRTX's
# wraps set `directory`, so the cache entries are named after the wraps.
{
  lib,
  fetchFromGitHub,
  linkFarm,
}:

let
  # Hashes are NAR hashes of the checkout without .git (nix-prefetch-git);
  # none of the wraps use clone-recursive, so no submodules are fetched.
  sources = {
    # subprojects/codec2.wrap: subproject('codec2') is unconditional. Native
    # builds prefer the system libcodec2; cross builds compile this source.
    codec2 = fetchFromGitHub {
      owner = "drowe67";
      repo = "codec2-dev";
      rev = "a298f3789d7ef9e829049ebe14da8845d97ba8c1";
      hash = "sha256-bV2C6By6nd1r908PsiItNGCNM8UuTpk5VynXo9F+YfQ=";
    };
    # subprojects/XPowersLib.wrap: subproject('XPowersLib') is unconditional;
    # its sources are only compiled by the Zephyr (ttwrplus) CMake build.
    XPowersLib = fetchFromGitHub {
      owner = "lewisxhe";
      repo = "XPowersLib";
      rev = "b0ed896c17d49a1b823ba0a6323cafd89b3a7d91";
      hash = "sha256-ijepXyvMTlieop6t79mf9iviLNQrMDgRO5v/UzxxJGY=";
    };
    # subprojects/catch2.wrap (v3.13.0): only the fallback for
    # dependency('catch2-with-main'); nixpkgs catch2_3 normally satisfies it.
    catch2 = fetchFromGitHub {
      owner = "catchorg";
      repo = "Catch2";
      rev = "b9eeca15d7598fae6a368c1c5d2462108f892849"; # v3.13.0
      hash = "sha256-WKp6/NX1SQJFLijW/fKwbR1FRoboAklDiHT6WqPRBjw=";
    };
    # subprojects/radio_tool.wrap pins `revision = head`, which cannot be
    # pinned. radio_tool.nix builds v0.3.0 as a proper package and puts it on
    # PATH, so find_program('radio_tool') never falls back to this wrap
    # (whose packagefiles meson.build runs cmake/make at configure time), and
    # the source is intentionally not fetched.
    # subprojects/tinyusb.wrap is not referenced by any meson.build or
    # CMakeLists.txt in the tree, so it is intentionally not fetched.
    # (hathach/tinyusb @ 4bfab30c02279a0530e1a56f4a7c539f2d35a293)
  };

  # Directory-form package cache: <cache>/<wrap directory name>/...
  packageCache = linkFarm "openrtx-meson-packagecache" (
    lib.mapAttrsToList (name: path: { inherit name path; }) sources
  );

  # Physically populate subprojects/<name> with the packagefiles overlay
  # applied, exactly like meson's apply_patch() (later files win). Needed by
  # builds that read subprojects/<name>/src outside of meson (Zephyr/CMake).
  # Run from the source root.
  populateHook = ''
    ${lib.concatMapStringsSep "\n" (name: ''
      if [ ! -e "subprojects/${name}/meson.build" ]; then
        mkdir -p "subprojects/${name}"
        cp -r --no-preserve=mode,ownership "${sources.${name}}"/. "subprojects/${name}"/
        if [ -d "subprojects/packagefiles/${name}" ]; then
          cp -r --no-preserve=mode,ownership "subprojects/packagefiles/${name}"/. "subprojects/${name}"/
        fi
      fi
    '') (builtins.attrNames sources)}
  '';
in
{
  inherit sources packageCache populateHook;
}
