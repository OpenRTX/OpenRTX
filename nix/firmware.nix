# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Factory for one Miosix-based firmware image, cross-compiled with the
# arm-miosix-eabi toolchain through the repository's own cross_<family>.txt.
#
# Notes on the build:
#   * All targets (Linux emulator included) are defined in every meson build
#     directory, so we never run a bare `ninja`; `meson compile` is given the
#     exact target names from targets.nix (which also resolves custom_target
#     names such as openrtx_gd77_wrap whose ninja name is the output file).
#   * find_program('radio_tool') is unconditional and required, so radio_tool
#     is on PATH for every configure.
#   * The cross files name the compilers by bare name; the toolchain is a
#     nativeBuildInput and is therefore on PATH. Nix's cc-wrapper flags
#     (NIX_CFLAGS_COMPILE, hardening) only reach the wrapped host compiler.
#   * meson.build shells out to `git describe`; there is no .git in the
#     sandbox, so that line is replaced with the version the flake computed.
#   * Both -ffile-prefix-map entries keep the toolchain and source store paths
#     out of the image: libstdc++'s _GLIBCXX_ASSERTIONS messages embed
#     __FILE__, which would otherwise change with every toolchain rebuild.
{
  lib,
  stdenv,
  meson,
  ninja,
  python3,
  miosix-toolchain,
  radio_tool,
  removeReferencesTo,
  subprojects,
  src,
  version,
  gitVersion,
  # Entry of targets.nix
  name,
  family,
  artifacts,
  description,
  flash ? null,
}:

stdenv.mkDerivation {
  pname = "openrtx-${name}";
  inherit version src;

  strictDeps = true;

  nativeBuildInputs = [
    meson
    ninja
    miosix-toolchain
    radio_tool
    removeReferencesTo
    # scripts/dfu-convert.py needs intelhex; the other helper scripts are
    # stdlib-only.
    (python3.withPackages (ps: [ ps.intelhex ]))
  ];

  # Offline resolution of the codec2 / XPowersLib wrap subprojects.
  MESON_PACKAGE_CACHE_DIR = subprojects.packageCache;

  postPatch = ''
    substituteInPlace meson.build \
      --replace-fail "run_command('git', 'describe', '--tags', '--dirty', '--always')" \
                     "run_command('echo', '${gitVersion}', check: true)"
    # Some helper scripts are executable and invoked through their
    # #!/usr/bin/env shebang, which does not exist in the sandbox.
    patchShebangs scripts
    substituteInPlace cross_${family}.txt \
      --replace-fail "'-D_DEFAULT_SOURCE=1'," \
                     "'-D_DEFAULT_SOURCE=1', '-ffile-prefix-map=${miosix-toolchain}=/miosix-toolchain', '-ffile-prefix-map=$PWD=/openrtx',"
  '';

  mesonFlags = [ "--cross-file=cross_${family}.txt" ];
  # The cross files carry -Os -g; nothing to add from buildtype.
  mesonBuildType = "plain";

  hardeningDisable = [ "all" ];

  # After mesonConfigurePhase the working directory is the build directory.
  buildPhase = ''
    runHook preBuild
    meson compile -j "$NIX_BUILD_CORES" ${lib.concatMapStringsSep " " (a: a.target) artifacts}
    runHook postBuild
  '';

  # meson.build has no install targets; copy the artifacts by exact name.
  dontUseMesonInstall = true;
  dontUseNinjaInstall = true;
  installPhase = ''
    runHook preInstall
    mkdir -p "$out"
    install -Dm644 openrtx_${name} "$out/openrtx_${name}.elf"
    ${lib.concatMapStringsSep "\n" (a: ''install -Dm644 "${a.file}" "$out/${a.install}"'') artifacts}
    # The toolchain's own libc/libgcc/libstdc++ were compiled with -g and
    # their DWARF names the toolchain store path, which would make the whole
    # compiler a runtime dependency of a 4 MB firmware package. The prefix
    # maps above cannot rewrite debug info that was baked in before, so blank
    # the hash in the ELF instead; the flashable images never contained it.
    remove-references-to -t ${miosix-toolchain} "$out/openrtx_${name}.elf"
    runHook postInstall
  '';
  disallowedReferences = [ miosix-toolchain ];

  # ARM ELF files and firmware images: keep the host strip/patchelf away.
  dontStrip = true;
  dontPatchELF = true;
  dontAuditTmpdir = true;

  passthru = {
    inherit
      family
      artifacts
      description
      gitVersion
      miosix-toolchain
      ;
  }
  // lib.optionalAttrs (flash != null) { inherit flash; };

  meta = {
    description = "OpenRTX firmware for the ${description}";
    homepage = "https://openrtx.org";
    license = lib.licenses.gpl3Plus;
    platforms = miosix-toolchain.meta.platforms;
  };
}
