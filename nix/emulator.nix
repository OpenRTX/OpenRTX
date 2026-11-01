# SPDX-FileCopyrightText: Copyright 2020-2026 OpenRTX Contributors
#
# SPDX-License-Identifier: GPL-3.0-or-later
#
# OpenRTX Linux emulator (three UI variants) and, with doCheck, the Catch2
# unit tests.
#
# Notes on the build (verified against nixpkgs meson 1.10.2):
#   * meson.build never sets install:true, so `meson install` installs
#     nothing: hand-written installPhase.
#   * The default ninja target also links the ARM firmware executables with
#     the host compiler, which fails: only the emulator targets are built.
#   * dependency('catch2-with-main') is required even when tests are not run,
#     so catch2 is a buildInput rather than a checkInput.
#   * codec2 is resolved through cpp.find_library('codec2') (the wrap's
#     [provide] section short-circuits the pkg-config lookup); the nixpkgs
#     codec2 dev and lib outputs make that succeed.
#   * nixpkgs SDL2 is sdl2-compat, which dlopens libSDL3 through its own
#     RUNPATH; AddressSanitizer intercepts dlopen and loses that RUNPATH, so
#     the ASan variant points LD_LIBRARY_PATH at the same SDL3 build.
{
  lib,
  stdenv,
  meson,
  ninja,
  pkg-config,
  python3,
  sdl2-compat,
  sdl3,
  codec2,
  catch2_3,
  readline,
  gcovr,
  radio_tool,
  subprojects,
  src,
  version,
  gitVersion,
  withAsan ? false,
  withCoverage ? false,
  doCheck ? false,
}:

let
  variant = lib.optionalString withAsan "-asan" + lib.optionalString withCoverage "-coverage";

  # The exact sdl3 that sdl2-compat links against and dlopens at runtime.
  sdl3ForCompat = lib.getLib (sdl3.override { traySupport = false; });

  targets = [
    "openrtx_linux"
    "openrtx_linux_smallscreen"
    "openrtx_linux_mod17"
  ];
in
stdenv.mkDerivation {
  pname = "openrtx-emulator${variant}";
  inherit version src;

  strictDeps = true;

  nativeBuildInputs = [
    meson
    ninja
    pkg-config
    python3
    radio_tool
  ]
  ++ lib.optional withCoverage gcovr;

  buildInputs = [
    sdl2-compat
    codec2
    readline
    catch2_3
  ];

  MESON_PACKAGE_CACHE_DIR = subprojects.packageCache;

  postPatch = ''
    substituteInPlace meson.build \
      --replace-fail "run_command('git', 'describe', '--tags', '--dirty', '--always')" \
                     "run_command('echo', '${gitVersion}', check: true)"
    patchShebangs scripts
  '';

  # buildtype=plain (nixpkgs default) plus the fortify hardening flag gives
  # -O2; upstream's default is meson's `debug`.
  mesonFlags = lib.optional withAsan "-Dasan=true" ++ lib.optional withCoverage "-Db_coverage=true";

  buildPhase = ''
    runHook preBuild
    meson compile -j "$NIX_BUILD_CORES" ${lib.concatStringsSep " " targets}
    runHook postBuild
  '';

  # _FORTIFY_SOURCE and AddressSanitizer do not mix.
  hardeningDisable = lib.optionals withAsan [
    "fortify"
    "fortify3"
  ];

  # openrtx/src/core/voicePromptData.S has no .note.GNU-stack section, so GNU
  # ld would otherwise give the emulator an executable stack.
  env = lib.optionalAttrs stdenv.hostPlatform.isLinux { NIX_CFLAGS_COMPILE = "-Wa,--noexecstack"; };

  inherit doCheck;
  # mesonCheckPhase runs `meson test --no-rebuild --print-errorlogs` after
  # building the test prerequisites.
  preCheck = ''
    export SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy
  ''
  + lib.optionalString withAsan ''
    export LD_LIBRARY_PATH=${sdl3ForCompat}/lib''${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
  '';

  postCheck = lib.optionalString withCoverage ''
    mkdir -p coverage
    gcovr . --root .. --filter '../openrtx/src/' --filter '../openrtx/include/' \
      --exclude '.*subprojects.*' --exclude '.*test.*' \
      --cobertura coverage/cobertura.xml --markdown coverage/coverage.md --print-summary
  '';

  dontUseMesonInstall = true;
  dontUseNinjaInstall = true;
  installPhase = ''
    runHook preInstall
    install -Dm755 -t "$out/bin" ${lib.concatStringsSep " " targets}
    if [ -f meson-logs/testlog.txt ]; then
      install -Dm644 meson-logs/testlog.txt "$out/share/openrtx/testlog.txt"
    fi
    if [ -d coverage ]; then
      mkdir -p "$out/share/openrtx"
      cp -r coverage "$out/share/openrtx/coverage"
    fi
    runHook postInstall
  '';

  passthru = {
    inherit gitVersion;
  };

  meta = {
    description =
      "OpenRTX Linux emulator"
      + lib.optionalString withAsan " (AddressSanitizer)"
      + lib.optionalString withCoverage " (coverage)";
    homepage = "https://openrtx.org";
    license = lib.licenses.gpl3Plus;
    # Builds on Linux; macOS is expected to work (meson.build handles the
    # linker differences) but has not been verified.
    platforms = lib.platforms.unix;
    mainProgram = "openrtx_linux";
  };
}
